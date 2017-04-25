#include <iostream>
#include <list>
#include <numeric>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( true || debug ), the_window_size(20), inflight(0), tau(45),
  bw(0.5), err(), err_max_sz(50), sum_err(0)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  float cur_err = bw*tau - inflight;
  err.push_front(cur_err);
  if (err.size() > err_max_sz) err.pop_back();

  sum_err = accumulate(err.begin(), err.end(), sum_err);

  the_window_size += (1e-4 * (err.front() - err.back())
      // + 0.0001 * (bw.front() - bw.back()) adjust to derivative of bw
                    + 1e-5 * sum_err);
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size
         << " sum_err is " << sum_err
         << " err_diff " << err.front() - err.back()
         << " bdp_est " << bw * tau
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
  /* Default: take no action */
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
  /* Default: take no action */
  inflight--;
 // tau = (timestamp_ack_received - send_timestamp_acked) / 2;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
         << " | window size: " << the_window_size
	 << endl;
  }
//  the_window_size = (timestamp_ack_received - send_timestamp_acked >= 500) ? 1 : 20;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
