#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );

  first_unacceptable_index_ = first_unassembled_index_ + output_.writer().available_capacity();

  if ( is_last_substring ) {
    eof_index_ = first_index + data.size();
  }

  if ( first_unassembled_index_ == eof_index_ ) {
    output_.writer().close();
    eof_index_ = -1;
    return;
  }

  if ( data.empty() ) {
    return;
  }

  uint64_t last_index = first_index + data.size() - 1;
  if ( last_index < first_unassembled_index_ || first_index >= first_unacceptable_index_ ) {
    return;
  }

  uint64_t start_idx = 0;
  if ( first_unassembled_index_ > first_index ) {
    start_idx += first_unassembled_index_ - first_index;
    first_index += first_unassembled_index_ - first_index;
  }

  uint64_t offset = first_index - first_unassembled_index_;
  uint64_t physical_idx = head_index_ + offset;
  data = data.substr( start_idx, first_unacceptable_index_ - first_index );

  for ( uint64_t i = 0; i < data.size(); i++ ) {
    buffer_[( physical_idx + i ) % capacity_] = data[i];
    present_[( physical_idx + i ) % capacity_] = true;
  }

  string buf;
  while ( present_[head_index_ % capacity_] ) {
    buf += buffer_[head_index_ % capacity_];
    present_[head_index_ % capacity_] = false;
    head_index_ = ( head_index_ + 1 ) % capacity_;
    first_unassembled_index_++;
  }

  output_.writer().push( std::move( buf ) );

  if ( first_unassembled_index_ == eof_index_ ) {
    output_.writer().close();
    eof_index_ = -1;
    return;
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  debug( "unimplemented count_bytes_pending() called" );
  return count( present_.begin(), present_.end(), true );
}
