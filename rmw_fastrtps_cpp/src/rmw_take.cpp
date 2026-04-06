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
#include "fastdds/dds/core/StackAllocatedSequence.hpp"
#include "fastdds/dds/core/condition/GuardCondition.hpp"

#include "rcutils/logging_macros.h"

#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "fastdds/dds/builtin/topic/PublicationBuiltinTopicData.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "buffer_backend_context.hpp"

#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"

#include "rcpputils/scope_exit.hpp"

namespace
{

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

  // CPU-only path: take from the shared CPU channel DataReader.
  if (info->cpu_data_reader_) {
    rmw_fastrtps_shared_cpp::SerializedData cpu_recv_data;
    cpu_recv_data.type = rmw_fastrtps_shared_cpp::FASTDDS_SERIALIZED_DATA_TYPE_ROS_MESSAGE;
    cpu_recv_data.data = ros_message;
    cpu_recv_data.impl = info->type_support_impl_;

    eprosima::fastdds::dds::StackAllocatedSequence<void *, 1> cpu_vals;
    const_cast<void **>(cpu_vals.buffer())[0] = &cpu_recv_data;
    eprosima::fastdds::dds::SampleInfoSeq cpu_info_seq{1};

    auto cpu_ret = info->cpu_data_reader_->take(cpu_vals, cpu_info_seq, 1);
    if (cpu_ret == eprosima::fastdds::dds::RETCODE_OK && cpu_info_seq[0].valid_data) {
      *taken = true;
      if (message_info) {
        rmw_fastrtps_shared_cpp::_assign_message_info(
          eprosima_fastrtps_identifier, message_info, &cpu_info_seq[0]);
      }
      cpu_vals.length(0);
      cpu_info_seq.length(0);

      if (info->cpu_data_reader_->get_unread_count() > 0 && info->buffer_data_guard_) {
        info->buffer_data_guard_->set_trigger_value(true);
      }
      return RMW_RET_OK;
    }
    cpu_vals.length(0);
    cpu_info_seq.length(0);
  }

  // Accelerated path: single shared DataReader for all buffer-aware publishers.
  if (!info->accel_data_reader_) {
    return RMW_RET_OK;
  }

  eprosima::fastcdr::FastBuffer receive_buffer;
  rmw_fastrtps_shared_cpp::SerializedData data;
  data.type = rmw_fastrtps_shared_cpp::FASTDDS_SERIALIZED_DATA_TYPE_CDR_BUFFER;
  data.data = &receive_buffer;
  data.impl = nullptr;

  eprosima::fastdds::dds::StackAllocatedSequence<void *, 1> data_values;
  const_cast<void **>(data_values.buffer())[0] = &data;
  eprosima::fastdds::dds::SampleInfoSeq info_seq{1};

  auto ret_code = info->accel_data_reader_->take(data_values, info_seq, 1);
  if (ret_code != eprosima::fastdds::dds::RETCODE_OK) {
    return RMW_RET_OK;
  }

  auto reset = rcpputils::make_scope_exit(
    [&]() {
      data_values.length(0);
      info_seq.length(0);
    });

  if (!info_seq[0].valid_data) {
    return RMW_RET_OK;
  }

  // Look up publisher metadata by writer GID.
  rmw_gid_t writer_gid{};
  rmw_fastrtps_shared_cpp::copy_from_fastdds_guid_to_byte_array(
    info_seq[0].sample_identity.writer_guid(),
    writer_gid.data);
  std::string writer_hex = rmw_fastrtps_shared_cpp::gid_to_hex(writer_gid);

  rmw_topic_endpoint_info_t pub_endpoint_info = rmw_get_zero_initialized_topic_endpoint_info();
  pub_endpoint_info.endpoint_type = RMW_ENDPOINT_PUBLISHER;
  std::memcpy(pub_endpoint_info.endpoint_gid, writer_gid.data, RMW_GID_STORAGE_SIZE);

  {
    std::lock_guard<std::mutex> lock(info->buffer_state_->mutex);
    auto it = info->buffer_state_->publisher_metadata.find(writer_hex);
    if (it != info->buffer_state_->publisher_metadata.end()) {
      pub_endpoint_info = it->second.publisher_endpoint_info;
    }
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
    return RMW_RET_OK;
  }

  bool deser_ok = false;
  deser_ok = callbacks->cdr_deserialize_with_endpoint(
    deser, ros_message, pub_endpoint_info,
    backend_context->serialization_context);

  if (!deser_ok) {
    RCUTILS_LOG_ERROR_NAMED(
      "rmw_fastrtps_cpp",
      "Buffer-aware deserialize failed for publisher '%s'", writer_hex.c_str());
    return RMW_RET_OK;
  }

  *taken = true;
  if (message_info) {
    rmw_fastrtps_shared_cpp::_assign_message_info(
      eprosima_fastrtps_identifier, message_info, &info_seq[0]);

    eprosima::fastdds::rtps::PublicationBuiltinTopicData pub_data;
    if (eprosima::fastdds::dds::RETCODE_OK ==
      info->accel_data_reader_->get_matched_publication_data(
        pub_data, info_seq[0].publication_handle))
    {
      rmw_gid_t main_gid{};
      auto & ud = pub_data.user_data.data_vec();
      if (parse_endpoint_gid_from_user_data(
            ud.data(), ud.size(), "PGID:", main_gid))
      {
        std::memcpy(
          message_info->publisher_gid.data, main_gid.data, RMW_GID_STORAGE_SIZE);
      }
    }
  }

  if (info->accel_data_reader_->get_unread_count() > 0 && info->buffer_data_guard_) {
    info->buffer_data_guard_->set_trigger_value(true);
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
