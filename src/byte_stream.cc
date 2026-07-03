#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , error_( false )
  , bytes_()
  , is_closed_( false )
  , head_offset_( 0 )
  , bytes_popped_( 0 )
  , bytes_pushed_( 0 )
{}

void Writer::push( string data )
{
  if ( available_capacity() == 0 || data.empty() ) {
    return;
  }

  if ( data.size() > available_capacity() ) {
    data.resize( available_capacity() );
  }

  bytes_pushed_ += data.size();
  bytes_.push_back( std::move( data ) );
}

void Writer::close()
{
  is_closed_ = true;
}

bool Writer::is_closed() const
{
  return is_closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - ( bytes_pushed_ - bytes_popped_ );
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  if ( !bytes_.empty() ) {
    string_view res( bytes_.front() );
    return res.substr( head_offset_ );
  } else {
    string_view res;
    return res;
  }
}

void Reader::pop( uint64_t len )
{
  while ( len > 0 && !bytes_.empty() ) {
    uint64_t bytes_remain = bytes_.front().size() - head_offset_;
    if ( bytes_.front().size() - head_offset_ <= len ) {
      bytes_popped_ += bytes_remain;
      len -= bytes_remain;
      head_offset_ = 0;
      bytes_.pop_front();
    } else if ( bytes_remain > len ) {
      bytes_popped_ += len;
      head_offset_ += len;
      len = 0;
    }
  }
}

bool Reader::is_finished() const
{
  return ( bytes_pushed_ == bytes_popped_ && is_closed_ );
}

uint64_t Reader::bytes_buffered() const
{
  return bytes_pushed_ - bytes_popped_;
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
