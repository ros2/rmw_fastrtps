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
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "fastdds/dds/subscriber/SampleInfo.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"
#include "fastdds/dds/core/StackAllocatedSequence.hpp"
#include "fastdds/dds/core/condition/GuardCondition.hpp"
#include "fastdds/dds/core/status/StatusMask.hpp"
#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/topic/Topic.hpp"

#include "rcutils/logging_macros.h"

#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "buffer_backend_context.hpp"

#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"

#include "rcpputils/scope_exit.hpp"

namespace
{

class BufferDataReaderListener final : public eprosima::fastdds::dds::DataReaderListener
{
public:
  BufferDataReaderListener(
    eprosima::fastdds::dds::GuardCondition * guard,
    RMWSubscriptionEvent * event)
  : guard_(guard), event_(event) {}

  void on_data_available(eprosima::fastdds::dds::DataReader * reader) override
  {
    if (guard_) {
      guard_->set_trigger_value(true);
    }
    auto unread = reader->get_unread_count();
    if (event_) {
      event_->notify_buffer_data_available(unread > 0 ? static_cast<size_t>(unread) : 1);
    }
  }

private:
  eprosima::fastdds::dds::GuardCondition * guard_;
  RMWSubscriptionEvent * event_;
};

void
create_pending_buffer_readers(CustomSubscriberInfo * info)
{
  auto & state = *info->buffer_state_;
  std::vector<PendingBufferSubscription> pending;
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.pending.empty()) {
      return;
    }
    pending = std::move(state.pending);
    state.pending.clear();
  }

  std::vector<std::shared_ptr<BufferSubscriptionEndpoint>> new_endpoints;
  for (auto & p : pending) {
    auto endpoint = std::make_shared<BufferSubscriptionEndpoint>();
    endpoint->key = p.unique_topic;
    endpoint->publisher_gid = p.publisher_gid;
    endpoint->publisher_endpoint_info = p.publisher_endpoint_info;
    endpoint->backend_metadata = std::move(p.backend_metadata);

    eprosima::fastdds::dds::Topic * topic = nullptr;
    auto * existing_desc =
      info->dds_participant_->lookup_topicdescription(p.unique_topic);
    if (existing_desc) {
      topic = dynamic_cast<eprosima::fastdds::dds::Topic *>(existing_desc);
      if (topic) {
        endpoint->owns_topic = false;
      }
    }
    if (!topic) {
      eprosima::fastdds::dds::TopicQos topic_qos =
        info->dds_participant_->get_default_topic_qos();
      topic = info->dds_participant_->create_topic(
        p.unique_topic, info->type_support_.get_type_name(), topic_qos);
    }
    if (!topic) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Failed to create per-publisher topic '%s'", p.unique_topic.c_str());
      continue;
    }
    endpoint->topic = topic;

    eprosima::fastdds::dds::DataReaderQos reader_qos =
      info->subscriber_->get_default_datareader_qos();
    reader_qos.endpoint().history_memory_policy =
      eprosima::fastdds::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
    reader_qos.data_sharing().off();
    reader_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    reader_qos.history() = info->datareader_qos_.history();
    constexpr auto rep = eprosima::fastdds::dds::XCDR_DATA_REPRESENTATION;
    reader_qos.representation().clear();
    reader_qos.representation().m_value.push_back(rep);

    auto buffer_listener = std::make_shared<BufferDataReaderListener>(
      info->buffer_data_guard_.get(), info->subscription_event_);
    auto * data_reader = info->subscriber_->create_datareader(
      topic, reader_qos, buffer_listener.get(),
      eprosima::fastdds::dds::StatusMask::data_available());
    if (!data_reader) {
      if (endpoint->owns_topic) {
        info->dds_participant_->delete_topic(topic);
      }
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Failed to create per-publisher DataReader for '%s'", p.unique_topic.c_str());
      continue;
    }
    endpoint->data_reader = data_reader;
    endpoint->listener = buffer_listener;

    RCUTILS_LOG_INFO_NAMED(
      "rmw_fastrtps_cpp",
      "Buffer subscription: created per-pub endpoint '%s'", p.unique_topic.c_str());
    new_endpoints.push_back(std::move(endpoint));
  }

  if (!new_endpoints.empty()) {
    std::lock_guard<std::mutex> lock(state.mutex);
    for (auto & ep : new_endpoints) {
      state.endpoints.push_back(std::move(ep));
    }
  }
}

rmw_ret_t
take_buffer_aware(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  *taken = false;
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  auto callbacks = static_cast<const message_type_support_callbacks_t *>(
    info->type_support_impl_);

  create_pending_buffer_readers(info);

  auto & state = *info->buffer_state_;
  std::lock_guard<std::mutex> lock(state.mutex);

  for (const auto & endpoint : state.endpoints) {
    eprosima::fastcdr::FastBuffer receive_buffer;
    rmw_fastrtps_shared_cpp::SerializedData data;
    data.type = rmw_fastrtps_shared_cpp::FASTDDS_SERIALIZED_DATA_TYPE_CDR_BUFFER;
    data.data = &receive_buffer;
    data.impl = nullptr;

    eprosima::fastdds::dds::StackAllocatedSequence<void *, 1> data_values;
    const_cast<void **>(data_values.buffer())[0] = &data;
    eprosima::fastdds::dds::SampleInfoSeq info_seq{1};

    auto ret_code = endpoint->data_reader->take(data_values, info_seq, 1);
    if (ret_code != eprosima::fastdds::dds::RETCODE_OK) {
      continue;
    }

    auto reset = rcpputils::make_scope_exit(
      [&]() {
        data_values.length(0);
        info_seq.length(0);
      });

    if (!info_seq[0].valid_data) {
      continue;
    }

    eprosima::fastcdr::Cdr deser(
      receive_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
      eprosima::fastcdr::CdrVersion::XCDRv1);
    deser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);
    auto * backend_context =
      static_cast<const rmw_fastrtps_cpp::BufferBackendContext *>(info->serialization_context_);
    if (!backend_context) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Buffer-aware deserialize missing buffer backend context");
      continue;
    }
    bool deser_ok = false;
    try {
      deser_ok = callbacks->cdr_deserialize_with_endpoint(
        deser, ros_message, endpoint->publisher_endpoint_info,
        backend_context->serialization_context);
    } catch (const std::exception & e) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Buffer-aware deserialize threw for endpoint '%s': %s",
        endpoint->key.c_str(), e.what());
    }

    if (!deser_ok) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "Buffer-aware deserialize failed for endpoint '%s'", endpoint->key.c_str());
      continue;
    }

    *taken = true;
    if (message_info) {
      rmw_fastrtps_shared_cpp::_assign_message_info(
        eprosima_fastrtps_identifier, message_info, &info_seq[0]);
    }

    for (const auto & ep : state.endpoints) {
      if (ep->data_reader->get_unread_count() > 0) {
        info->buffer_data_guard_->set_trigger_value(true);
        break;
      }
    }

    return RMW_RET_OK;
  }

  return RMW_RET_OK;
}

}  // namespace

extern "C"
{
rmw_ret_t
rmw_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription, subscription->implementation_identifier, eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros message handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    taken, "taken handle is null",
    return RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  if (info->is_buffer_aware_) {
    rmw_ret_t ret = take_buffer_aware(subscription, ros_message, taken, nullptr);
    if (ret != RMW_RET_OK || *taken) {
      return ret;
    }
    // No data from buffer endpoints; fall back to the main DataReader for
    // messages published by non-buffer-aware publishers (e.g. cross-RMW).
    return rmw_fastrtps_shared_cpp::__rmw_take(
      eprosima_fastrtps_identifier, subscription, ros_message, taken, allocation);
  }

  return rmw_fastrtps_shared_cpp::__rmw_take(
    eprosima_fastrtps_identifier, subscription, ros_message, taken, allocation);
}

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription, subscription->implementation_identifier, eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  if (info->is_buffer_aware_) {
    rmw_ret_t ret = take_buffer_aware(subscription, ros_message, taken, message_info);
    if (ret != RMW_RET_OK || *taken) {
      return ret;
    }
    return rmw_fastrtps_shared_cpp::__rmw_take_with_info(
      eprosima_fastrtps_identifier, subscription, ros_message, taken, message_info, allocation);
  }

  return rmw_fastrtps_shared_cpp::__rmw_take_with_info(
    eprosima_fastrtps_identifier, subscription, ros_message, taken, message_info, allocation);
}

rmw_ret_t
rmw_take_sequence(
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_sequence(
    eprosima_fastrtps_identifier, subscription, count, message_sequence, message_info_sequence,
    taken, allocation);
}

rmw_ret_t
rmw_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_serialized_message(
    eprosima_fastrtps_identifier, subscription, serialized_message, taken, allocation);
}

rmw_ret_t
rmw_take_serialized_message_with_info(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_serialized_message_with_info(
    eprosima_fastrtps_identifier, subscription, serialized_message, taken, message_info,
    allocation);
}

rmw_ret_t
rmw_take_loaned_message(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  static_cast<void>(allocation);
  return rmw_fastrtps_shared_cpp::__rmw_take_loaned_message_internal(
    eprosima_fastrtps_identifier, subscription, loaned_message, taken, nullptr);
}

rmw_ret_t
rmw_take_loaned_message_with_info(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  static_cast<void>(allocation);
  RMW_CHECK_ARGUMENT_FOR_NULL(message_info, RMW_RET_INVALID_ARGUMENT);
  return rmw_fastrtps_shared_cpp::__rmw_take_loaned_message_internal(
    eprosima_fastrtps_identifier, subscription, loaned_message, taken, message_info);
}

rmw_ret_t
rmw_return_loaned_message_from_subscription(
  const rmw_subscription_t * subscription,
  void * loaned_message)
{
  return rmw_fastrtps_shared_cpp::__rmw_return_loaned_message_from_subscription(
    eprosima_fastrtps_identifier, subscription, loaned_message);
}

rmw_ret_t
rmw_take_event(
  const rmw_event_t * event_handle,
  void * event_info,
  bool * taken)
{
  return rmw_fastrtps_shared_cpp::__rmw_take_event(
    eprosima_fastrtps_identifier, event_handle, event_info, taken);
}
}  // extern "C"
