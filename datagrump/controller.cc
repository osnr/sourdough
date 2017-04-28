#include <iostream>
#include <list>
#include <algorithm>
#include <cmath>
#include <numeric>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), the_window_size(10), inflight(0),
  err(), err_max_sz(41), recent_acks(), acks_max_sz(15),
  head_ack_time(0), recent_tau(), tau_max_sz(91), timeout_delay(500),
  sitting(), sitting_max_sz(31), smallest_expected(1), sit_per_packet(1),
  win_incr_gain(0.5), win_decr_gain(2.0), max_win_sz(35), signal_delay_target(200),
  tau_qt(0.5), sit_qt(0.7)
{
  err.resize(err_max_sz, 0);
  recent_acks.resize(acks_max_sz, 1);
  recent_tau.resize(tau_max_sz, 35);
  sitting.resize(sitting_max_sz, 0);
}


/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
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
  inflight--;

  // Save the most recent propagation delay estimates.
  recent_tau = recent_tau.cshift(1);
  recent_tau[0] = (timestamp_ack_received - send_timestamp_acked) / 2;
  valarray<unsigned int> sorted_tau = recent_tau;
  sort(begin(sorted_tau), end(sorted_tau));

  // Estimate queuing delays.
  uint64_t sequence_diff = sequence_number_acked - smallest_expected;
  if (sequence_diff < sitting_max_sz) {
    sitting[sequence_diff] = send_timestamp_acked;
    if (sitting.min() != 0) {
      valarray<uint64_t> diff_sit = sitting - sitting.shift(-1);
      diff_sit = diff_sit.shift(1);
      valarray<uint64_t> sorted_diff_sit = diff_sit;
      sort(begin(sorted_diff_sit), end(sorted_diff_sit));
      sit_per_packet = sorted_diff_sit[sit_qt*sitting_max_sz+1];
      sit_per_packet = max(1.0, (double)sit_per_packet);
      smallest_expected = sequence_number_acked + 5;
      sitting = 0;
    }
  }

  // Adjust window
  float spare_time = signal_delay_target - 2*sorted_tau[tau_max_sz*tau_qt+1];
  float cur_err = max((double)spare_time, 0.0) / sit_per_packet - inflight;
  err = err.cshift(1);
  err[0] = cur_err;

  float del = (cur_err < 0) ? win_decr_gain * cur_err : win_incr_gain * cur_err;
  int window = the_window_size + del;

  window = max(1, window);
  the_window_size = min((int)max_win_sz, window);

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
      cerr << "At time " << timestamp_ms()
           << " window size is " << the_window_size
           << "err is " << err[0]
           << " sit_per " << sit_per_packet
           << " tau " << recent_tau[0]
           << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return timeout_delay; /* timeout of one second */
}
