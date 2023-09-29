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
#include <string>
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_count_publishers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  return rmw_fastrtps_shared_cpp::__rmw_count_publishers(
    eprosima_fastrtps_identifier, node, topic_name, count);
}

rmw_ret_t
rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  return rmw_fastrtps_shared_cpp::__rmw_count_subscribers(
    eprosima_fastrtps_identifier, node, topic_name, count);
}

rmw_ret_t
rmw_count_clients(
  const rmw_node_t * node,
  const char * service_name,
  size_t * count)
{
  return rmw_fastrtps_shared_cpp::__rmw_count_clients(
    eprosima_fastrtps_identifier, node, service_name, count);
}

rmw_ret_t
rmw_count_services(
  const rmw_node_t * node,
  const char * service_name,
  size_t * count)
{
  return rmw_fastrtps_shared_cpp::__rmw_count_services(
    eprosima_fastrtps_identifier, node, service_name, count);
}
}  // extern "C"
