#pragma once

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <functional>

class Timer
{
public:
  Timer() : active_( false ), elapsed_ms_( 0 ), rto_( 0 ) {}

  bool is_active() const;

  bool is_expired() const;

  void start( uint64_t rto );

  void stop();

  void tick( uint64_t ms_since_last_tick );

private:
  bool active_;
  uint64_t elapsed_ms_;
  uint64_t rto_;
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , consecutive_retransmissions_( 0 )
    , current_RTO_ms_( initial_RTO_ms )
    , outstandings_segments_()
    , timer_()
    , SYN_sent_( false )
    , FIN_sent_( false )
    , rwnd_( 1 )
    , cwnd_( TCPConfig::MAX_PAYLOAD_SIZE )
    , ssthresh_( UINT64_MAX )
    , ack_cnt_( 0 )
    , last_ack_seqno_( 0 )
    , duplicate_ack_count_( 0 )
    , need_fast_retransmit_( false )
    , in_fast_recovery_( false )
    , next_seqno_( 0 )
    , max_acked_seqno_( 0 )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }

  struct absTCPSenderMessage
  {
    uint64_t abs_seqno_;
    TCPSenderMessage msg_;
  };

  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t consecutive_retransmissions_;
  uint64_t current_RTO_ms_;

  std::deque<absTCPSenderMessage> outstandings_segments_;
  Timer timer_;

  bool SYN_sent_;
  bool FIN_sent_;

  uint64_t rwnd_;
  uint64_t cwnd_;
  uint64_t ssthresh_;
  uint64_t ack_cnt_;

  uint64_t last_ack_seqno_;
  uint64_t duplicate_ack_count_;
  bool need_fast_retransmit_;
  bool in_fast_recovery_;

  uint64_t next_seqno_;
  uint64_t max_acked_seqno_;
};
