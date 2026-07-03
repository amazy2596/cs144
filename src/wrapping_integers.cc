#include "wrapping_integers.hh"
#include "debug.hh"
#include <algorithm>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // debug( "unimplemented wrap( {}, {} ) called", n, zero_point.raw_value_ );
  return Wrap32( zero_point + n );
}

uint64_t sub( uint64_t a, uint64_t b )
{
  return a > b ? ( a - b ) : ( b - a );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // debug( "unimplemented unwrap( {}, {} ) called", zero_point.raw_value_, checkpoint );
  uint32_t offset = ( raw_value_ - zero_point.raw_value_ );

  if ( checkpoint < offset ) {
    return offset;
  }

  int k = ( checkpoint - offset ) / ( 1ULL << 32 );

  uint64_t abs_seq1 = offset + k * ( 1ULL << 32 );
  uint64_t abs_seq2 = offset + ( k + 1 ) * ( 1ULL << 32 );

  if ( sub( checkpoint, abs_seq1 ) > sub( checkpoint, abs_seq2 ) ) {
    return abs_seq2;
  } else {
    return abs_seq1;
  }
}
