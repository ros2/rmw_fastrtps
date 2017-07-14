// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include "rcutils/strdup.h"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"

#include "identifier.hpp"
#include "demangle.hpp"
#include "types/custom_participant_info.hpp"

extern "C"
{
rmw_ret_t
rmw_get_service_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types)
{
  if (!allocator) {
    RMW_SET_ERROR_MSG("allocator is null")
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!node) {
    RMW_SET_ERROR_MSG_ALLOC("null node handle", *allocator)
    return RMW_RET_INVALID_ARGUMENT;
  }
  rmw_ret_t ret = rmw_names_and_types_check_zero(service_names_and_types);
  if (ret != RMW_RET_OK) {
    return ret;
  }

  // Get participant pointer from node
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG_ALLOC("node handle not from this implementation", *allocator);
    return RMW_RET_ERROR;
  }

  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);

  // Access the slave Listeners, which are the ones that have the topicnamesandtypes member
  // Get info from publisher and subscriber
  // Combined results from the two lists
  std::map<std::string, std::set<std::string>> services;
  {
    ReaderInfo * slave_target = impl->secondarySubListener;
    slave_target->mapmutex.lock();
    for (auto it : slave_target->topicNtypes) {
      std::string service_name = _demangle_service_from_topic(it.first);
      if (!service_name.length()) {
        // not a service
        continue;
      }
      for (auto & itt : it.second) {
        std::string service_type = _demangle_service_type_only(itt);
        if (service_type.length()) {
          services[service_name].insert(service_type);
        }
      }
    }
    slave_target->mapmutex.unlock();
  }
  {
    WriterInfo * slave_target = impl->secondaryPubListener;
    slave_target->mapmutex.lock();
    for (auto it : slave_target->topicNtypes) {
      std::string service_name = _demangle_service_from_topic(it.first);
      if (!service_name.length()) {
        // not a service
        continue;
      }
      for (auto & itt : it.second) {
        std::string service_type = _demangle_service_type_only(itt);
        if (service_type.length()) {
          services[service_name].insert(service_type);
        }
      }
    }
    slave_target->mapmutex.unlock();
  }

  // Fill out service_names_and_types
  if (services.size()) {
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
          RCUTILS_LOG_ERROR("error during report of error: %s", rmw_get_error_string_safe())
        }
      };
    // For each service, store the name, initialize the string array for types, and store all types
    size_t index = 0;
    for (const auto & service_n_types : services) {
      // Duplicate and store the service_name
      char * service_name = rcutils_strdup(service_n_types.first.c_str(), *allocator);
      if (!service_name) {
        RMW_SET_ERROR_MSG_ALLOC("failed to allocate memory for service name", *allocator);
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
          RMW_SET_ERROR_MSG(rcutils_get_error_string_safe())
          fail_cleanup();
          return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
        }
      }
      // Duplicate and store each type for the service
      size_t type_index = 0;
      for (const auto & type : service_n_types.second) {
        char * type_name = rcutils_strdup(type.c_str(), *allocator);
        if (!type_name) {
          RMW_SET_ERROR_MSG_ALLOC("failed to allocate memory for type name", *allocator)
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
}  // extern "C"
