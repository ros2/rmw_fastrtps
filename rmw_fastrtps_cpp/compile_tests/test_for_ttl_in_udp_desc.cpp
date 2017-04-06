// Copyright 2016 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
