// Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "rmw/get_topic_endpoint_info.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/topic_endpoint_info_array.h"
#include "rmw/types.h"
#include "rcutils/error_handling.h"
#include "rcutils/types/string_map.h"

#include "buffer_endpoint_registry.hpp"
#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/visibility_control.h"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

std::string format_backend_metadata(
  const std::unordered_map<std::string, std::string> & backend_metadata)
{
  std::vector<std::pair<std::string, std::string>> sorted(
    backend_metadata.begin(), backend_metadata.end());
  std::sort(sorted.begin(), sorted.end());

  std::string result;
  for (const auto & [backend, metadata] : sorted) {
    if (!result.empty()) {
      result += ", ";
    }
    result += backend;
    if (!metadata.empty()) {
      result += "(";
      result += metadata;
      result += ")";
    }
  }
  return result;
}

}  // namespace

extern "C"
{
rmw_ret_t
rmw_get_publishers_info_by_topic(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * publishers_info)
{
  return rmw_fastrtps_shared_cpp::__rmw_get_publishers_info_by_topic(
    eprosima_fastrtps_identifier, node, allocator, topic_name, no_mangle, publishers_info);
}

rmw_ret_t
rmw_get_subscriptions_info_by_topic(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * subscriptions_info)
{
  return rmw_fastrtps_shared_cpp::__rmw_get_subscriptions_info_by_topic(
    eprosima_fastrtps_identifier, node, allocator, topic_name, no_mangle, subscriptions_info);
}

RMW_FASTRTPS_CPP_PUBLIC
rmw_ret_t
rmw_fastrtps_cpp_get_buffer_backend_metadata_by_topic(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_endpoint_type_t endpoint_type,
  rcutils_string_map_t * backend_metadata_by_gid)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(node->context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(node->context->impl, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "allocator argument is invalid", return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_name, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(backend_metadata_by_gid, RMW_RET_INVALID_ARGUMENT);

  if (endpoint_type != RMW_ENDPOINT_PUBLISHER &&
    endpoint_type != RMW_ENDPOINT_SUBSCRIPTION)
  {
    RMW_SET_ERROR_MSG("endpoint_type must be publisher or subscription");
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto * registry = static_cast<rmw_fastrtps_cpp::BufferEndpointRegistry *>(
    node->context->impl->buffer_endpoint_registry);
  if (!registry) {
    return RMW_RET_OK;
  }

  const bool is_reader = endpoint_type == RMW_ENDPOINT_SUBSCRIPTION;
  const std::string lookup_topic =
    no_mangle ? _strip_ros_prefix_if_exists(topic_name) : topic_name;
  auto endpoints = registry->get_endpoints_by_topic(lookup_topic, is_reader);

  rcutils_ret_t rcutils_ret =
    rcutils_string_map_init(backend_metadata_by_gid, endpoints.size(), *allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    rcutils_reset_error();
    return RMW_RET_ERROR;
  }

  for (const auto & endpoint : endpoints) {
    const std::string gid = rmw_fastrtps_shared_cpp::gid_to_hex(endpoint.gid);
    const std::string metadata = format_backend_metadata(endpoint.backend_metadata);
    rcutils_ret = rcutils_string_map_set(
      backend_metadata_by_gid,
      gid.c_str(),
      metadata.c_str());
    if (rcutils_ret != RCUTILS_RET_OK) {
      RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
      rcutils_reset_error();
      return RMW_RET_ERROR;
    }
  }
  return RMW_RET_OK;
}
}  // extern "C"
