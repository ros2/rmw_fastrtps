// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <string>

#include "rmw/get_network_flow_endpoints.h"
#include "rmw/error_handling.h"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "fastrtps/utils/IPLocator.h"


namespace rmw_fastrtps_shared_cpp
{

using Locator_t = eprosima::fastrtps::rtps::Locator_t;
using LocatorList_t = eprosima::fastrtps::rtps::LocatorList_t;
using IPLocator = eprosima::fastrtps::rtps::IPLocator;

rmw_ret_t fill_network_flow_endpoint(rmw_network_flow_endpoint_t *, const Locator_t &);

rmw_ret_t
__rmw_publisher_get_network_flow_endpoints(
  const rmw_publisher_t * publisher,
  rcutils_allocator_t * allocator,
  rmw_network_flow_endpoint_array_t * network_flow_endpoint_array)
{
  rmw_ret_t res = RMW_RET_OK;

  // Retrieve the sender locators
  CustomPublisherInfo * data =
    static_cast<CustomPublisherInfo *>(publisher->data);
  LocatorList_t locators;
  data->data_writer_->get_sending_locators(locators);

  if (locators.empty()) {
    return res;
  }

  // It must be a non-initialized array
  if (RMW_RET_OK !=
    (res = rmw_network_flow_endpoint_array_check_zero(network_flow_endpoint_array)))
  {
    return res;
  }

  // Allocate
  if (RMW_RET_OK !=
    (res = rmw_network_flow_endpoint_array_init(
      network_flow_endpoint_array,
      locators.size(),
      allocator)))
  {
    return res;
  }

  // Translate the locators, on error reset the array
  try {
    auto rmw_nf = network_flow_endpoint_array->network_flow_endpoint;
    for (const Locator_t & loc : locators) {
      if (RMW_RET_OK !=
        (res = fill_network_flow_endpoint(rmw_nf++, loc)))
      {
        throw res;
      }
    }
  } catch (rmw_ret_t) {
    // clear the array
    rmw_network_flow_endpoint_array_fini(
      network_flow_endpoint_array);

    // set error message
    RMW_SET_ERROR_MSG("Failed to compose network_flow_endpoint_array");
  }

  return res;
}

rmw_ret_t
__rmw_subscription_get_network_flow_endpoints(
  const rmw_subscription_t * subscription,
  rcutils_allocator_t * allocator,
  rmw_network_flow_endpoint_array_t * network_flow_endpoint_array)
{
  rmw_ret_t res = RMW_RET_OK;

  // Retrieve the listener locators
  CustomSubscriberInfo * data =
    static_cast<CustomSubscriberInfo *>(subscription->data);
  LocatorList_t locators;
  data->data_reader_->get_listening_locators(locators);

  if (locators.empty()) {
    return res;
  }

  // It must be a non-initialized array
  if (RMW_RET_OK !=
    (res = rmw_network_flow_endpoint_array_check_zero(network_flow_endpoint_array)))
  {
    return res;
  }

  // Allocate
  if (RMW_RET_OK !=
    (res = rmw_network_flow_endpoint_array_init(
      network_flow_endpoint_array,
      locators.size(),
      allocator)))
  {
    return res;
  }

  // Translate the locators, on error reset the array
  try {
    auto rmw_nf = network_flow_endpoint_array->network_flow_endpoint;
    for (const Locator_t & loc : locators) {
      if (RMW_RET_OK !=
        (res = fill_network_flow_endpoint(rmw_nf++, loc)))
      {
        throw res;
      }
    }
  } catch (rmw_ret_t) {
    // clear the array
    rmw_network_flow_endpoint_array_fini(
      network_flow_endpoint_array);

    // set error message
    RMW_SET_ERROR_MSG("Failed to compose network_flow_endpoint_array");
  }

  return res;
}

// Ancillary translation methods
rmw_transport_protocol_t
get_transport_protocol(const Locator_t & loc)
{
  if (loc.kind & (LOCATOR_KIND_UDPv4 | LOCATOR_KIND_UDPv6)) {
    return RMW_TRANSPORT_PROTOCOL_UDP;
  } else if (loc.kind & (LOCATOR_KIND_TCPv4 | LOCATOR_KIND_TCPv6)) {
    return RMW_TRANSPORT_PROTOCOL_TCP;
  }

  return RMW_TRANSPORT_PROTOCOL_UNKNOWN;
}

rmw_internet_protocol_t
get_internet_protocol(const Locator_t & loc)
{
  if (loc.kind & (LOCATOR_KIND_UDPv4 | LOCATOR_KIND_TCPv4)) {
    return RMW_INTERNET_PROTOCOL_IPV4;
  } else if (loc.kind & (LOCATOR_KIND_TCPv6 | LOCATOR_KIND_UDPv6)) {
    return RMW_INTERNET_PROTOCOL_IPV6;
  }

  return RMW_INTERNET_PROTOCOL_UNKNOWN;
}

rmw_ret_t
fill_network_flow_endpoint(
  rmw_network_flow_endpoint_t * network_flow_endpoint,
  const Locator_t & locator)
{
  rmw_ret_t res = RMW_RET_OK;

  // Translate transport protocol
  network_flow_endpoint->transport_protocol = get_transport_protocol(locator);

  // Translate internet protocol
  network_flow_endpoint->internet_protocol = get_internet_protocol(locator);

  // Set the port
  network_flow_endpoint->transport_port = IPLocator::getPhysicalPort(locator);

  // Set the address
  std::string address = IPLocator::ip_to_string(locator);

  if (RMW_RET_OK !=
    (res = rmw_network_flow_endpoint_set_internet_address(
      network_flow_endpoint,
      address.c_str(),
      address.length())))
  {
    return res;
  }

  return res;
}

}  // namespace rmw_fastrtps_shared_cpp
