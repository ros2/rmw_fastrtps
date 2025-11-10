// Copyright 2025 Minju Lee (이민주).
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

#include "rmw/get_service_endpoint_info.h"
#include "rmw/service_endpoint_info_array.h"
#include "rmw/types.h"
#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

extern "C"
{
rmw_ret_t
rmw_get_clients_info_by_service(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * service_name,
  bool no_mangle,
  rmw_service_endpoint_info_array_t * clients_info)
{
  return rmw_fastrtps_shared_cpp::__rmw_get_clients_info_by_service(
    eprosima_fastrtps_identifier, node, allocator, service_name, no_mangle, clients_info);
}

rmw_ret_t
rmw_get_servers_info_by_service(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * service_name,
  bool no_mangle,
  rmw_service_endpoint_info_array_t * servers_info)
{
  return rmw_fastrtps_shared_cpp::__rmw_get_servers_info_by_service(
    eprosima_fastrtps_identifier, node, allocator, service_name, no_mangle, servers_info);
}
}  // extern "C"
