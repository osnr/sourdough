#include <iostream>
#include <cstdlib>
#include <queue>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

int inflight = 0;

uint64_t latest_receive = 0;
uint64_t receive_spacing = 0;

uint64_t last_send = 0;

uint64_t last_delay = 0;


/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug ), the_window_size(1)
{
}


/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  // If there's any unusual delay, we've overshot.
  // But we don't want to sawtooth down to 0.... fixme?
  // if (last_delay > 100 && inflight > 0) {
  //   return 0;
  // }

  if (timestamp_ms() - last_send > receive_spacing) {
    last_send = timestamp_ms();
    return 1;
  }

  // // FIXME: Probe every so often.
  // probestate = (probestate + 1) % 1000000;
  // if (probestate == 0) {
  //   receive_spacing = 0;
  // }

  return 0;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

  inflight++;
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

  if (timestamp_ack_received < latest_receive) return;

  receive_spacing = timestamp_ack_received - latest_receive;
  latest_receive = timestamp_ack_received;

  last_delay = (timestamp_ack_received - send_timestamp_acked);
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
