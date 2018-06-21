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

#include <array>
#include <utility>
#include <set>
#include <string>

#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"

extern "C"
{
rmw_node_t *
rmw_create_node(
  const char * name,
  const char * namespace_,
  size_t domain_id,
  const rmw_node_security_options_t * security_options)
{
  return rmw_fastrtps_shared_cpp::__rmw_create_node(
    eprosima_fastrtps_identifier, name, namespace_, domain_id, security_options);
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  return rmw_fastrtps_shared_cpp::__rmw_destroy_node(
    eprosima_fastrtps_identifier, node);
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  return rmw_fastrtps_shared_cpp::__rmw_node_get_graph_guard_condition(
    node);
}
}  // extern "C"
