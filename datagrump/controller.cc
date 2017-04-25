#include <iostream>
#include <list>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug||true ), the_window_size(20), inflight(0), tau(45),
  bw(0.5), err(), err_max_sz(50), recent_acks()
{
  err.resize(err_max_sz);
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  float cur_err = bw*tau - inflight;
  err.cshift(1);
  err[0] = cur_err;

  int window = the_window_size + 1e-2 * err[0] + (2e-1 * err.sum()) / err_max_sz;
  the_window_size = (window < 1) ? 1 : window;
      // + 0.0001 * (bw.front() - bw.back()) adjust to derivative of bw

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size
         << " sum_err is " << err.sum()
         << " err_diff " << err[0] - err[err_max_sz]
         << " bdp_est " << bw * tau
         << " err list sz " << err.size()
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

  tau = (timestamp_ack_received - send_timestamp_acked) / 2;

  // recent_acks.push_front(timestamp_ack_received);

  // uint64_t time = timestamp_ms();

  // int packets_seen_10_ms = 0;

  // list<uint64_t>::iterator it = recent_acks.begin();
  // while (it != recent_acks.end()) {
  //   int seen_ms_ago = time - *it;
  //   if (seen_ms_ago > 10) { // If older than 10 ms
  //     it = recent_acks.erase(it);
  //     continue;
  //   }

  //   packets_seen_10_ms++;
  // }

  // bw = packets_seen_10_ms / 10.0;

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
