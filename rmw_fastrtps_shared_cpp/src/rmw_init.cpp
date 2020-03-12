// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#include "rmw_fastrtps_shared_cpp/rmw_init.hpp"

#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/init.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/init_options.h"

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t
rmw_init_options_init(
  const char * identifier, rmw_init_options_t * init_options, rcutils_allocator_t allocator)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);
  if (NULL != init_options->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected zero-initialized init_options");
    return RMW_RET_INVALID_ARGUMENT;
  }
  init_options->instance_id = 0;
  init_options->implementation_identifier = identifier;
  init_options->allocator = allocator;
  init_options->impl = nullptr;
  init_options->name = NULL;
  init_options->domain_id = RMW_DEFAULT_DOMAIN_ID;
  init_options->security_options = rmw_get_default_security_options();
  init_options->localhost_only = RMW_LOCALHOST_ONLY_DEFAULT;
  return RMW_RET_OK;
}

rmw_ret_t
rmw_init_options_copy(
  const char * identifier, const rmw_init_options_t * src, rmw_init_options_t * dst)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(src, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(dst, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    src,
    src->implementation_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (NULL != dst->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected zero-initialized dst");
    return RMW_RET_INVALID_ARGUMENT;
  }
  const rcutils_allocator_t * allocator = &src->allocator;
  rmw_ret_t ret = RMW_RET_OK;

  allocator->deallocate(dst->name, allocator->state);
  *dst = *src;
  dst->name = NULL;
  dst->security_options = rmw_get_zero_initialized_security_options();

  dst->name = rcutils_strdup(src->name, *allocator);
  if (src->name && !dst->name) {
    ret = RMW_RET_BAD_ALLOC;
    goto fail;
  }
  return rmw_security_options_copy(&src->security_options, allocator, &dst->security_options);
fail:
  allocator->deallocate(dst->name, allocator->state);
  return ret;
}

rmw_ret_t
rmw_init_options_fini(const char * identifier, rmw_init_options_t * init_options)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  rcutils_allocator_t & allocator = init_options->allocator;
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init_options,
    init_options->implementation_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  allocator.deallocate(init_options->name, allocator.state);
  rmw_security_options_fini(&init_options->security_options, &allocator);
  *init_options = rmw_get_zero_initialized_init_options();
  return RMW_RET_OK;
}

}  // namespace rmw_fastrtps_shared_cpp
