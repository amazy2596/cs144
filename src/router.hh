#pragma once

#include "exception.hh"
#include "network_interface.hh"

#include <optional>

class Trie
{
public:
  struct RouteInfo
  {
    std::optional<Address> next_hop_ {};
    size_t interface_num_ { 0 };
  };

  struct Node
  {
    std::unique_ptr<Node> left { nullptr };
    std::unique_ptr<Node> right { nullptr };
    std::optional<RouteInfo> route_info_ {};
  };

  Trie() : root_( std::make_unique<Node>() ) {}

  void insert( uint32_t route_prefix, uint8_t prefix_length, const RouteInfo& route_info )
  {
    insert_helper( root_, route_prefix, prefix_length, 0, route_info );
  }

  bool remove( uint32_t route_prefix, uint8_t prefix_length )
  {
    return remove_helper( root_, route_prefix, prefix_length, 0 );
  }

  std::optional<RouteInfo> find( const uint32_t ip )
  {
    std::optional<RouteInfo> res = std::nullopt;
    return find_helper( root_, ip, 0, res );
  }

  std::unique_ptr<Node> root_ {};

private:
  void insert_helper( std::unique_ptr<Node>& node,
                      uint32_t route_prefix,
                      uint8_t prefix_length,
                      uint8_t depth,
                      const RouteInfo& route_info );

  bool remove_helper( std::unique_ptr<Node>& node, uint32_t route_prefix, uint8_t prefix_length, uint8_t depth );

  std::optional<RouteInfo> find_helper( std::unique_ptr<Node>& node,
                                        const uint32_t ip,
                                        uint8_t depth,
                                        std::optional<RouteInfo>& res );
};

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    interfaces_.push_back( notnull( "add_interface", std::move( interface ) ) );
    return interfaces_.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return interfaces_.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> interfaces_ {};

  Trie trie_ {};
};
