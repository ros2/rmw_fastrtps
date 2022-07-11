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

#include <atomic>
#include <cassert>
#include <cstring>
#include <thread>

#include "rcutils/macros.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/init.h"
#include "rmw/ret_types.h"
#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/gid_utils.hpp"
#include "rmw_dds_common/msg/participant_entities_info.hpp"

#include "rmw_fastrtps_shared_cpp/listener_thread.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

using rmw_dds_common::operator<<;

static
void
node_listener(
  rmw_context_t * context);

rmw_ret_t
rmw_fastrtps_shared_cpp::run_listener_thread(
  rmw_context_t * context)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);

  auto common_context = static_cast<rmw_dds_common::Context *>(context->impl->common);
  common_context->thread_is_running.store(true);
  common_context->listener_thread_gc = rmw_fastrtps_shared_cpp::__rmw_create_guard_condition(
    context->implementation_identifier);
  if (common_context->listener_thread_gc) {
    try {
      common_context->listener_thread = std::thread(node_listener, context);
      return RMW_RET_OK;
    } catch (const std::exception & exc) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to create std::thread: %s", exc.what());
    } catch (...) {
      RMW_SET_ERROR_MSG("Failed to create std::thread");
    }
  } else {
    RMW_SET_ERROR_MSG("Failed to create guard condition");
  }
  common_context->thread_is_running.store(false);
  if (common_context->listener_thread_gc) {
    if (RMW_RET_OK != rmw_fastrtps_shared_cpp::__rmw_destroy_guard_condition(
        common_context->listener_thread_gc))
    {
      RCUTILS_SAFE_FWRITE_TO_STDERR(
        RCUTILS_STRINGIFY(__FILE__) ":" RCUTILS_STRINGIFY(__function__) ":"
        RCUTILS_STRINGIFY(__LINE__) ": Failed to destroy guard condition");
    }
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_fastrtps_shared_cpp::join_listener_thread(
  rmw_context_t * context)
{
  auto common_context = static_cast<rmw_dds_common::Context *>(context->impl->common);
  common_context->thread_is_running.exchange(false);
  rmw_ret_t rmw_ret = rmw_fastrtps_shared_cpp::__rmw_trigger_guard_condition(
    context->implementation_identifier, common_context->listener_thread_gc);
  if (RMW_RET_OK != rmw_ret) {
    return rmw_ret;
  }
  try {
    common_context->listener_thread.join();
  } catch (const std::exception & exc) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Failed to join std::thread: %s", exc.what());
    return RMW_RET_ERROR;
  } catch (...) {
    RMW_SET_ERROR_MSG("Failed to join std::thread");
    return RMW_RET_ERROR;
  }
  rmw_ret = rmw_fastrtps_shared_cpp::__rmw_destroy_guard_condition(
    common_context->listener_thread_gc);
  if (RMW_RET_OK != rmw_ret) {
    return rmw_ret;
  }
  return RMW_RET_OK;
}

#define LOG_THREAD_FATAL_ERROR(msg) \
  { \
    RCUTILS_SAFE_FWRITE_TO_STDERR( \
      RCUTILS_STRINGIFY(__FILE__) ":" RCUTILS_STRINGIFY(__function__) ":" \
      RCUTILS_STRINGIFY(__LINE__) RCUTILS_STRINGIFY(msg) \
      ": ros discovery info listener thread will shutdown ...\n"); \
  }

void
node_listener(
  rmw_context_t * context)
{
  assert(nullptr != context);
  assert(nullptr != context->impl);
  assert(nullptr != context->impl->common);
  auto common_context = static_cast<rmw_dds_common::Context *>(context->impl->common);
  // number of conditions of a subscription is 2
  rmw_wait_set_t * wait_set = rmw_fastrtps_shared_cpp::__rmw_create_wait_set(
    context->implementation_identifier, context, 2);
  if (nullptr == wait_set) {
    LOG_THREAD_FATAL_ERROR("failed to create waitset");
    return;
  }
  while (common_context->thread_is_running.load()) {
    assert(nullptr != common_context->sub);
    assert(nullptr != common_context->sub->data);
    void * subscriptions_buffer[] = {common_context->sub->data};
    void * guard_conditions_buffer[] = {common_context->listener_thread_gc->data};
    rmw_subscriptions_t subscriptions;
    rmw_guard_conditions_t guard_conditions;
    subscriptions.subscriber_count = 1;
    subscriptions.subscribers = subscriptions_buffer;
    guard_conditions.guard_condition_count = 1;
    guard_conditions.guard_conditions = guard_conditions_buffer;
    if (RMW_RET_OK != rmw_fastrtps_shared_cpp::__rmw_wait(
        context->implementation_identifier,
        &subscriptions,
        &guard_conditions,
        nullptr,
        nullptr,
        nullptr,
        wait_set,
        nullptr))
    {
      LOG_THREAD_FATAL_ERROR("rmw_wait failed");
      break;
    }
    if (subscriptions_buffer[0]) {
      rmw_dds_common::msg::ParticipantEntitiesInfo msg;
      bool taken = true;

      while (taken) {
        if (RMW_RET_OK != rmw_fastrtps_shared_cpp::__rmw_take(
            context->implementation_identifier,
            common_context->sub,
            static_cast<void *>(&msg),
            &taken,
            nullptr))
        {
          LOG_THREAD_FATAL_ERROR("__rmw_take failed");
          break;
        }
        if (taken) {
          if (std::memcmp(
              reinterpret_cast<char *>(common_context->gid.data),
              reinterpret_cast<char *>(&msg.gid.data),
              RMW_GID_STORAGE_SIZE) == 0)
          {
            // ignore local messages
            continue;
          }
          common_context->graph_cache.update_participant_entities(msg);
        }
      }
    }
  }
  if (RMW_RET_OK != rmw_fastrtps_shared_cpp::__rmw_destroy_wait_set(
      context->implementation_identifier, wait_set))
  {
    LOG_THREAD_FATAL_ERROR("failed to destroy waitset");
  }
}
