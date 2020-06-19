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
#include "rmw_fastrtps_shared_cpp/rmw_init.hpp"
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
  return rmw_fastrtps_shared_cpp::rmw_init_options_init(
    eprosima_fastrtps_identifier, init_options, allocator);
}

rmw_ret_t
rmw_init_options_copy(const rmw_init_options_t * src, rmw_init_options_t * dst)
{
  return rmw_fastrtps_shared_cpp::rmw_init_options_copy(
    eprosima_fastrtps_identifier, src, dst);
}

rmw_ret_t
rmw_init_options_fini(rmw_init_options_t * init_options)
{
  return rmw_fastrtps_shared_cpp::rmw_init_options_fini(
    eprosima_fastrtps_identifier, init_options);
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

  std::unique_ptr<rmw_context_impl_t> context_impl(new (std::nothrow) rmw_context_impl_t());
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
  context_impl->is_shutdown = false;
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
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context->impl, RMW_RET_INVALID_ARGUMENT);
  context->impl->is_shutdown = true;
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
  if (!context->impl->is_shutdown) {
    RCUTILS_SET_ERROR_MSG("context has not been shutdown");
    return RMW_RET_INVALID_ARGUMENT;
  }
  delete context->impl;
  *context = rmw_get_zero_initialized_context();
  return RMW_RET_OK;
}
}  // extern "C"
