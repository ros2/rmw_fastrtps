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

#include <vector>

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "fastdds/rtps/common/Time_t.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rcutils/logging_macros.h"

#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "buffer_backend_context.hpp"

#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"

#include "tracetools/tracetools.h"

namespace
{

void
create_pending_buffer_writers(CustomPublisherInfo * info)
{
  auto & state = *info->buffer_state_;
  std::vector<PendingBufferPublisher> pending;
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.pending.empty()) {
      return;
    }
    pending = std::move(state.pending);
    state.pending.clear();
  }

  std::vector<std::shared_ptr<BufferPublisherEndpoint>> new_endpoints;
  for (auto & p : pending) {
    auto endpoint = std::make_shared<BufferPublisherEndpoint>();
    endpoint->key = p.unique_topic;
    endpoint->target_subscriber_gid = p.target_subscriber_gid;
    endpoint->subscriber_endpoint_info = p.subscriber_endpoint_info;
    endpoint->backend_metadata = std::move(p.backend_metadata);

    eprosima::fastdds::dds::Topic * topic = nullptr;
    auto * existing_desc = info->participant_->lookup_topicdescription(p.unique_topic);
    if (existing_desc) {
      topic = dynamic_cast<eprosima::fastdds::dds::Topic *>(existing_desc);
      if (topic) {
        endpoint->owns_topic = false;
      }
    }
    if (!topic) {
      eprosima::fastdds::dds::TopicQos topic_qos = info->topic_->get_qos();
      topic = info->participant_->create_topic(
        p.unique_topic, info->type_support_.get_type_name(), topic_qos);
    }
    if (!topic) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Failed to create per-subscriber topic '%s'", p.unique_topic.c_str());
      continue;
    }
    endpoint->topic = topic;

    eprosima::fastdds::dds::DataWriterQos writer_qos = info->data_writer_->get_qos();
    writer_qos.publish_mode().kind = eprosima::fastdds::dds::SYNCHRONOUS_PUBLISH_MODE;
    writer_qos.endpoint().history_memory_policy =
      eprosima::fastdds::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
    writer_qos.data_sharing().off();

    {
      std::string main_gid_str =
        encode_endpoint_gid_for_user_data(info->publisher_gid, "PGID:");
      auto ud_vec = writer_qos.user_data().data_vec();
      ud_vec.insert(ud_vec.end(), main_gid_str.begin(), main_gid_str.end());
      writer_qos.user_data().setValue(ud_vec);
    }

    auto * data_writer = info->dds_publisher_->create_datawriter(
      topic, writer_qos, nullptr);
    if (!data_writer) {
      if (endpoint->owns_topic) {
        info->participant_->delete_topic(topic);
      }
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Failed to create per-subscriber DataWriter for '%s'", p.unique_topic.c_str());
      continue;
    }
    endpoint->data_writer = data_writer;

    RCUTILS_LOG_INFO_NAMED(
      "rmw_fastrtps_cpp",
      "Buffer publisher: created per-sub endpoint '%s'", p.unique_topic.c_str());
    RCUTILS_LOG_DEBUG_NAMED(
      "rmw_fastrtps_cpp",
      "Buffer publisher endpoint '%s': reliability=%d, durability=%d, history=%d depth=%d",
      p.unique_topic.c_str(),
      writer_qos.reliability().kind,
      writer_qos.durability().kind,
      writer_qos.history().kind,
      writer_qos.history().depth);
    new_endpoints.push_back(std::move(endpoint));
  }

  if (!new_endpoints.empty()) {
    std::lock_guard<std::mutex> lock(state.mutex);
    for (auto & ep : new_endpoints) {
      state.endpoints.push_back(std::move(ep));
    }
  }
}

/// Publish to buffer-aware channels.  Only called when all matched
/// subscribers are backend-aware (no legacy endpoints).
///
/// - Write once to the shared CPU channel for all CPU-only subscribers.
/// - Write per-endpoint to peer-to-peer channels for non-CPU subscribers.
void
publish_to_buffer_endpoints(
  CustomPublisherInfo * info,
  const void * ros_message,
  const rmw_publisher_t * publisher)
{
  create_pending_buffer_writers(info);

  auto callbacks = static_cast<const message_type_support_callbacks_t *>(info->type_support_impl_);
  auto & state = *info->buffer_state_;

  std::lock_guard<std::mutex> lock(state.mutex);

  eprosima::fastdds::dds::Time_t stamp;
  eprosima::fastdds::dds::Time_t::now(stamp);
  TRACETOOLS_TRACEPOINT(rmw_publish, publisher, ros_message, stamp.to_ns());

  // Publish to the shared CPU channel for all CPU-only subscribers.
  if (!state.cpu_only_subscribers.empty() && info->cpu_data_writer_) {
    rmw_fastrtps_shared_cpp::SerializedData cpu_data;
    cpu_data.type = rmw_fastrtps_shared_cpp::FASTDDS_SERIALIZED_DATA_TYPE_ROS_MESSAGE;
    cpu_data.data = const_cast<void *>(ros_message);
    cpu_data.impl = info->type_support_impl_;

    if (eprosima::fastdds::dds::RETCODE_OK !=
      info->cpu_data_writer_->write_w_timestamp(
        &cpu_data, eprosima::fastdds::dds::HANDLE_NIL, stamp))
    {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Failed to write to CPU channel DataWriter");
    }
  }

  // Publish to per-subscriber peer-to-peer endpoints for non-CPU subscribers.
  for (const auto & endpoint : state.endpoints) {
    uint32_t serialized_size = callbacks->get_serialized_size(ros_message);
    size_t buffer_size = serialized_size + 4;  // +4 for CDR encapsulation header
    std::vector<uint8_t> buffer_data(buffer_size);

    eprosima::fastcdr::FastBuffer fast_buffer(
      reinterpret_cast<char *>(buffer_data.data()), buffer_size);
    eprosima::fastcdr::Cdr ser(
      fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::CdrVersion::XCDRv1);
    ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

    auto * backend_context =
      static_cast<const rmw_fastrtps_cpp::BufferBackendContext *>(info->serialization_context_);
    if (!backend_context) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Buffer-aware serialize missing buffer backend context");
      continue;
    }
    bool ok = false;
    try {
      ok = callbacks->cdr_serialize_with_endpoint(
        ros_message, ser, endpoint->subscriber_endpoint_info,
        backend_context->serialization_context);
    } catch (const std::exception & e) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Buffer-aware serialize threw for endpoint '%s': %s",
        endpoint->key.c_str(), e.what());
      continue;
    }

    if (!ok) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Buffer-aware serialize failed for endpoint '%s'", endpoint->key.c_str());
      continue;
    }

    rmw_fastrtps_shared_cpp::SerializedData data;
    data.type = rmw_fastrtps_shared_cpp::FASTDDS_SERIALIZED_DATA_TYPE_CDR_BUFFER;
    data.data = &ser;
    data.impl = nullptr;

    if (eprosima::fastdds::dds::RETCODE_OK !=
      endpoint->data_writer->write_w_timestamp(
        &data, eprosima::fastdds::dds::HANDLE_NIL, stamp))
    {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Buffer-aware write failed for endpoint '%s'", endpoint->key.c_str());
    }
  }
}

}  // namespace

extern "C"
{
rmw_ret_t
rmw_publish(
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  (void) allocation;
  RMW_CHECK_FOR_NULL_WITH_MSG(
    publisher, "publisher handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher, publisher->implementation_identifier, eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros message handle is null",
    return RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  if (info->is_buffer_aware_) {
    // Check whether any non-backend-aware (legacy) subscribers exist.
    // If so, skip buffer channels entirely and fall through to the main
    // DataWriter so old RMW endpoints receive the message.  Buffer-aware
    // endpoints will also receive it via their main DataReader fallback.
    size_t total_matched = info->publisher_event_->subscription_count();
    size_t buffer_aware_count;
    {
      std::lock_guard<std::mutex> lock(info->buffer_state_->mutex);
      auto & st = *info->buffer_state_;
      buffer_aware_count =
        st.endpoints.size() + st.pending.size() + st.cpu_only_subscribers.size();
    }

    if (total_matched <= buffer_aware_count) {
      publish_to_buffer_endpoints(info, ros_message, publisher);
      return RMW_RET_OK;
    }
    // Legacy subscribers present — publish via main (legacy) DataWriter.
    return rmw_fastrtps_shared_cpp::__rmw_publish(
      eprosima_fastrtps_identifier, publisher, ros_message, allocation);
  }

  return rmw_fastrtps_shared_cpp::__rmw_publish(
    eprosima_fastrtps_identifier, publisher, ros_message, allocation);
}

rmw_ret_t
rmw_publish_serialized_message(
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_publish_serialized_message(
    eprosima_fastrtps_identifier, publisher, serialized_message, allocation);
}

rmw_ret_t
rmw_publish_loaned_message(
  const rmw_publisher_t * publisher,
  void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_publish_loaned_message(
    eprosima_fastrtps_identifier, publisher, ros_message, allocation);
}
}  // extern "C"
