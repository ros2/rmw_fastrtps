// Copyright 2022 Open Source Robotics Foundation, Inc.
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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include <rcutils/logging_macros.h>

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include <rosidl_dynamic_typesupport_fastrtps/serialization_support.h>

#include <fastcdr/Cdr.h>

extern "C"
{
rmw_ret_t
rmw_take_dynamic_message(
  const rmw_subscription_t * subscription,
  rosidl_dynamic_typesupport_dynamic_data_t * dynamic_data,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_dynamic_message(
    eprosima_fastrtps_identifier, subscription, dynamic_data, taken, allocation);
}

rmw_ret_t
rmw_take_dynamic_message_with_info(
  const rmw_subscription_t * subscription,
  rosidl_dynamic_typesupport_dynamic_data_t * dynamic_data,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_dynamic_message_with_info(
    eprosima_fastrtps_identifier, subscription, dynamic_data, taken, message_info,
    allocation);
}

rosidl_dynamic_typesupport_serialization_support_t *
rmw_get_serialization_support(  // Fallback to rcl if the rmw doesn't implement it
  const char * /*serialization_lib_name*/)
{
  return rosidl_dynamic_typesupport_serialization_support_init(
    rosidl_dynamic_typesupport_fastrtps_create_serialization_support_impl(),
    rosidl_dynamic_typesupport_fastrtps_create_serialization_support_interface());
}

}  // extern "C"
