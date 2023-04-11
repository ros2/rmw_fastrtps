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

#include <fastcdr/Cdr.h>
#include <rcutils/allocator.h>
#include <rcutils/logging_macros.h>
#include <rosidl_dynamic_typesupport/types.h>
#include <rosidl_dynamic_typesupport/api/serialization_support_interface.h>
#include <rosidl_dynamic_typesupport_fastrtps/serialization_support.h>

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"


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

rmw_ret_t
rmw_serialization_support_init(
  const char * /*serialization_lib_name*/,
  rcutils_allocator_t * allocator,
  rosidl_dynamic_typesupport_serialization_support_t * serialization_support)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(allocator, RMW_RET_INVALID_ARGUMENT);
  if (!rcutils_allocator_is_valid(allocator)) {
    RMW_SET_ERROR_MSG("allocator is invalid");
    return RMW_RET_INVALID_ARGUMENT;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(serialization_support, RMW_RET_INVALID_ARGUMENT);

  rcutils_ret_t ret = RCUTILS_RET_ERROR;

  rosidl_dynamic_typesupport_serialization_support_impl_t impl =
    rosidl_dynamic_typesupport_get_zero_initialized_serialization_support_impl();

  rosidl_dynamic_typesupport_serialization_support_interface_t methods =
    rosidl_dynamic_typesupport_get_zero_initialized_serialization_support_interface();

  ret = rosidl_dynamic_typesupport_fastrtps_init_serialization_support_impl(allocator, &impl);
  if (ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG_AND_APPEND_PREV_ERROR("Could not initialize serialization support impl");
    goto fail;
  }

  ret = rosidl_dynamic_typesupport_fastrtps_init_serialization_support_interface(
    allocator, &methods);
  if (ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG_AND_APPEND_PREV_ERROR("could not initialize serialization support interface");
    goto fail;
  }

  return rmw_convert_rcutils_ret_to_rmw_ret(
    rosidl_dynamic_typesupport_serialization_support_init(
      &impl, &methods, allocator, serialization_support));

fail:
  if (rosidl_dynamic_typesupport_serialization_support_fini(serialization_support) !=
    RCUTILS_RET_ERROR)
  {
    RCUTILS_SAFE_FWRITE_TO_STDERR_AND_APPEND_PREV_ERROR(
      "While handling another error, could not finalize serialization support");
  }
  return rmw_convert_rcutils_ret_to_rmw_ret(ret);
}
}  // extern "C"
