// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#include "rmw_fastrtps_shared_cpp/init_rmw_context_impl.hpp"

#include <cassert>

#include "rmw/error_handling.h"
#include "rmw/init.h"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/msg/participant_entities_info.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/participant.hpp"
#include "rmw_fastrtps_shared_cpp/publisher.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

#include "rmw_fastrtps_shared_cpp/listener_thread.hpp"

rmw_ret_t
rmw_fastrtps_shared_cpp::decrement_context_impl_ref_count(
  rmw_context_t * context)
{
  std::lock_guard<std::mutex> guard(context->impl->mutex);

  assert(context);
  assert(context->impl);
  assert(0u < context->impl->count);

  if (--context->impl->count > 0) {
    return RMW_RET_OK;
  }

  rmw_ret_t err = RMW_RET_OK;
  rmw_ret_t ret = RMW_RET_OK;
  rmw_error_string_t error_string;

  ret = rmw_fastrtps_shared_cpp::join_listener_thread(context);
  if (RMW_RET_OK != ret) {
    return ret;
  }

  auto common_context = static_cast<rmw_dds_common::Context *>(context->impl->common);
  auto participant_info = static_cast<CustomParticipantInfo *>(context->impl->participant_info);

  if (!common_context->graph_cache.remove_participant(common_context->gid)) {
    RMW_SAFE_FWRITE_TO_STDERR(
      RCUTILS_STRINGIFY(__function__) ":" RCUTILS_STRINGIFY(__line__) ": "
      "couldn't remove Participant gid from graph_cache when destroying Participant");
  }

  ret = rmw_fastrtps_shared_cpp::destroy_subscription(
    context->implementation_identifier,
    participant_info,
    common_context->sub);
  // Try to clean the other objects if the above failed.
  if (RMW_RET_OK != ret) {
    error_string = rmw_get_error_string();
    rmw_reset_error();
  }
  err = rmw_fastrtps_shared_cpp::destroy_publisher(
    context->implementation_identifier,
    participant_info,
    common_context->pub);
  if (RMW_RET_OK != ret && RMW_RET_OK != err) {
    // We can just return one error, log about the previous one.
    RMW_SAFE_FWRITE_TO_STDERR(
      RCUTILS_STRINGIFY(__function__) ":" RCUTILS_STRINGIFY(__LINE__)
      ": 'destroy_subscription' failed\n");
    ret = err;
    error_string = rmw_get_error_string();
    rmw_reset_error();
  }
  err = rmw_fastrtps_shared_cpp::destroy_participant(participant_info);
  if (RMW_RET_OK != ret && RMW_RET_OK != err) {
    RMW_SAFE_FWRITE_TO_STDERR(
      RCUTILS_STRINGIFY(__function__) ":" RCUTILS_STRINGIFY(__LINE__)
      ": 'destroy_publisher' failed\n");
    ret = err;
  } else if (RMW_RET_OK != ret) {
    RMW_SET_ERROR_MSG(error_string.str);
  }

  common_context->graph_cache.clear_on_change_callback();
  if (RMW_RET_OK != rmw_fastrtps_shared_cpp::__rmw_destroy_guard_condition(
      common_context->graph_guard_condition))
  {
    RMW_SAFE_FWRITE_TO_STDERR(
      RCUTILS_STRINGIFY(__function__) ":" RCUTILS_STRINGIFY(__line__) ": "
      "couldn't destroy graph_guard_condtion");
  }

  delete common_context;
  context->impl->common = nullptr;
  context->impl->participant_info = nullptr;
  return ret;
}
