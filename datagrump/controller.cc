#include <iostream>
#include <cstdlib>
#include <queue>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

int inflight = 0;

uint64_t sent_times[200000];

uint64_t last_send = 0;

uint64_t latest_delay = 80;
uint64_t latest_delay_send = 0;

int slow_packets = 0;
int all_packets = 0;


/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), the_window_size(1)
{
}

int countdown = -1;

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  // if (latest_delay > 100 || inflight > 80) {
  //   return (rand() % (latest_delay * 1000)) == 0;
  // }

  if ( (float)slow_packets/(float)all_packets > 0.04 ) { // Optimize for low latency.
    if (timestamp_ms() - last_send > 40) {
      last_send = timestamp_ms();
      return 1;
    }
    return 0;

  } else { // Fixed-window.
    if (inflight > 40) {
      return 0;
    } else {
      return 1;
    }
  }

  return 0;
  // int64_t r = rand();
  // int64_t m = latest_delay * inflight * 10 + 1;
  // return (r % m) == 0;

  // return 1;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

  inflight++;
  sent_times[sequence_number] = send_timestamp;
  last_send = send_timestamp;

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
}

int fails = 0;

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

  inflight--;

  all_packets++;
  uint64_t signal_delay = (timestamp_ack_received - send_timestamp_acked) + (sent_times[sequence_number_acked] - sent_times[sequence_number_acked - 1]);
  if (signal_delay > 150) {
    slow_packets++;
  }

  if (send_timestamp_acked > latest_delay_send) {
    latest_delay = signal_delay;
    latest_delay_send = send_timestamp_acked;
  }


}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
