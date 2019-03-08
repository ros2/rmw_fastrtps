// Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"

#include "demangle.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_shared_cpp/topic_cache.hpp"

namespace rmw_fastrtps_shared_cpp
{

constexpr char kLoggerTag[] = "rmw_fastrtps_shared_cpp";

/**
 * Get the guid that corresponds to the node and namespace.
 *
 * @param node to discover other participants with
 * @param node_name of the desired node
 * @param node_namespace of the desired node
 * @param guid [out] result
 * @return RMW_RET_ERROR if unable to find guid
 * @return RMW_RET_OK if guid is available
 */
rmw_ret_t __get_guid_by_name(
  const rmw_node_t * node, const char * node_name,
  const char * node_namespace, GUID_t & guid)
{
  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  if (strcmp(node->name, node_name) == 0) {
    guid = impl->participant->getGuid();
  } else {
    std::set<GUID_t> nodes_in_desired_namespace;
    auto namespaces = impl->listener->discovered_namespaces;
    for (auto & guid_to_namespace : impl->listener->discovered_namespaces) {
      if (guid_to_namespace.second == node_namespace) {
        nodes_in_desired_namespace.insert(guid_to_namespace.first);
      }
    }

    auto guid_node_pair = std::find_if(impl->listener->discovered_names.begin(),
        impl->listener->discovered_names.end(),
        [node_name, &nodes_in_desired_namespace](const std::pair<const GUID_t,
        std::string> & pair) {
          return pair.second == node_name &&
          nodes_in_desired_namespace.find(pair.first) != nodes_in_desired_namespace.end();
        });

    if (guid_node_pair == impl->listener->discovered_names.end()) {
      RCUTILS_LOG_ERROR_NAMED(
        kLoggerTag,
        "Unable to find GUID for node: %s", node_name);
      RMW_SET_ERROR_MSG("Unable to find GUID for node ");
      return RMW_RET_ERROR;
    }
    guid = guid_node_pair->first;
  }
  return RMW_RET_OK;
}

/**
 * Validate the input data of node_info_and_types functions.
 *
 * @return RMW_RET_INVALID_ARGUMENT for null input args
 * @return RMW_RET_ERROR if identifier is not the same as the input node
 * @return RMW_RET_OK if all input is valid
 */
rmw_ret_t __validate_input(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * topic_names_and_types)
{
  if (!allocator) {
    RMW_SET_ERROR_MSG("allocator is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (!node_name) {
    RMW_SET_ERROR_MSG("null node name");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (!node_namespace) {
    RMW_SET_ERROR_MSG("null node namespace");
    return RMW_RET_INVALID_ARGUMENT;
  }

  rmw_ret_t ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  // Get participant pointer from node
  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }
  return RMW_RET_OK;
}

/**
 * Access the slave Listeners, which are the ones that have the topicnamesandtypes member
 * Get info from publisher and subscriber
 * Combined results from the two lists
 *
 * @param topic_cache cache with topic information
 * @param topics [out] resulting topics
 * @param node_guid_ to find information for
 * @param no_demangle true if demangling will not occur
 */
void
__accumulate_topics(
  const LockedObject<TopicCache> & topic_cache,
  std::map<std::string, std::set<std::string>> & topics,
  const GUID_t & node_guid_,
  bool no_demangle)
{
  std::lock_guard<std::mutex> guard(topic_cache.getMutex());
  const auto & node_topics = topic_cache().getParticipantToTopics().find(node_guid_);
  if (node_topics == topic_cache().getParticipantToTopics().end()) {
    RCUTILS_LOG_DEBUG_NAMED(
      kLoggerTag,
      "No topics found for node");
    return;
  }
  for (auto & topic_pair : node_topics->second) {
    if (!no_demangle && _get_ros_prefix_if_exists(topic_pair.first) != ros_topic_prefix) {
      // if we are demangling and this is not prefixed with rt/, skip it
      continue;
    }
    RCUTILS_LOG_DEBUG_NAMED(
      kLoggerTag,
      "accumulate_topics: Found topic %s",
      topic_pair.first.c_str());

    topics[topic_pair.first].insert(topic_pair.second.begin(),
      topic_pair.second.end());
  }
}

/**
 * Copy topic data to results
 *
 * @param topics to copy over
 * @param allocator to use
 * @param no_demangle true if demangling will not occur
 * @param topic_names_and_types [out] final rmw result
 * @return RMW_RET_OK if successful
 */
rmw_ret_t
__copy_data_to_results(
  const std::map<std::string, std::set<std::string>> & topics,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  // Copy data to results handle
  if (!topics.empty()) {
    // Setup string array to store names
    rmw_ret_t rmw_ret = rmw_names_and_types_init(topic_names_and_types, topics.size(), allocator);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }
    // Setup cleanup function, in case of failure below
    auto fail_cleanup = [&topic_names_and_types]() {
        rmw_ret_t rmw_ret = rmw_names_and_types_fini(topic_names_and_types);
        if (rmw_ret != RMW_RET_OK) {
          RCUTILS_LOG_ERROR_NAMED(
            kLoggerTag,
            "error during report of error: %s", rmw_get_error_string());
        }
      };
    // Setup demangling functions based on no_demangle option
    auto demangle_topic = _demangle_if_ros_topic;
    auto demangle_type = _demangle_if_ros_type;
    if (no_demangle) {
      auto noop = [](const std::string & in) {
          return in;
        };
      demangle_topic = noop;
      demangle_type = noop;
    }
    // For each topic, store the name, initialize the string array for types, and store all types
    size_t index = 0;
    for (const auto & topic_n_types : topics) {
      // Duplicate and store the topic_name
      char * topic_name = rcutils_strdup(demangle_topic(topic_n_types.first).c_str(), *allocator);
      if (!topic_name) {
        RMW_SET_ERROR_MSG("failed to allocate memory for topic name");
        fail_cleanup();
        return RMW_RET_BAD_ALLOC;
      }
      topic_names_and_types->names.data[index] = topic_name;
      // Setup storage for types
      {
        rcutils_ret_t rcutils_ret = rcutils_string_array_init(
          &topic_names_and_types->types[index],
          topic_n_types.second.size(),
          allocator);
        if (rcutils_ret != RCUTILS_RET_OK) {
          RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
          fail_cleanup();
          return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
        }
      }
      // Duplicate and store each type for the topic
      size_t type_index = 0;
      for (const auto & type : topic_n_types.second) {
        char * type_name = rcutils_strdup(demangle_type(type).c_str(), *allocator);
        if (!type_name) {
          RMW_SET_ERROR_MSG("failed to allocate memory for type name");
          fail_cleanup();
          return RMW_RET_BAD_ALLOC;
        }
        topic_names_and_types->types[index].data[type_index] = type_name;
        ++type_index;
      }        // for each type
      ++index;
    }      // for each topic
  }
  return RMW_RET_OK;
}

void
__log_debug_information(const CustomParticipantInfo & impl)
{
  if (rcutils_logging_logger_is_enabled_for(kLoggerTag, RCUTILS_LOG_SEVERITY_DEBUG)) {
    {
      auto & topic_cache = impl.listener->writer_topic_cache;
      std::lock_guard<std::mutex> guard(topic_cache.getMutex());
      std::stringstream map_ss;
      map_ss << topic_cache();
      RCUTILS_LOG_DEBUG_NAMED(
        kLoggerTag,
        "Publisher Topic cache is: %s", map_ss.str().c_str());
    }
    {
      auto & topic_cache = impl.listener->reader_topic_cache;
      std::lock_guard<std::mutex> guard(topic_cache.getMutex());
      std::stringstream map_ss;
      map_ss << topic_cache();
      RCUTILS_LOG_DEBUG_NAMED(
        kLoggerTag,
        "Subscriber Topic cache is: %s", map_ss.str().c_str());
    }
    {
      std::stringstream ss;
      for (auto & node_pair : impl.listener->discovered_names) {
        ss << node_pair.first << " : " << node_pair.second << " ";
      }
      RCUTILS_LOG_DEBUG_NAMED(kLoggerTag, "Discovered names: %s", ss.str().c_str());
    }
    {
      std::stringstream ss;
      for (auto & node_pair : impl.listener->discovered_namespaces) {
        ss << node_pair.first << " : " << node_pair.second << " ";
      }
      RCUTILS_LOG_DEBUG_NAMED(kLoggerTag, "Discovered namespaces: %s", ss.str().c_str());
    }
  }
}

/**
 * Function to abstract which topic_cache to use when gathering information.
 */
typedef std::function<const LockedObject<TopicCache>&(CustomParticipantInfo & participant_info)>
  RetrieveCache;

/**
 * Get topic names and types for the specific node_name and node_namespace requested.
 *
 * @param identifier corresponding to the input node
 * @param node to use for discovery
 * @param allocator for returned value
 * @param node_name to search
 * @param node_namespace to search
 * @param no_demangle true if the topics should not be demangled
 * @param retrieve_cache_func getter for topic cache
 * @param topic_names_and_types result
 * @return RMW_RET_OK if successful
 */
rmw_ret_t
__rmw_get_topic_names_and_types_by_node(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  RetrieveCache & retrieve_cache_func,
  rmw_names_and_types_t * topic_names_and_types)
{
  rmw_ret_t valid_input = __validate_input(identifier, node, allocator, node_name,
      node_namespace, topic_names_and_types);
  if (valid_input != RMW_RET_OK) {
    return valid_input;
  }
  RCUTILS_LOG_DEBUG_NAMED(kLoggerTag, "rmw_get_subscriber_names_and_types_by_node");
  auto impl = static_cast<CustomParticipantInfo *>(node->data);

  __log_debug_information(*impl);

  GUID_t guid;
  rmw_ret_t valid_guid = __get_guid_by_name(node, node_name, node_namespace, guid);
  if (valid_guid != RMW_RET_OK) {
    return valid_guid;
  }
  std::map<std::string, std::set<std::string>> topics;
  __accumulate_topics(retrieve_cache_func(*impl), topics, guid, no_demangle);
  return __copy_data_to_results(topics, allocator, no_demangle, topic_names_and_types);
}

rmw_ret_t
__rmw_get_subscriber_names_and_types_by_node(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  RetrieveCache retrieve_sub_cache =
    [](CustomParticipantInfo & participant_info) -> const LockedObject<TopicCache> & {
      return participant_info.listener->reader_topic_cache;
    };
  return __rmw_get_topic_names_and_types_by_node(identifier, node, allocator, node_name,
           node_namespace, no_demangle, retrieve_sub_cache, topic_names_and_types);
}

rmw_ret_t
__rmw_get_publisher_names_and_types_by_node(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  RetrieveCache retrieve_pub_cache =
    [](CustomParticipantInfo & participant_info) -> const LockedObject<TopicCache> & {
      return participant_info.listener->writer_topic_cache;
    };
  return __rmw_get_topic_names_and_types_by_node(identifier, node, allocator, node_name,
           node_namespace, no_demangle, retrieve_pub_cache, topic_names_and_types);
}

rmw_ret_t
__rmw_get_service_names_and_types_by_node(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types)
{
  rmw_ret_t valid_input = __validate_input(identifier, node, allocator, node_name,
      node_namespace, service_names_and_types);
  if (valid_input != RMW_RET_OK) {
    return valid_input;
  }
  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  __log_debug_information(*impl);

  GUID_t guid;
  rmw_ret_t valid_guid = __get_guid_by_name(node, node_name, node_namespace, guid);
  if (valid_guid != RMW_RET_OK) {
    return valid_guid;
  }

  std::map<std::string, std::set<std::string>> services;
  {
    auto & topic_cache = impl->listener->reader_topic_cache;
    std::lock_guard<std::mutex> guard(topic_cache.getMutex());
    const auto & node_topics = topic_cache().getParticipantToTopics().find(guid);
    if (node_topics != topic_cache().getParticipantToTopics().end()) {
      for (auto & topic_pair : node_topics->second) {
        std::string service_name = _demangle_service_from_topic(topic_pair.first);
        if (service_name.empty()) {
          // not a service
          continue;
        }
        for (auto & itt : topic_pair.second) {
          std::string service_type = _demangle_service_type_only(itt);
          if (!service_type.empty()) {
            services[service_name].insert(service_type);
          }
        }
      }
    }
  }
  if (services.empty()) {
    return RMW_RET_OK;
  }
  // Setup string array to store names
  rmw_ret_t rmw_ret =
    rmw_names_and_types_init(service_names_and_types, services.size(), allocator);
  if (rmw_ret != RMW_RET_OK) {
    return rmw_ret;
  }
  // Setup cleanup function, in case of failure below
  auto fail_cleanup = [&service_names_and_types]() {
      rmw_ret_t rmw_ret = rmw_names_and_types_fini(service_names_and_types);
      if (rmw_ret != RMW_RET_OK) {
        RCUTILS_LOG_ERROR_NAMED(
          kLoggerTag,
          "error during report of error: %s", rmw_get_error_string());
      }
    };
  // For each service, store the name, initialize the string array for types, and store all types
  size_t index = 0;
  for (const auto & service_n_types : services) {
    // Duplicate and store the service_name
    char * service_name = rcutils_strdup(service_n_types.first.c_str(), *allocator);
    if (!service_name) {
      RMW_SET_ERROR_MSG("failed to allocate memory for service name");
      fail_cleanup();
      return RMW_RET_BAD_ALLOC;
    }
    service_names_and_types->names.data[index] = service_name;
    // Setup storage for types
    rcutils_ret_t rcutils_ret = rcutils_string_array_init(
      &service_names_and_types->types[index],
      service_n_types.second.size(),
      allocator);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
      fail_cleanup();
      return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
    }
    // Duplicate and store each type for the service
    size_t type_index = 0;
    for (const auto & type : service_n_types.second) {
      char * type_name = rcutils_strdup(type.c_str(), *allocator);
      if (!type_name) {
        RMW_SET_ERROR_MSG("failed to allocate memory for type name");
        fail_cleanup();
        return RMW_RET_BAD_ALLOC;
      }
      service_names_and_types->types[index].data[type_index] = type_name;
      ++type_index;
    }        // for each type
    ++index;
  }      // for each service
  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
