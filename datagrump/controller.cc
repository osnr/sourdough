#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

uint64_t slow_packets = 0;
uint64_t all_packets = 0;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), the_window_size(40)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  return the_window_size;
}


uint64_t sent_time[200000];

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */
  sent_time[sequence_number] = send_timestamp;

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

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }

  uint64_t signal_delay;
  if (sequence_number_acked == 0) {
    signal_delay = 0;
  } else {
    signal_delay = sent_time[sequence_number_acked] - sent_time[sequence_number_acked - 1];
  }
  signal_delay += timestamp_ack_received - send_timestamp_acked;

  all_packets++;
  if (signal_delay >= 100) {
    slow_packets++;
  }

  if ((float)slow_packets / (float)all_packets > 0.04) {
    // We're in an intolerable state.
    the_window_size = 1;

  } else {
    the_window_size = 1;
  }

  // all_packets++;
  // if (timestamp_ack_received - send_timestamp_acked >= 100) {
  //   slow_packets++;
  //   the_window_size = 10;

  // } else {
  //   the_window_size = 40;
  // }

  // if ((float)slow_packets / (float)all_packets > 0.04) {
  //   // We are over our slow budget.
  //   the_window_size = 1;

  //   // // Maybe it's fast enough to probe, though...
  //   // if (timestamp_ack_received - send_timestamp_acked < 100) {
  //   //   the_window_size = ;
  //   // }
  // }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
