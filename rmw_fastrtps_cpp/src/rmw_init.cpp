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

#include <memory>

#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/init.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/init_options.h"
#include "rmw/publisher_options.h"
#include "rmw/rmw.h"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/msg/participant_entities_info.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/participant.hpp"
#include "rmw_fastrtps_shared_cpp/publisher.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"

#include "rosidl_typesupport_cpp/message_type_support.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/publisher.hpp"
#include "rmw_fastrtps_cpp/subscription.hpp"

extern "C"
{
rmw_ret_t
rmw_init_options_init(rmw_init_options_t * init_options, rcutils_allocator_t allocator)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);
  if (NULL != init_options->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected zero-initialized init_options");
    return RMW_RET_INVALID_ARGUMENT;
  }
  init_options->instance_id = 0;
  init_options->implementation_identifier = eprosima_fastrtps_identifier;
  init_options->allocator = allocator;
  init_options->impl = nullptr;
  init_options->name = rcutils_strdup("", allocator);
  if (!init_options->name) {
    RMW_SET_ERROR_MSG("failed to copy context name");
    return RMW_RET_BAD_ALLOC;
  }
  init_options->namespace_ = rcutils_strdup("", allocator);
  if (!init_options->namespace_) {
    allocator.deallocate(init_options->name, allocator.state);
    RMW_SET_ERROR_MSG("failed to copy context namespace");
    return RMW_RET_BAD_ALLOC;
  }
  return RMW_RET_OK;
}

rmw_ret_t
rmw_init_options_copy(const rmw_init_options_t * src, rmw_init_options_t * dst)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(src, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(dst, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    src,
    src->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (NULL != dst->implementation_identifier) {
    RMW_SET_ERROR_MSG("expected zero-initialized dst");
    return RMW_RET_INVALID_ARGUMENT;
  }
  const rcutils_allocator_t * allocator = &src->allocator;
  rmw_ret_t ret = RMW_RET_OK;

  *dst = *src;
  dst->name = NULL;
  dst->namespace_ = NULL;
  dst->security_options = rmw_get_zero_initialized_security_options();

  dst->name = rcutils_strdup(src->name, *allocator);
  if (!dst->name) {
    ret = RMW_RET_BAD_ALLOC;
    goto fail;
  }
  dst->namespace_ = rcutils_strdup(src->namespace_, *allocator);
  if (!dst->namespace_) {
    ret = RMW_RET_BAD_ALLOC;
    goto fail;
  }
  return rmw_security_options_copy(&src->security_options, allocator, &dst->security_options);
fail:
  allocator->deallocate(dst->name, allocator->state);
  allocator->deallocate(dst->namespace_, allocator->state);
  return ret;
}

rmw_ret_t
rmw_init_options_fini(rmw_init_options_t * init_options)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(init_options, RMW_RET_INVALID_ARGUMENT);
  rcutils_allocator_t & allocator = init_options->allocator;
  RCUTILS_CHECK_ALLOCATOR(&allocator, return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init_options,
    init_options->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  allocator.deallocate(init_options->name, allocator.state);
  allocator.deallocate(init_options->namespace_, allocator.state);
  rmw_security_options_fini(&init_options->security_options, &allocator);
  *init_options = rmw_get_zero_initialized_init_options();
  return RMW_RET_OK;
}

using rmw_dds_common::msg::ParticipantEntitiesInfo;

rmw_ret_t
rmw_init(const rmw_init_options_t * options, rmw_context_t * context)
{
  std::unique_ptr<rmw_context_t, void (*)(rmw_context_t *)> clean_when_fail(
    context,
    [](rmw_context_t * context)
    {
      *context = rmw_get_zero_initialized_context();
    });
  rmw_ret_t ret = RMW_RET_OK;

  RCUTILS_CHECK_ARGUMENT_FOR_NULL(options, RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    options,
    options->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  context->instance_id = options->instance_id;
  context->implementation_identifier = eprosima_fastrtps_identifier;

  std::unique_ptr<rmw_context_impl_t> context_impl(new rmw_context_impl_t());
  if (!context_impl) {
    RMW_SET_ERROR_MSG("failed to allocate context impl");
    return RMW_RET_BAD_ALLOC;
  }
  context->options = rmw_get_zero_initialized_init_options();
  ret = rmw_init_options_copy(options, &context->options);
  if (RMW_RET_OK != ret) {
    if (RMW_RET_OK != rmw_init_options_fini(&context->options)) {
      RMW_SAFE_FWRITE_TO_STDERR(
        "'rmw_init_options_fini' failed while being executed due to '"
        RCUTILS_STRINGIFY(__function__) "' failing.\n");
    }
    return ret;
  }
  context->impl = context_impl.release();
  clean_when_fail.release();
  return RMW_RET_OK;
}

rmw_ret_t
rmw_shutdown(rmw_context_t * context)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    context,
    context->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  // Nothing to do here for now.
  // This is just the middleware's notification that shutdown was called.
  return RMW_RET_OK;
}

rmw_ret_t
rmw_context_fini(rmw_context_t * context)
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    context,
    context->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  delete context->impl;
  *context = rmw_get_zero_initialized_context();
  return RMW_RET_OK;
}
}  // extern "C"
