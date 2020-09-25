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
__rmw_create_wait_set(const char * identifier, rmw_context_t * context, size_t max_conditions)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init context,
    context->implementation_identifier,
    identifier,
    // TODO(wjwwood): replace this with RMW_RET_INCORRECT_RMW_IMPLEMENTATION when refactored
    return nullptr);

  (void)max_conditions;

  // From here onward, error results in unrolling in the goto fail block.
  CustomWaitsetInfo * wait_set_info = nullptr;
  rmw_wait_set_t * wait_set = rmw_wait_set_allocate();
  if (!wait_set) {
    RMW_SET_ERROR_MSG("failed to allocate wait set");
    goto fail;
  }
  wait_set->implementation_identifier = identifier;
  wait_set->data = rmw_allocate(sizeof(CustomWaitsetInfo));
  if (!wait_set->data) {
    RMW_SET_ERROR_MSG("failed to allocate wait set info");
    goto fail;
  }
  // This should default-construct the fields of CustomWaitsetInfo
  RMW_TRY_PLACEMENT_NEW(
    wait_set_info,
    wait_set->data,
    goto fail,
    // cppcheck-suppress syntaxError
    CustomWaitsetInfo, );
  (void) wait_set_info;

  return wait_set;

fail:
  if (wait_set) {
    if (wait_set->data) {
      rmw_free(wait_set->data);
    }
    rmw_wait_set_free(wait_set);
  }
  return nullptr;
}

rmw_ret_t
__rmw_destroy_wait_set(const char * identifier, rmw_wait_set_t * wait_set)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    wait set handle,
    wait_set->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)

  auto result = RMW_RET_OK;
  // If wait_set_info is ever nullptr, it can only mean one of three things:
  // - Wait set is invalid. Caller did not respect preconditions.
  // - Implementation is logically broken. Definitely not something we want to treat as a normal
  // error.
  // - Heap is corrupt.
  // In all three cases, it's better if this crashes soon enough.
  auto wait_set_info = static_cast<CustomWaitsetInfo *>(wait_set->data);

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
