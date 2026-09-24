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
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "fastdds/dds/core/condition/GuardCondition.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_guard_condition_t *
__rmw_create_guard_condition(const char * identifier)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(nullptr);

  rmw_guard_condition_t * guard_condition_handle = new rmw_guard_condition_t;
  guard_condition_handle->implementation_identifier = identifier;
  guard_condition_handle->data = new eprosima::fastdds::dds::GuardCondition();
  return guard_condition_handle;
}

rmw_ret_t
__rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition)
{
  rmw_ret_t ret = RMW_RET_ERROR;

  if (guard_condition) {
    delete static_cast<eprosima::fastdds::dds::GuardCondition *>(guard_condition->data);
    delete guard_condition;
    ret = RMW_RET_OK;
  }

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion
  return ret;
}
}  // namespace rmw_fastrtps_shared_cpp
