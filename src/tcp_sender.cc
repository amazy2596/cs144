#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
#include "wrapping_integers.hh"
#include <iostream>
#include <numeric>

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // debug( "unimplemented sequence_numbers_in_flight() called" );
  return std::accumulate(
    outstandings_segments_.begin(),
    outstandings_segments_.end(),
    0ULL,
    [&]( uint64_t sum, const absTCPSenderMessage& seg ) { return sum + seg.msg_.sequence_length(); } );
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  // debug( "unimplemented consecutive_retransmissions() called" );
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // debug( "unimplemented push() called" );
  if ( need_fast_retransmit_ ) {
    transmit( outstandings_segments_.front().msg_ );
    need_fast_retransmit_ = false;
  }

  uint64_t flow_win = rwnd_ == 0 ? 1ULL : rwnd_;
  uint64_t limit_win = flow_win;

  uint64_t avail_win
    = ( limit_win > sequence_numbers_in_flight() ) ? limit_win - sequence_numbers_in_flight() : 0ULL;
  while ( avail_win > 0 ) {
    avail_win = ( limit_win > sequence_numbers_in_flight() ) ? limit_win - sequence_numbers_in_flight() : 0ULL;

    TCPSenderMessage msg;

    if ( !SYN_sent_ ) {
      msg.SYN = true;
      SYN_sent_ = true;
    }

    msg.RST = input_.has_error();

    msg.seqno = Wrap32::wrap( next_seqno_, isn_ );

    uint64_t max_payload_size = min( (uint64_t)TCPConfig::MAX_PAYLOAD_SIZE, avail_win - msg.SYN );

    read( reader(), max_payload_size, msg.payload );

    if ( reader().is_finished() && !FIN_sent_ && avail_win - msg.SYN - msg.payload.size() > 0 ) {
      msg.FIN = true;
      FIN_sent_ = true;
    }

    if ( msg.sequence_length() == 0 ) {
      break;
    }

    if ( !timer_.is_active() ) {
      timer_.start( current_RTO_ms_ );
    }

    const uint64_t msg_len = msg.sequence_length();
    transmit( msg );
    outstandings_segments_.push_back( { next_seqno_, std::move( msg ) } );
    next_seqno_ += msg_len;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // debug( "unimplemented make_empty_message() called" );
  return { Wrap32::wrap( next_seqno_, isn_ ), false, "", false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    this->input_.set_error();
    this->timer_.stop();
    this->outstandings_segments_.clear();
    return;
  }

  rwnd_ = msg.window_size;

  if ( !msg.ackno.has_value() ) {
    return;
  }

  uint64_t abs_ackno = msg.ackno.value().unwrap( isn_, max_acked_seqno_ );

  if ( abs_ackno > next_seqno_ || abs_ackno <= max_acked_seqno_ ) {
    return;
  }

  if ( abs_ackno == last_ack_seqno_ ) {
    duplicate_ack_count_++;

    if ( duplicate_ack_count_ == 3 ) {
      if ( !outstandings_segments_.empty() ) {
        need_fast_retransmit_ = true;

        ssthresh_ = max( cwnd_ / 2, (uint64_t)2ULL * TCPConfig::MAX_PAYLOAD_SIZE );
        cwnd_ = ssthresh_ + 3ULL * TCPConfig::MAX_PAYLOAD_SIZE;
        in_fast_recovery_ = true;
      }
    } else if ( in_fast_recovery_ && duplicate_ack_count_ > 3 ) {
      cwnd_ += TCPConfig::MAX_PAYLOAD_SIZE;
    }
  } else {
    last_ack_seqno_ = abs_ackno;
    duplicate_ack_count_ = 0;
  }

  max_acked_seqno_ = abs_ackno;
  bool update = false;
  while ( !outstandings_segments_.empty() ) {
    auto& front = outstandings_segments_.front();
    if ( front.abs_seqno_ + front.msg_.sequence_length() <= abs_ackno ) {
      outstandings_segments_.pop_front();
      update = true;
    } else {
      break;
    }
  }

  if ( update ) {
    current_RTO_ms_ = initial_RTO_ms_;
    consecutive_retransmissions_ = 0;

    if ( in_fast_recovery_ ) {
      cwnd_ = ssthresh_;
      in_fast_recovery_ = false;
    } else {
      if ( cwnd_ < ssthresh_ ) {
        cwnd_ += TCPConfig::MAX_PAYLOAD_SIZE;
      } else {
        ack_cnt_ += TCPConfig::MAX_PAYLOAD_SIZE;
        if ( ack_cnt_ >= cwnd_ ) {
          cwnd_ += TCPConfig::MAX_PAYLOAD_SIZE;
          ack_cnt_ = 0;
        }
      }
    }

    duplicate_ack_count_ = 0;

    if ( outstandings_segments_.empty() ) {
      timer_.stop();
    } else {
      timer_.start( current_RTO_ms_ );
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // debug( "unimplemented tick({}, ...) called", ms_since_last_tick );
  if ( this->input_.has_error() ) {
    return;
  }

  timer_.tick( ms_since_last_tick );
  if ( timer_.is_expired() && !outstandings_segments_.empty() ) {
    transmit( outstandings_segments_.front().msg_ );

    ssthresh_ = max( cwnd_ / 2, (uint64_t)2ULL * TCPConfig::MAX_PAYLOAD_SIZE );
    cwnd_ = TCPConfig::MAX_PAYLOAD_SIZE;
    ack_cnt_ = 0;

    if ( rwnd_ != 0 ) {
      consecutive_retransmissions_++;
      current_RTO_ms_ <<= 1;
    }

    timer_.start( current_RTO_ms_ );
  }
}

bool Timer::is_active() const
{
  return this->active_;
}

bool Timer::is_expired() const
{
  return active_ && ( this->elapsed_ms_ >= this->rto_ );
}

void Timer::start( uint64_t rto )
{
  this->active_ = true;
  this->elapsed_ms_ = 0;
  this->rto_ = rto;
}

void Timer::stop()
{
  this->active_ = false;
}

void Timer::tick( uint64_t ms_since_last_tick )
{
  if ( !active_ ) {
    return;
  }
  this->elapsed_ms_ += ms_since_last_tick;
}
