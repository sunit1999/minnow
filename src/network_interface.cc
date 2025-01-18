#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

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
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  (void)dgram;
  (void)next_hop;

  EthernetFrame frame = EthernetFrame();
  EthernetHeader header = EthernetHeader();

  // dst's MAC address found in Cache
  if (cache_.contains(next_hop.ipv4_numeric()))
  {
    header.type = EthernetHeader::TYPE_IPv4;
    header.src = ethernet_address_;
    header.dst = cache_[next_hop.ipv4_numeric()].first;

    frame.header = header;
    frame.payload = serialize(dgram);

    transmit(frame);
    return;
  }
  
  // else, send ARP request to find dst's MAC address
  header.type = EthernetHeader::TYPE_ARP;
  header.src = ethernet_address_;
  header.dst = ETHERNET_BROADCAST;

  ARPMessage arp_req = ARPMessage();
  arp_req.opcode = ARPMessage::OPCODE_REQUEST;
  arp_req.sender_ip_address = ip_address_.ipv4_numeric();
  arp_req.sender_ethernet_address = ethernet_address_;
  arp_req.target_ip_address = next_hop.ipv4_numeric();

  frame.header = header;
  frame.payload = serialize(arp_req);

  // Save datagram until its dst MAC is found out
  outbound_datagrams_[next_hop.ipv4_numeric()].push_back(dgram);

  // Only transmit if last request was sent more than 5 seconds ago
  if (
    !outbound_arp_requests_.contains(next_hop.ipv4_numeric()) or
    uptime_ms_ - outbound_arp_requests_[next_hop.ipv4_numeric()] > MAX_RETX_WAITING_TIME
  )
  {
    transmit(frame);
    outbound_arp_requests_[next_hop.ipv4_numeric()] = uptime_ms_;
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  (void)frame;

  if (frame.header.dst != ethernet_address_ and frame.header.dst != ETHERNET_BROADCAST)
  {
    cerr << "DEBUG: Destination MAC address -> " << to_string(frame.header.dst) << " invalid" << endl;
    return;
  }

  if (frame.header.type == EthernetHeader::TYPE_IPv4)
  {
    InternetDatagram dgram;
    if (!parse(dgram, frame.payload)) {
      cerr << "DEBUG: Failed to parse IP Datagram" << endl;
      return;
    }

    datagrams_received_.push(dgram);
    return;
  }
  
  ARPMessage msg {};
  if (!parse(msg, frame.payload)) {
    cerr << "DEBUG: Failed to parse ARP Message" << endl;
    return;
  }

  // Save IP -> MAC
  cache_[msg.sender_ip_address] = { msg.sender_ethernet_address, uptime_ms_ };

  // Only respond, if MAC corresponding to our IP is requested
  if (msg.opcode == ARPMessage::OPCODE_REQUEST and msg.target_ip_address == ip_address_.ipv4_numeric())
  {
    EthernetFrame res_frame = EthernetFrame();
    EthernetHeader res_header = EthernetHeader();
    ARPMessage arp_reply = ARPMessage();

    arp_reply.opcode = ARPMessage::OPCODE_REPLY;
    arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
    arp_reply.sender_ethernet_address = ethernet_address_;
    arp_reply.target_ip_address = msg.sender_ip_address;
    arp_reply.target_ethernet_address = msg.sender_ethernet_address;

    res_header.type = EthernetHeader::TYPE_ARP;
    res_header.src = ethernet_address_;
    res_header.dst = msg.sender_ethernet_address;

    res_frame.header = res_header;
    res_frame.payload = serialize(arp_reply);

    transmit(res_frame);
    return;
  }

  if (msg.opcode == ARPMessage::OPCODE_REPLY) {
    if (outbound_datagrams_.contains(msg.sender_ip_address)) {
      for (InternetDatagram& dgram: outbound_datagrams_[msg.sender_ip_address]) {
        send_datagram(dgram, Address::from_ipv4_numeric(msg.sender_ip_address));
      }

      // Remove outstanding requests
      outbound_datagrams_.erase(msg.sender_ip_address);
      outbound_arp_requests_.erase(msg.sender_ip_address);
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  (void)ms_since_last_tick;

  uptime_ms_ += ms_since_last_tick;

  // Evict cache entries after 30s
  for (auto it = cache_.begin(); it != cache_.end(); ) {
      if (uptime_ms_ - it->second.second > MAX_CACHE_TIME) {
          it = cache_.erase(it); // Correct way to erase during iteration
      } else {
          ++it;
      }
  }
}
