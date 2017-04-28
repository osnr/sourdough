#include <iostream>
#include <list>
#include <algorithm>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug || true ), the_window_size(10), inflight(0), tau(45),
  err(), err_max_sz(201), recent_acks(), acks_max_sz(15),
  head_ack_time(0), recent_tau(), tau_max_sz(91), timeout_delay(500), last_ack_proc(0),
  bw(), bw_max_sz(91), bw_quant(0.5), cumul(0)
{
  err.resize(err_max_sz, 0);
  recent_acks.resize(acks_max_sz, 1);
  recent_tau.resize(tau_max_sz, tau);
  bw.resize(bw_max_sz, 0.1);
}


/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  // Note: Seems to have little to no effect, though sounds nice.
  // If we haven't received any ack in a long time, there's likely a long
  // queue, so reduce window size.
 // if (((int)last_ack_proc - (int)timestamp_ms()) > (0.5*timeout_delay)) {
 //   the_window_size -= 1;
 // }

    if ( debug_ ) {
      cerr << "At time " << timestamp_ms()
           << " window size is " << the_window_size
           << endl;
    }
  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  inflight++;

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  last_ack_proc = timestamp_ack_received;
  inflight--;

  // Save the most recent propagation delay estimates.
  recent_tau = recent_tau.cshift(1);
  recent_tau[0] = (timestamp_ack_received - send_timestamp_acked) / 2;
  valarray<unsigned int> sorted_tau = recent_tau;
  sort(begin(sorted_tau), end(sorted_tau));
  // Keep a low quantile (Heavy tailed distribution, so pick a low quantile
  // to increase chance of being near true value of tau). Greater than the
  // timeout delay is not actionable, so set it as max.
  tau = min(sorted_tau[(tau_max_sz*0.2)], timeout_delay/2);

  // Only keep stats on bandwidth if RTT is reasonable. Otherwise,
  // this incoming packet is not representative of current bw.
  if (recent_tau[0] < timeout_delay/2) {
    unsigned int time_diff = timestamp_ack_received - head_ack_time;
    recent_acks = recent_acks.shift(time_diff);
    recent_acks[0]++;
    // Track last time parameter estimates were updated for possible use later.
    head_ack_time = timestamp_ack_received;
    bw = bw.cshift(1);
    bw[0] = (float)recent_acks.sum() / (float)acks_max_sz;
  }

    // Quantiled bw
    valarray<float>bw_sort = bw;
    sort(begin(bw_sort), end(bw_sort));
    float bw_est = bw_sort[(float)bw_max_sz*bw_quant];

    // BDP - inflight 
    // "Predict" bandwidth with a stupid tangent fit. Lower tau seems to
    // yield better results (i.e. we can't actually precit too far, or
    // higher derivatives too large).
    float bw_pred = bw_est + 0.5*tau*(bw[0] - bw[bw_max_sz-1]) / bw_max_sz;
    bw_pred = bw_pred < 0 ? 0 : bw_pred;

    float cur_err = bw_pred*tau - inflight;
    err = err.cshift(1);
    err[0] = cur_err;

    // Mean error
    float i_err = err.sum() / err_max_sz;

    // Window size increase ("gain"). Dampen the negative bias here.
    float k = (i_err < 0) ? 0.0005*i_err : i_err;
    // Protect against wild increases. Might be adjusted for more
    // aggressive behavior. (don't increase window by unreasonable
    // amounts because it's hard to recover from).
    k = min(2*(bw_est*tau-inflight), k);

    // Perhaps limit how much window size can be reduced in one step.
    // Seems to make the system unstable (large + errors need correcting).
    //k = max(k, -2*(bw_est*100-inflight));
    //
    int window = the_window_size + k;

    // Increase window size if steady state reached.
    valarray<float> err_diff = err - err.shift(1);
    err_diff = abs(err_diff.shift(-1));

    //cerr << "err diff " << err_diff.sum() / err_max_sz<< endl;
    // Average movement in window, i.e. what constitutes a steady-state,
    // has high impact on risk-taking behavior.
    if (err_diff.sum() / err_max_sz  < 2) {
      // The longer we stay in steady-state across acks, the more riskily
      // we want to behave.
      cumul += 0.01;
      window += 8.0 + cumul;

      bw_quant += 0.03;
      bw_quant = min(bw_quant, (float)0.8);
    } else { // Decrease "steady-state counter" every time there's movement.
      cumul *= 0.9;
    }
    
    // Here if the conservative integrator sends us to 0, we want to fight it
    // so we set the window to a number > 0. Impacts risk-taking behavior.
    the_window_size = (window < 1) ? 3 : window;
    // If window is in Small Land despite our best efforts, make the bw estimate
    // conservative too.
    if (window <= 3) {
      bw_quant = 0.6;
    }
  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
         << " | bw: " << bw[0]
         << " | recent_acks[0]: " << recent_acks[0]
	 << endl;
      cerr << "At time " << timestamp_ms()
           << " window size is " << the_window_size
           << " sum_err is " << err.sum() / err_max_sz
           << " err_diff " << err[0] - err[2]
           << " bdp_est " << (bw_est* tau)
           << " bw " << bw_est
           << " tau " << tau
           << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return timeout_delay; /* timeout of one second */
}
