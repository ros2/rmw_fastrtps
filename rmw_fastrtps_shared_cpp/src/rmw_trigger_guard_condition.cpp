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

#include <cassert>

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "fastdds/dds/core/condition/GuardCondition.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_trigger_guard_condition(
  const char * identifier,
  const rmw_guard_condition_t * guard_condition_handle)
{
  assert(guard_condition_handle);

  if (guard_condition_handle->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("guard condition handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto guard_condition =
    static_cast<eprosima::fastdds::dds::GuardCondition *>(guard_condition_handle->data);
  guard_condition->set_trigger_value(true);
  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
