#ifndef CONTROLLER_HH
#define CONTROLLER_HH

#include <cstdint>
#include <list>
#include <valarray>

/* Congestion controller interface */

class Controller
{
private:
  bool debug_; /* Enables debugging output */
  unsigned int the_window_size;

  unsigned int inflight;

  std::valarray<unsigned int> recent_acks;
  const unsigned int acks_max_sz;
  unsigned int head_ack_time;
  
  std::valarray<unsigned int> recent_tau;
  const unsigned int tau_max_sz;

  const unsigned int timeout_delay;

  std::valarray<uint64_t> sitting;
  unsigned int sitting_max_sz;
  uint64_t smallest_expected;
  float sit_per_packet;

  float win_incr_gain;
  float win_decr_gain;
  unsigned int max_win_sz;
  unsigned int signal_delay_target;
  float tau_qt; // tau quantile



public:
  /* Public interface for the congestion controller */
  /* You can change these if you prefer, but will need to change
     the call site as well (in sender.cc) */

  /* Default constructor */
  Controller( const bool debug );

  /* Get current window size, in datagrams */
  unsigned int window_size( void );

  /* A datagram was sent */
  void datagram_was_sent( const uint64_t sequence_number,
			  const uint64_t send_timestamp );

  /* An ack was received */
  void ack_received( const uint64_t sequence_number_acked,
		     const uint64_t send_timestamp_acked,
		     const uint64_t recv_timestamp_acked,
		     const uint64_t timestamp_ack_received );

  /* How long to wait (in milliseconds) if there are no acks
     before sending one more datagram */
  unsigned int timeout_ms( void );
};

#endif
