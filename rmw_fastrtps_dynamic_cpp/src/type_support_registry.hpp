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

#ifndef TYPE_SUPPORT_REGISTRY_HPP_
#define TYPE_SUPPORT_REGISTRY_HPP_

#include <unordered_map>

#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/locked_object.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "type_support_common.hpp"

using type_support_ptr = rmw_fastrtps_dynamic_cpp::BaseTypeSupport *;

/**
 * A data structure to use as value type for the type registry.
 */
struct RefCountedTypeSupport
{
  type_support_ptr type_support = nullptr;
  uint32_t ref_count = 0;
};

using msg_map_t = std::unordered_map<const rosidl_message_type_support_t *, RefCountedTypeSupport>;
using srv_map_t = std::unordered_map<const rosidl_service_type_support_t *, RefCountedTypeSupport>;

class TypeSupportRegistry
{
private:
  LockedObject<msg_map_t> message_types_;
  LockedObject<srv_map_t> request_types_;
  LockedObject<srv_map_t> response_types_;

  TypeSupportRegistry() = default;

public:
  ~TypeSupportRegistry();

  static TypeSupportRegistry & get_instance();

  type_support_ptr get_message_type_support(
    const rosidl_message_type_support_t * ros_type_support);

  type_support_ptr get_request_type_support(
    const rosidl_service_type_support_t * ros_type_support);

  type_support_ptr get_response_type_support(
    const rosidl_service_type_support_t * ros_type_support);

  void return_message_type_support(
    const rosidl_message_type_support_t * ros_type_support);

  void return_request_type_support(
    const rosidl_service_type_support_t * ros_type_support);

  void return_response_type_support(
    const rosidl_service_type_support_t * ros_type_support);
};

#endif  // TYPE_SUPPORT_REGISTRY_HPP_
