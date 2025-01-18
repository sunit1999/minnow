#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

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
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  _route_list.push_back(RouteEntry{route_prefix, prefix_length, next_hop, interface_num});
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for (const auto& interface: _interfaces)
  {
    auto& datagrams_received = interface->datagrams_received();
    while (!datagrams_received.empty())
    {
      route_datagram(datagrams_received.front());
      datagrams_received.pop();
    }
  }
}

void Router::route_datagram(InternetDatagram& dgram) {
  // Drop iff TTL is zero or becomes zero after decrement
  if (dgram.header.ttl <= 1) {
    cerr<<"TTL Expired\n";
    return;
  }

  uint32_t dest_ip = dgram.header.dst;
  RouteEntry best_route;
  bool match_found = false;

  // Iterate over each route, if route is LPM, update the best route
  for (auto& route: _route_list) {
      if (is_match(dest_ip, route.route_prefix, route.prefix_length)) {
          if (!match_found or best_route.prefix_length < route.prefix_length) {
              best_route = route;
              match_found = true;
          }
      }
  }

  // No match found -> drop datagram
  if (!match_found) {
    cerr<<"No match found\n";
    return;
  }

  // Decrement TTL
  dgram.header.ttl -= 1;
  dgram.header.compute_checksum();

  // Route packet to next hop or attached to attached interface (if next hop is missing)
  _interfaces[best_route.interface_num]->send_datagram(dgram, best_route.next_hop.value_or(Address::from_ipv4_numeric(dest_ip)));
}

bool Router::is_match(uint32_t ip1, uint32_t ip2, uint8_t len) {
  // Create a mask with the specified number of most significant bits set to 1
  uint32_t mask = (len == 0) ? 0 : 0xffffffff << (32 - len);

  // Perform a bitwise AND operation with the mask on both IP addresses
  return (ip1 & mask) == (ip2 & mask);
}
