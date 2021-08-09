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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take(
    eprosima_fastrtps_identifier, subscription, ros_message, taken, allocation);
}

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_with_info(
    eprosima_fastrtps_identifier, subscription, ros_message, taken, message_info, allocation);
}

rmw_ret_t
rmw_take_sequence(
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_sequence(
    eprosima_fastrtps_identifier, subscription, count, message_sequence, message_info_sequence,
    taken, allocation);
}

rmw_ret_t
rmw_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_serialized_message(
    eprosima_fastrtps_identifier, subscription, serialized_message, taken, allocation);
}

rmw_ret_t
rmw_take_serialized_message_with_info(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_serialized_message_with_info(
    eprosima_fastrtps_identifier, subscription, serialized_message, taken, message_info,
    allocation);
}

rmw_ret_t
rmw_take_loaned_message(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  static_cast<void>(allocation);
  return rmw_fastrtps_shared_cpp::__rmw_take_loaned_message_internal(
    eprosima_fastrtps_identifier, subscription, loaned_message, taken, nullptr);
}

rmw_ret_t
rmw_take_loaned_message_with_info(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  static_cast<void>(allocation);
  RMW_CHECK_ARGUMENT_FOR_NULL(message_info, RMW_RET_INVALID_ARGUMENT);
  return rmw_fastrtps_shared_cpp::__rmw_take_loaned_message_internal(
    eprosima_fastrtps_identifier, subscription, loaned_message, taken, message_info);
}

rmw_ret_t
rmw_return_loaned_message_from_subscription(
  const rmw_subscription_t * subscription,
  void * loaned_message)
{
  return rmw_fastrtps_shared_cpp::__rmw_return_loaned_message_from_subscription(
    eprosima_fastrtps_identifier, subscription, loaned_message);
}

rmw_ret_t
rmw_take_event(
  const rmw_event_t * event_handle,
  void * event_info,
  bool * taken)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_event(
    eprosima_fastrtps_identifier, event_handle, event_info, taken);
}
}  // extern "C"
