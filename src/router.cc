#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;
using Node = Trie::Node;
using RouteInfo = Trie::RouteInfo;

void Trie::insert_helper( unique_ptr<Node>& node,
                          uint32_t route_prefix,
                          uint8_t prefix_length,
                          uint8_t depth,
                          const RouteInfo& route_info )
{
  if ( depth == prefix_length ) {
    node->route_info_ = route_info;
    return;
  }

  int bit = ( route_prefix >> ( 31 - depth ) ) & 1;
  auto& next_node = ( bit == 0 ) ? node->left : node->right;
  if ( next_node == nullptr ) {
    next_node = make_unique<Node>();
  };
  insert_helper( next_node, route_prefix, prefix_length, depth + 1, route_info );
}

bool Trie::remove_helper( unique_ptr<Node>& node, uint32_t route_prefix, uint8_t prefix_length, uint8_t depth )
{
  if ( node == nullptr ) {
    return true;
  }

  if ( depth == prefix_length ) {
    node->route_info_ = nullopt;
  } else {
    int bit = ( route_prefix >> ( 31 - depth ) ) & 1;
    auto& next_node = ( bit == 0 ) ? node->left : node->right;
    if ( remove_helper( next_node, route_prefix, prefix_length, depth + 1 ) ) {
      next_node = nullptr;
    }
  }

  return ( node->left == nullptr ) && ( node->right == nullptr ) && ( !node->route_info_.has_value() );
}

optional<RouteInfo> Trie::find_helper( unique_ptr<Node>& node,
                                       const uint32_t ip,
                                       uint8_t depth,
                                       optional<RouteInfo>& res )
{
  if ( node == nullptr ) {
    return res;
  }

  if ( node->route_info_.has_value() ) {
    res = node->route_info_;
  }

  if ( depth == 32 ) {
    return res;
  }

  int bit = ( ip >> ( 31 - depth ) ) & 1;
  auto& next_node = ( bit == 0 ) ? node->left : node->right;
  return find_helper( next_node, ip, depth + 1, res );
}

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  // cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
  //  << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
  //  << " on interface " << interface_num << "\n";

  // debug( "unimplemented add_route() called" );

  this->trie_.insert( route_prefix, prefix_length, RouteInfo { next_hop, interface_num } );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // debug( "unimplemented route() called" );

  for ( auto& current_interface : this->interfaces_ ) {
    while ( !current_interface->datagrams_received().empty() ) {
      auto dgram = move( current_interface->datagrams_received().front() );
      current_interface->datagrams_received().pop();

      if ( dgram.header.ttl <= 1 ) {
        continue;
      }
      dgram.header.ttl--;
      dgram.header.compute_checksum();

      auto res = this->trie_.find( dgram.header.dst );
      if ( res.has_value() ) {
        Address next_hop = ( res->next_hop_.has_value() ) ? res->next_hop_.value()
                                                          : Address::from_ipv4_numeric( dgram.header.dst );

        interface( res->interface_num_ )->send_datagram( dgram, next_hop );
      }
    }
  }
}
