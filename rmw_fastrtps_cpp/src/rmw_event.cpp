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

#include "rmw_fastrtps_cpp/identifier.hpp"

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
rmw_event_set_listener_callback(
  rmw_event_t * rmw_event,
  rmw_listener_callback_t callback,
  const void * user_data,
  bool use_previous_events)
{
  return rmw_fastrtps_shared_cpp::__rmw_event_set_listener_callback(
    rmw_event,
    callback,
    user_data,
    use_previous_events);
}
}  // extern "C"
