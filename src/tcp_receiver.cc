#include "tcp_receiver.hh"
#include "debug.hh"
#include <algorithm>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // debug( "unimplemented receive() called" );

  if ( message.RST == true ) {
    reader().set_error();
    return;
  }

  if ( message.SYN == true ) {
    zero_point_ = message.seqno;
    is_start_ = true;
  }

  if ( !is_start_ ) {
    return;
  }

  uint64_t first_index = message.seqno.unwrap( zero_point_, writer().bytes_pushed() + 1 ) + message.SYN - 1;
  reassembler_.insert( first_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // debug( "unimplemented send() called" );
  std::optional<Wrap32> ackno = std::nullopt;
  if ( is_start_ ) {
    ackno = Wrap32::wrap( writer().bytes_pushed() + 1 + writer().is_closed(), zero_point_ );
  }

  uint16_t window_size
    = ( writer().available_capacity() > 65535 ) ? 65535 : static_cast<uint16_t>( writer().available_capacity() );

  bool RST = writer().has_error();

  return TCPReceiverMessage { ackno, window_size, RST };
}
