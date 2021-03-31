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

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"

extern "C"
{
rmw_guard_condition_t *
rmw_create_guard_condition(rmw_context_t * context)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init context,
    context->implementation_identifier,
    eprosima_fastrtps_identifier,
    // TODO(wjwwood): replace this with RMW_RET_INCORRECT_RMW_IMPLEMENTATION when refactored
    return NULL);
  return rmw_fastrtps_shared_cpp::__rmw_create_guard_condition(
    eprosima_fastrtps_identifier);
}

rmw_ret_t
rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition)
{
  return rmw_fastrtps_shared_cpp::__rmw_destroy_guard_condition(
    guard_condition);
}
}  // extern "C"
