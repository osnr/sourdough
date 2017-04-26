#include <iostream>
#include <list>
#include <algorithm>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug || true ), the_window_size(40), inflight(0), tau(45),
  err(), err_max_sz(91), recent_acks(), acks_max_sz(10),
  head_ack_time(0), recent_tau(), tau_max_sz(41), timeout_delay(500), last_ack_proc(0),
  bw(), bw_max_sz(51)
{
  err.resize(err_max_sz, 0);
  recent_acks.resize(acks_max_sz, 1);
  recent_tau.resize(tau_max_sz, tau);
  bw.resize(bw_max_sz, 0.5);
}


/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  // If we haven't received any ack in a long time, there's likely a long
  // queue, so reduce window size.
  if (((int)last_ack_proc - (int)timestamp_ms()) > (0.5*timeout_delay)) {
    the_window_size = 1;
  } else {
    // Quantiled bw
    valarray<float>bw_sort = bw;
    sort(begin(bw_sort), end(bw_sort));
    float bw_est =  bw_sort[bw_max_sz*0.6];

    // Mean bw
    //float bw_est =  bw.sum() / bw_max_sz;

    // BDP - inflight 
    float cur_err = bw_est*tau - inflight;
    err = err.cshift(1);
    err[0] = cur_err;

    // Mean error
    float i_err = err.sum() / err_max_sz;

    float k = (i_err < 0) ? i_err : log1p(3*i_err);
    k = min((bw_est*tau-inflight), k);
    int window = the_window_size + k;

    // Increase window size if steady state reached.
//    if (abs(err[0] - err[err_max_sz-1]) < 1e-6) window++;

    // Saturate window (limit to bdp).
   // window = min((float)window, (bw_est*tau-inflight));
    
    the_window_size = (window < 1) ? 1 : window;

    if ( debug_ ) {
      cerr << "At time " << timestamp_ms()
           << " window size is " << the_window_size
           << " sum_err is " << err.sum() / err_max_sz
           << " err_diff " << err[0] - err[2]
           << " bdp_est " << (bw.sum() / bw_max_sz)* tau
           << " bw " << bw_est
           << " tau " << tau
           << endl;
    }
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

  // Save 
  recent_tau = recent_tau.cshift(1);
  recent_tau[0] = (timestamp_ack_received - send_timestamp_acked) / 2;
  valarray<unsigned int> sorted_tau = recent_tau;
  sort(begin(sorted_tau), end(sorted_tau));
  tau = min(sorted_tau[(tau_max_sz*0.8)+1], timeout_delay/2);

  // Only keep stats on bandwidth if RTT is reasonable. Otherwise,
  // this incoming packet is not representative of current bw.
  if (recent_tau[0] < timeout_delay/2) {
    unsigned int time_diff = timestamp_ack_received - head_ack_time;
    recent_acks = recent_acks.shift(time_diff);
    recent_acks[0]++;
    head_ack_time = timestamp_ack_received;
    //bw = (float)recent_acks.sum() / (float)acks_max_sz;
    bw = bw.cshift(1);
    bw[0] = (float)recent_acks.sum() / (float)acks_max_sz;
  }

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
         << " | bw: " << bw[0]
         << " | recent_acks[0]: " << recent_acks[0]
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return timeout_delay; /* timeout of one second */
}
