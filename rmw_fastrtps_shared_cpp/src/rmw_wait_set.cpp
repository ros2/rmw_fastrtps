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
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

#include "types/custom_wait_set_info.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_wait_set_t *
__rmw_create_wait_set(const char * identifier, size_t max_conditions)
{
  (void)max_conditions;
  rmw_wait_set_t * wait_set = rmw_wait_set_allocate();
  CustomWaitsetInfo * wait_set_info = nullptr;

  // From here onward, error results in unrolling in the goto fail block.
  if (!wait_set) {
    RMW_SET_ERROR_MSG("failed to allocate wait set");
    goto fail;
  }
  wait_set->implementation_identifier = identifier;
  wait_set->data = rmw_allocate(sizeof(CustomWaitsetInfo));
  // This should default-construct the fields of CustomWaitsetInfo
  wait_set_info = static_cast<CustomWaitsetInfo *>(wait_set->data);
  RMW_TRY_PLACEMENT_NEW(wait_set_info, wait_set_info, goto fail, CustomWaitsetInfo, )
  if (!wait_set_info) {
    RMW_SET_ERROR_MSG("failed to construct wait set info struct");
    goto fail;
  }

  return wait_set;

fail:
  if (wait_set) {
    if (wait_set->data) {
      RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
        wait_set_info->~CustomWaitsetInfo(), wait_set_info)
      rmw_free(wait_set->data);
    }
    rmw_wait_set_free(wait_set);
  }
  return nullptr;
}

rmw_ret_t
__rmw_destroy_wait_set(const char * identifier, rmw_wait_set_t * wait_set)
{
  if (!wait_set) {
    RMW_SET_ERROR_MSG("wait set handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    wait set handle,
    wait_set->implementation_identifier, identifier,
    return RMW_RET_ERROR)

  auto result = RMW_RET_OK;
  auto wait_set_info = static_cast<CustomWaitsetInfo *>(wait_set->data);
  if (!wait_set_info) {
    RMW_SET_ERROR_MSG("wait set info is null");
    return RMW_RET_ERROR;
  }
  std::mutex * conditionMutex = &wait_set_info->condition_mutex;
  if (!conditionMutex) {
    RMW_SET_ERROR_MSG("wait set mutex is null");
    return RMW_RET_ERROR;
  }

  if (wait_set->data) {
    if (wait_set_info) {
      RMW_TRY_DESTRUCTOR(
        wait_set_info->~CustomWaitsetInfo(), wait_set_info, result = RMW_RET_ERROR)
    }
    rmw_free(wait_set->data);
  }
  rmw_wait_set_free(wait_set);
  return result;
}
}  // namespace rmw_fastrtps_shared_cpp
