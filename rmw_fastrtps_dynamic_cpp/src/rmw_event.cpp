// Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_publisher_event_init(
  rmw_event_t * rmw_event,
  const rmw_publisher_t * publisher,
  rmw_event_type_t event_type)
{
  return rmw_fastrtps_shared_cpp::__rmw_init_event(
    eprosima_fastrtps_identifier,
    rmw_event,
    publisher->implementation_identifier,
    publisher->data,
    event_type);
}

rmw_ret_t
rmw_subscription_event_init(
  rmw_event_t * rmw_event,
  const rmw_subscription_t * subscription,
  rmw_event_type_t event_type)
{
  return rmw_fastrtps_shared_cpp::__rmw_init_event(
    eprosima_fastrtps_identifier,
    rmw_event,
    subscription->implementation_identifier,
    subscription->data,
    event_type);
}

rmw_ret_t
rmw_event_set_callback(
  rmw_event_t * rmw_event,
  rmw_event_callback_t callback,
  const void * user_data)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(rmw_event, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_event_set_callback(
    rmw_event,
    callback,
    user_data);
}

bool
rmw_event_type_is_supported(rmw_event_type_t rmw_event_type)
{
  return rmw_fastrtps_shared_cpp::__rmw_event_type_is_supported(rmw_event_type);
}
}  // extern "C"
