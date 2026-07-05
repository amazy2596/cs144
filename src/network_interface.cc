#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"
#include <vector>

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  // cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address
  // "
  //  << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // debug( "unimplemented send_datagram called" );
  // (void)dgram;
  // (void)next_hop;
  uint32_t dst = next_hop.ipv4_numeric();

  auto it = arp_cache_.find( dst );
  if ( it == arp_cache_.end() ) {
    datagrams_wait_ip_[dst].push( dgram );

    if ( arp_request_timers.find( dst ) == arp_request_timers.end() ) {
      arp_request_timers[dst] = 5000;
      ARPMessage arp_request;
      arp_request.opcode = ARPMessage::OPCODE_REQUEST;
      arp_request.sender_ethernet_address = this->ethernet_address_;
      arp_request.sender_ip_address = this->ip_address_.ipv4_numeric();
      arp_request.target_ip_address = dst;

      EthernetFrame frame;
      frame.header.src = this->ethernet_address_;
      frame.header.dst = ETHERNET_BROADCAST;
      frame.header.type = EthernetHeader::TYPE_ARP;
      frame.payload = serialize( arp_request );

      transmit( frame );
    }
  } else {
    arp_request_timers.erase( dst );

    EthernetFrame frame;
    frame.header.src = this->ethernet_address_;
    frame.header.dst = ( *it ).second.mac_;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.payload = serialize( dgram );
    transmit( frame );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  // debug( "unimplemented recv_frame called" );
  // (void)frame;
  if ( !( frame.header.dst == ETHERNET_BROADCAST || frame.header.dst == this->ethernet_address_ ) ) {
    return;
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram internet_datagram;
    if ( parse( internet_datagram, frame.payload ) ) {
      datagrams_received_.push( std::move( internet_datagram ) );
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_msg;

    if ( parse( arp_msg, frame.payload ) ) {
      arp_cache_[arp_msg.sender_ip_address] = ArpEntry { arp_msg.sender_ethernet_address, 30000 };
      arp_request_timers.erase( arp_msg.sender_ip_address );

      while ( !datagrams_wait_ip_[arp_msg.sender_ip_address].empty() ) {
        InternetDatagram dgram = std::move( datagrams_wait_ip_[arp_msg.sender_ip_address].front() );
        datagrams_wait_ip_[arp_msg.sender_ip_address].pop();

        EthernetFrame frame_send;
        frame_send.header.src = this->ethernet_address_;
        frame_send.header.dst = arp_msg.sender_ethernet_address;
        frame_send.header.type = EthernetHeader::TYPE_IPv4;
        frame_send.payload = serialize( dgram );
        transmit( frame_send );
      }

      if ( arp_msg.opcode == ARPMessage::OPCODE_REQUEST
           && arp_msg.target_ip_address == this->ip_address_.ipv4_numeric() ) {
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = this->ethernet_address_;
        arp_reply.sender_ip_address = this->ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = arp_msg.sender_ethernet_address;
        arp_reply.target_ip_address = arp_msg.sender_ip_address;

        EthernetFrame frame_reply;
        frame_reply.header.src = this->ethernet_address_;
        frame_reply.header.dst = arp_msg.sender_ethernet_address;
        frame_reply.header.type = EthernetHeader::TYPE_ARP;
        frame_reply.payload = serialize( arp_reply );

        transmit( frame_reply );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // debug( "unimplemented tick({}) called", ms_since_last_tick );
  std::erase_if( arp_cache_, [&]( auto& item ) {
    auto& [key, val] = item;
    val.ttl_ms_ -= ms_since_last_tick;

    return val.ttl_ms_ <= 0;
  } );

  std::erase_if( arp_request_timers, [&]( auto& item ) {
    auto& [key, val] = item;
    val -= ms_since_last_tick;
    if ( val <= 0 ) {
      while ( !datagrams_wait_ip_[key].empty() ) {
        datagrams_wait_ip_[key].pop();
      }
    }
    return val <= 0;
  } );
}
