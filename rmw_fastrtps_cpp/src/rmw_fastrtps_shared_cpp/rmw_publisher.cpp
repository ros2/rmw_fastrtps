// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/time.h"

#include "fastdds/dds/publisher/DataWriter.hpp"
#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/publisher.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_destroy_publisher(
  const char * identifier,
  const rmw_node_t * node,
  rmw_publisher_t * publisher)
{
  assert(node->implementation_identifier == identifier);
  assert(publisher->implementation_identifier == identifier);

  rmw_ret_t ret = RMW_RET_OK;
  rmw_error_state_t error_state;
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<const CustomPublisherInfo *>(publisher->data);

  // Update graph
  rmw_ret_t rmw_ret = common_context->remove_publisher_graph(
    info->publisher_gid,
    node->name, node->namespace_
  );
  if (RMW_RET_OK != rmw_ret) {
    error_state = *rmw_get_error_state();
    ret = rmw_ret;
    rmw_reset_error();
  }

  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  rmw_ret_t inner_ret = destroy_publisher(identifier, participant_info, publisher);
  if (RMW_RET_OK != inner_ret) {
    if (RMW_RET_OK != ret) {
      RMW_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
      RMW_SAFE_FWRITE_TO_STDERR(" during '" RCUTILS_STRINGIFY(__function__) "'\n");
    } else {
      error_state = *rmw_get_error_state();
      ret = inner_ret;
    }
    rmw_reset_error();
  }

  if (RMW_RET_OK != ret) {
    rmw_set_error_state(error_state.message, error_state.file, error_state.line_number);
  }
  return ret;
}

rmw_ret_t
__rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  auto info = static_cast<CustomPublisherInfo *>(publisher->data);

  *subscription_count = info->publisher_event_->subscription_count();

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_publisher_assert_liveliness(
  const char * identifier,
  const rmw_publisher_t * publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  if (nullptr == info) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }

  info->data_writer_->assert_liveliness();
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_publisher_wait_for_all_acked(
  const char * identifier,
  const rmw_publisher_t * publisher,
  rmw_time_t wait_timeout)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);

  const auto now = std::chrono::steady_clock::now();
  const auto total = std::chrono::nanoseconds(rmw_time_total_nsec(wait_timeout));
  const bool infinite = rmw_time_equal(wait_timeout, RMW_DURATION_INFINITE) ||
    total >= std::chrono::steady_clock::time_point::max() - now;
  const auto deadline = infinite ? now : now + total;

  // A sample published while waiting may have gone to a DataWriter that was already checked,
  // so check them all again until no sample was published during a pass.
  uint64_t published;
  do {
    published = info->publish_count_.load();

    // Buffer-aware samples are written by the buffer channel DataWriters instead of the
    // main one, so wait on them too.
    std::vector<eprosima::fastdds::dds::DataWriter *> writers{info->data_writer_};
    std::vector<std::shared_ptr<BufferPublisherEndpoint>> endpoints;
    if (info->is_buffer_aware_) {
      if (info->cpu_data_writer_) {
        writers.push_back(info->cpu_data_writer_);
      }
      std::lock_guard<std::mutex> lock(info->buffer_state_->mutex);
      endpoints = info->buffer_state_->endpoints;
    }
    for (const auto & endpoint : endpoints) {
      if (endpoint->data_writer) {
        writers.push_back(endpoint->data_writer);
      }
    }

    // Check every DataWriter without waiting first, so they all send their heartbeats at once,
    // and keep the ones that are not acknowledged yet.
    std::vector<eprosima::fastdds::dds::DataWriter *> pending;
    for (auto * writer : writers) {
      if (eprosima::fastdds::dds::RETCODE_OK !=
        writer->wait_for_acknowledgments(eprosima::fastdds::dds::Duration_t(0, 0)))
      {
        pending.push_back(writer);
      }
    }

    // Wait on the remaining ones until the deadline. Once it has passed, the remaining time is
    // zero, so the last DataWriters are still checked.
    for (auto * writer : pending) {
      eprosima::fastdds::dds::Duration_t timeout = eprosima::fastdds::dds::c_TimeInfinite;
      if (!infinite) {
        const auto remaining = std::max(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
            deadline - std::chrono::steady_clock::now()),
          std::chrono::nanoseconds(0));
        timeout = eprosima::fastdds::dds::Duration_t(
          static_cast<int32_t>(remaining.count() / 1000000000),
          static_cast<uint32_t>(remaining.count() % 1000000000));
      }
      if (eprosima::fastdds::dds::RETCODE_OK != writer->wait_for_acknowledgments(timeout)) {
        return RMW_RET_TIMEOUT;
      }
    }
  } while (info->publish_count_.load() != published);

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos)
{
  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  eprosima::fastdds::dds::DataWriter * fastdds_dw = info->data_writer_;
  const eprosima::fastdds::dds::DataWriterQos & dds_qos = fastdds_dw->get_qos();

  dds_qos_to_rmw_qos(dds_qos, qos);

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_borrow_loaned_message(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const rosidl_message_type_support_t * type_support,
  void ** ros_message)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher, publisher->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (!publisher->can_loan_messages) {
    RMW_SET_ERROR_MSG("Loaning is not supported");
    return RMW_RET_UNSUPPORTED;
  }

  RMW_CHECK_ARGUMENT_FOR_NULL(type_support, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);
  if (nullptr != *ros_message) {
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  if (eprosima::fastdds::dds::RETCODE_OK != info->data_writer_->loan_sample(*ros_message)) {
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_return_loaned_message_from_publisher(
  const char * identifier,
  const rmw_publisher_t * publisher,
  void * loaned_message)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher, publisher->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (!publisher->can_loan_messages) {
    RMW_SET_ERROR_MSG("Loaning is not supported");
    return RMW_RET_UNSUPPORTED;
  }

  RMW_CHECK_ARGUMENT_FOR_NULL(loaned_message, RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  if (eprosima::fastdds::dds::RETCODE_OK != info->data_writer_->discard_loan(loaned_message)) {
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
