// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include "identifier.hpp"
#include "types/custom_waitset_info.hpp"

extern "C"
{
rmw_waitset_t *
rmw_create_waitset(size_t max_conditions)
{
  (void)max_conditions;
  rmw_waitset_t * waitset = rmw_waitset_allocate();
  CustomWaitsetInfo * waitset_info = nullptr;

  // From here onward, error results in unrolling in the goto fail block.
  if (!waitset) {
    RMW_SET_ERROR_MSG("failed to allocate waitset");
    goto fail;
  }
  waitset->implementation_identifier = eprosima_fastrtps_identifier;
  waitset->data = rmw_allocate(sizeof(CustomWaitsetInfo));
  // This should default-construct the fields of CustomWaitsetInfo
  waitset_info = static_cast<CustomWaitsetInfo *>(waitset->data);
  RMW_TRY_PLACEMENT_NEW(waitset_info, waitset_info, goto fail, CustomWaitsetInfo, )
  if (!waitset_info) {
    RMW_SET_ERROR_MSG("failed to construct waitset info struct");
    goto fail;
  }

  return waitset;

fail:
  if (waitset) {
    if (waitset->data) {
      RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
        waitset_info->~CustomWaitsetInfo(), waitset_info)
      rmw_free(waitset->data);
    }
    rmw_waitset_free(waitset);
  }
  return nullptr;
}

rmw_ret_t
rmw_destroy_waitset(rmw_waitset_t * waitset)
{
  if (!waitset) {
    RMW_SET_ERROR_MSG("waitset handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    waitset handle,
    waitset->implementation_identifier, eprosima_fastrtps_identifier,
    return RMW_RET_ERROR)

  auto result = RMW_RET_OK;
  CustomWaitsetInfo * waitset_info = static_cast<CustomWaitsetInfo *>(waitset->data);
  if (!waitset_info) {
    RMW_SET_ERROR_MSG("waitset info is null");
    return RMW_RET_ERROR;
  }
  std::mutex * conditionMutex = &waitset_info->condition_mutex;
  if (!conditionMutex) {
    RMW_SET_ERROR_MSG("waitset mutex is null");
    return RMW_RET_ERROR;
  }

  if (waitset->data) {
    if (waitset_info) {
      RMW_TRY_DESTRUCTOR(
        waitset_info->~CustomWaitsetInfo(), waitset_info, result = RMW_RET_ERROR)
    }
    rmw_free(waitset->data);
  }
  rmw_waitset_free(waitset);
  return result;
}
}  // extern "C"
