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

#include <algorithm>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/topic_endpoint_info_array.h"
#include "rmw/topic_endpoint_info.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

#include "rmw_dds_common/graph_cache.hpp"

#include "demangle.hpp"

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t
_validate_params(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  rmw_topic_endpoint_info_array_t * participants_info)
{
  if (!identifier) {
    RMW_SET_ERROR_MSG("null implementation identifier provided");
    return RMW_RET_ERROR;
  }

  if (!topic_name) {
    RMW_SET_ERROR_MSG("null topic_name provided");
    return RMW_RET_ERROR;
  }

  if (!allocator) {
    RMW_SET_ERROR_MSG("null allocator provided");
    return RMW_RET_ERROR;
  }

  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }

  // Get participant pointer from node
  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!participants_info) {
    RMW_SET_ERROR_MSG("null participants_info provided");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}


rmw_ret_t
__rmw_get_publishers_info_by_topic(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * publishers_info)
{
  rmw_ret_t ret = _validate_params(
    identifier,
    node,
    allocator,
    topic_name,
    publishers_info);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  rmw_dds_common::GraphCache & graph_cache = common_context->graph_cache;
  std::string mangled_topic_name = topic_name;
  DemangleFunction demangle_type = _identity_demangle;
  if (!no_mangle) {
    mangled_topic_name = _mangle_topic_name(ros_topic_prefix, topic_name).to_string();
    demangle_type = _demangle_if_ros_type;
  }

  return graph_cache.get_writers_info_by_topic(
    mangled_topic_name,
    demangle_type,
    allocator,
    publishers_info);
}

rmw_ret_t
__rmw_get_subscriptions_info_by_topic(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_endpoint_info_array_t * subscriptions_info)
{
  rmw_ret_t ret = _validate_params(
    identifier,
    node,
    allocator,
    topic_name,
    subscriptions_info);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  rmw_dds_common::GraphCache & graph_cache = common_context->graph_cache;
  std::string mangled_topic_name = topic_name;
  DemangleFunction demangle_type = _identity_demangle;
  if (!no_mangle) {
    mangled_topic_name = _mangle_topic_name(ros_topic_prefix, topic_name).to_string();
    demangle_type = _demangle_if_ros_type;
  }

  return graph_cache.get_readers_info_by_topic(
    mangled_topic_name,
    demangle_type,
    allocator,
    subscriptions_info);
}
}  // namespace rmw_fastrtps_shared_cpp
