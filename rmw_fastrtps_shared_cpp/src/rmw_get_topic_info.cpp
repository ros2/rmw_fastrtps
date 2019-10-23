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
#include "rmw/topic_info_array.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t
_validate_params(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  rmw_topic_info_array_t * participants_info)
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

std::vector<std::string>
_get_topic_fqdns(const char * topic_name, bool no_mangle)
{
  std::vector<std::string> topic_fqdns;
  topic_fqdns.push_back(topic_name);
  // if mangle
  if (!no_mangle) {
    auto ros_prefixes = _get_all_ros_prefixes();
    // Build the list of all possible topic FQDN
    std::for_each(ros_prefixes.begin(), ros_prefixes.end(),
      [&topic_fqdns, &topic_name](const std::string & prefix) {
        topic_fqdns.push_back(prefix + topic_name);
      });
  }
  return topic_fqdns;
}

rmw_ret_t
_set_rmw_topic_info(
  rcutils_allocator_t * allocator,
  std::tuple<GUID_t, std::string, rmw_qos_profile_t> data,
  ::ParticipantListener * slave_target,
  rmw_topic_info_t * topic_info)
{
  const auto & gid = std::get<0>(data);
  // convert gid to const char * and set it inside topic_info
  std::ostringstream gid_stream;
  gid_stream << gid;
  rmw_ret_t ret = rmw_topic_info_set_gid(allocator, gid_stream.str().c_str(), topic_info);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  // set topic type
  ret = rmw_topic_info_set_topic_type(allocator, std::get<1>(data).c_str(), topic_info);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  // set qos profile
  ret = rmw_topic_info_set_qos_profile(&std::get<2>(data), topic_info);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  // set node name
  const auto & d_name_it = slave_target->discovered_names.find(gid);
  if (d_name_it != slave_target->discovered_names.end()) {
    ret = rmw_topic_info_set_node_name(allocator, d_name_it->second.c_str(), topic_info);
  } else {
    ret = rmw_topic_info_set_node_name(allocator, "_NODE_NAME_UNKNOWN_", topic_info);
  }
  if (ret != RMW_RET_OK) {
    return ret;
  }
  // set node namespace
  const auto & d_namespace_it = slave_target->discovered_namespaces.find(gid);
  if (d_namespace_it != slave_target->discovered_namespaces.end()) {
    ret = rmw_topic_info_set_node_namespace(allocator, d_namespace_it->second.c_str(), topic_info);
  } else {
    ret = rmw_topic_info_set_node_namespace(allocator, "_NODE_NAMESPACE_UNKNOWN_", topic_info);
  }
  return ret;
}


rmw_ret_t
_get_info_by_topic(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  bool is_publisher,
  rmw_topic_info_array_t * participants_info)
{
  rmw_ret_t ret = _validate_params(
    identifier,
    node,
    allocator,
    topic_name,
    participants_info);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  const auto & topic_fqdns = _get_topic_fqdns(topic_name, no_mangle);

  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  ::ParticipantListener * slave_target = impl->listener;
  auto & topic_cache =
    is_publisher ? slave_target->writer_topic_cache : slave_target->reader_topic_cache;
  {
    std::lock_guard<std::mutex> guard(topic_cache.getMutex());
    auto & topic_name_to_data = topic_cache().getTopicNameToTopicData();
    std::vector<rmw_topic_info_t> topic_info_vector;
    for (const auto & topic_name : topic_fqdns) {
      const auto & it = topic_name_to_data.find(topic_name);
      if (it != topic_name_to_data.end()) {
        for (const auto & data : it->second) {
          rmw_topic_info_t topic_info;
          rmw_ret_t ret = _set_rmw_topic_info(allocator, data, slave_target, &topic_info);
          if (ret != RMW_RET_OK) {
            RMW_SET_ERROR_MSG("Failed to create set_rmw_topic_info.");
            return ret;
          }
          // add rmw_topic_info_t to a vector
          topic_info_vector.push_back(topic_info);
        }
      }
    }

    // add all the elements from the vector to rmw_topic_info_array_t
    auto count = topic_info_vector.size();
    ret = rmw_topic_info_array_init_with_size(allocator, count, participants_info);
    if (ret != RMW_RET_OK) {
      RMW_SET_ERROR_MSG("rmw_topic_info_array_init_with_size failed to allocate memory.");
      return RMW_RET_BAD_ALLOC;
    }
    for (auto i = 0u; i < count; i++) {
      participants_info->info_array[i] = topic_info_vector.at(i);
    }
    participants_info->count = count;
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
  rmw_topic_info_array_t * publishers_info)
{
  return _get_info_by_topic(
    identifier,
    node,
    allocator,
    topic_name,
    no_mangle,
    true, /*is_publisher*/
    publishers_info);
}

rmw_ret_t
__rmw_get_subscriptions_info_by_topic(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * topic_name,
  bool no_mangle,
  rmw_topic_info_array_t * subscriptions_info)
{
  return _get_info_by_topic(
    identifier,
    node,
    allocator,
    topic_name,
    no_mangle,
    false, /*is_publisher*/
    subscriptions_info);
}
}  // namespace rmw_fastrtps_shared_cpp
