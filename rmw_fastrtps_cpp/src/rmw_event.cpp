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

#include "rmw/rmw.h"

#include "rmw_fastrtps_cpp/identifier.hpp"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"


extern "C"
{

rmw_event_t *
rmw_create_publisher_event(const rmw_publisher_t * publisher)
{
  return rmw_fastrtps_shared_cpp::__rmw_create_publisher_event(
    eprosima_fastrtps_identifier, publisher);
}

rmw_event_t *
rmw_create_subscription_event(const rmw_subscription_t * subscription)
{
  return rmw_fastrtps_shared_cpp::__rmw_create_subscription_event(
    eprosima_fastrtps_identifier, subscription);
}

rmw_event_t *
rmw_create_client_event(const rmw_client_t * client)
{
  return rmw_fastrtps_shared_cpp::__rmw_create_client_event(
    eprosima_fastrtps_identifier, client);
}

rmw_event_t *
rmw_create_service_event(const rmw_service_t * service)
{
  return rmw_fastrtps_shared_cpp::__rmw_create_service_event(
    eprosima_fastrtps_identifier, service);
}

rmw_ret_t
rmw_destroy_event(rmw_event_t * event)
{
  return rmw_fastrtps_shared_cpp::__rmw_destroy_event(
    eprosima_fastrtps_identifier, event);
}

}  // extern "C"
