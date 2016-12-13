#include <memory>

#include "fastrtps/transport/UDPv4TransportDescriptor.h"
#include "fastrtps/transport/UDPv6TransportDescriptor.h"

int main(void)
{
  using eprosima::fastrtps::rtps::UDPv4TransportDescriptor;
  auto udpv4_transport_desc = std::make_shared<UDPv4TransportDescriptor>();
  udpv4_transport_desc->TTL = 0;  // may fail to compile

  using eprosima::fastrtps::rtps::UDPv6TransportDescriptor;
  auto udpv6_transport_desc = std::make_shared<UDPv6TransportDescriptor>();
  udpv6_transport_desc->TTL = 0;  // may fail to compile
  return 0;
}
