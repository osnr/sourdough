#include <iostream>
#include <fstream>
#include <map>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

unsigned int the_window_size = 1;
map<uint64_t, uint64_t> capacity_over_time;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
  std::ifstream infile("capacity_over_time.txt");
  int time, capacity;
  // int i = 0;
  while (!infile.eof()) {
    // cout << "time:" << time << ",capacity:" << capacity;
    // i++;
    // if (i > 100) return;
    infile >> time;
    infile >> capacity;
    capacity_over_time[time] = capacity;
  }
}

uint64_t sendtime = 0;
int sends_remaining = 0;
/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }

  // cout << "At time " << timestamp_ms() << " capacity is ";

  uint64_t time = timestamp_ms();
  if (sendtime != time) {
    sendtime = time;
    sends_remaining = capacity_over_time[time - 2];
  }
  sends_remaining--;

  return sends_remaining >= 0;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

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
  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
  // the_window_size = (timestamp_ack_received - send_timestamp_acked >= 200) ? 10 : 40;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
