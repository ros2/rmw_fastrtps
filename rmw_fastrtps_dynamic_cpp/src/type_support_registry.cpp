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

#include "rcutils/logging_macros.h"

#include "rmw/error_handling.h"

#include "type_support_common.hpp"
#include "type_support_registry.hpp"

template<typename key_type, typename map_type, typename creator>
type_support_ptr get_type_support(
  const key_type & ros_type_support, map_type & map, creator fun)
{
  std::lock_guard<std::mutex> guard(map.getMutex());
  RefCountedTypeSupport & item = map()[ros_type_support];
  if (0 == item.ref_count++) {
    item.type_support = fun();
    if (!item.type_support) {
      map().erase(ros_type_support);
      return nullptr;
    }
  }
  return item.type_support;
}

template<typename key_type, typename map_type>
void return_type_support(
  const key_type & ros_type_support, map_type & map)
{
  std::lock_guard<std::mutex> guard(map.getMutex());
  auto it = map().find(ros_type_support);
  assert(it != map().end());
  if (0 == --it->second.ref_count) {
    delete it->second.type_support;
    map().erase(it);
  }
}

template<typename map_type>
void cleanup(map_type & map, const char * msg)
{
  std::lock_guard<std::mutex> guard(map.getMutex());
  if (!map().empty()) {
    RCUTILS_LOG_DEBUG_NAMED(
      "rmw_fastrtps_dynamic_cpp",
      "TypeSupportRegistry %s is not empty. Cleaning it up...", msg);
    for (auto it : map() ) {
      delete it.second.type_support;
    }
    map().clear();
  }
}

TypeSupportRegistry::~TypeSupportRegistry()
{
  cleanup(message_types_, "message_types_");
  cleanup(request_types_, "request_types_");
  cleanup(response_types_, "response_types_");
}

TypeSupportRegistry & TypeSupportRegistry::get_instance()
{
  static TypeSupportRegistry type_registry_instance;
  return type_registry_instance;
}

type_support_ptr TypeSupportRegistry::get_message_type_support(
  const rosidl_message_type_support_t * ros_type_support)
{
  auto creator_fun = [&ros_type_support]() -> type_support_ptr
    {
      if (using_introspection_c_typesupport(ros_type_support->typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_c__MessageMembers *>(
          ros_type_support->data);
        return new MessageTypeSupport_c(members, ros_type_support);
      } else if (using_introspection_cpp_typesupport(ros_type_support->typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers *>(
          ros_type_support->data);
        return new MessageTypeSupport_cpp(members, ros_type_support);
      }
      RMW_SET_ERROR_MSG("Unknown typesupport identifier");
      return nullptr;
    };

  return get_type_support(ros_type_support, message_types_, creator_fun);
}

type_support_ptr TypeSupportRegistry::get_request_type_support(
  const rosidl_service_type_support_t * ros_type_support)
{
  auto creator_fun = [&ros_type_support]() -> type_support_ptr
    {
      if (using_introspection_c_typesupport(ros_type_support->typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
          ros_type_support->data);
        return new RequestTypeSupport_c(members, ros_type_support);
      } else if (using_introspection_cpp_typesupport(ros_type_support->typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers *>(
          ros_type_support->data);
        return new RequestTypeSupport_cpp(members, ros_type_support);
      }
      RMW_SET_ERROR_MSG("Unknown typesupport identifier");
      return nullptr;
    };

  return get_type_support(ros_type_support, request_types_, creator_fun);
}

type_support_ptr TypeSupportRegistry::get_response_type_support(
  const rosidl_service_type_support_t * ros_type_support)
{
  auto creator_fun = [&ros_type_support]() -> type_support_ptr
    {
      if (using_introspection_c_typesupport(ros_type_support->typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers *>(
          ros_type_support->data);
        return new ResponseTypeSupport_c(members, ros_type_support);
      } else if (using_introspection_cpp_typesupport(ros_type_support->typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers *>(
          ros_type_support->data);
        return new ResponseTypeSupport_cpp(members, ros_type_support);
      }
      RMW_SET_ERROR_MSG("Unknown typesupport identifier");
      return nullptr;
    };

  return get_type_support(ros_type_support, response_types_, creator_fun);
}

void TypeSupportRegistry::return_message_type_support(
  const rosidl_message_type_support_t * ros_type_support)
{
  return_type_support(ros_type_support, message_types_);
}

void TypeSupportRegistry::return_request_type_support(
  const rosidl_service_type_support_t * ros_type_support)
{
  return_type_support(ros_type_support, request_types_);
}

void TypeSupportRegistry::return_response_type_support(
  const rosidl_service_type_support_t * ros_type_support)
{
  return_type_support(ros_type_support, response_types_);
}
