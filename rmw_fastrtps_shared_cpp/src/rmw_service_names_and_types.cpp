// Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <map>
#include <set>
#include <string>
#include <vector>

#include "rcutils/strdup.h"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"

#include "demangle.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"

#include "rmw_fastrtps_shared_cpp/topic_cache.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_get_service_names_and_types(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types)
{
  if (!allocator) {
    RMW_SET_ERROR_MSG("allocator is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_INVALID_ARGUMENT;
  }
  rmw_ret_t ret = rmw_names_and_types_check_zero(service_names_and_types);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  // Get participant pointer from node
  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto impl = static_cast<CustomParticipantInfo *>(node->data);

  // Access the slave Listeners, which are the ones that have the topicnamesandtypes member
  // Get info from publisher and subscriber
  // Combined results from the two lists
  std::map<std::string, std::set<std::string>> services;

  // Setup processing function, will be used with two maps
  auto map_process = [&services](const LockedObject<TopicCache> & topic_cache) {
      std::lock_guard<std::mutex> guard(topic_cache.getMutex());
      for (auto it : topic_cache().getTopicToTypes()) {
        std::string service_name = _demangle_service_from_topic(it.first);
        if (service_name.empty()) {
          // not a service
          continue;
        }
        for (auto & itt : it.second) {
          std::string service_type = _demangle_service_type_only(itt);
          if (!service_type.empty()) {
            services[service_name].insert(service_type);
          }
        }
      }
    };

  ::ParticipantListener * slave_target = impl->listener;
  map_process(slave_target->reader_topic_cache);
  map_process(slave_target->writer_topic_cache);

  // Fill out service_names_and_types
  if (!services.empty()) {
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
            "rmw_fastrtps_shared_cpp",
            "error during report of error: %s", rmw_get_error_string().str);
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
      {
        rcutils_ret_t rcutils_ret = rcutils_string_array_init(
          &service_names_and_types->types[index],
          service_n_types.second.size(),
          allocator);
        if (rcutils_ret != RCUTILS_RET_OK) {
          RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
          fail_cleanup();
          return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
        }
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
      }  // for each type
      ++index;
    }  // for each service
  }
  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
