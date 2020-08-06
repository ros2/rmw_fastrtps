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
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
void
_assign_message_info(
  const char * identifier,
  rmw_message_info_t * message_info,
  const eprosima::fastrtps::SampleInfo_t * sinfo)
{
  message_info->source_timestamp = sinfo->sourceTimestamp.to_ns();
  message_info->received_timestamp = sinfo->receptionTimestamp.to_ns();
  rmw_gid_t * sender_gid = &message_info->publisher_gid;
  sender_gid->implementation_identifier = identifier;
  memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);

  rmw_fastrtps_shared_cpp::copy_from_fastrtps_guid_to_byte_array(
    sinfo->sample_identity.writer_guid(),
    sender_gid->data);
}

rmw_ret_t
_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void) allocation;
  *taken = false;

  if (subscription->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  eprosima::fastrtps::SampleInfo_t sinfo;

  rmw_fastrtps_shared_cpp::SerializedData data;
  data.is_cdr_buffer = false;
  data.data = ros_message;
  data.impl = info->type_support_impl_;
  if (info->subscriber_->takeNextData(&data, &sinfo)) {
    info->listener_->update_unread_count(info->subscriber_);

    if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
      if (message_info) {
        _assign_message_info(identifier, message_info, &sinfo);
      }
      *taken = true;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
_take_sequence(
  const char * identifier,
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  *taken = 0;
  bool taken_flag = false;
  rmw_ret_t ret = RMW_RET_OK;

  if (subscription->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  // Limit the upper bound of reads to the number unread at the beginning.
  // This prevents any samples that are added after the beginning of the
  // _take_sequence call from being read.
  auto unread_count = info->subscriber_->get_unread_count();
  if (unread_count < count) {
    count = unread_count;
  }

  for (size_t ii = 0; ii < count; ++ii) {
    taken_flag = false;
    ret = _take(
      identifier, subscription, message_sequence->data[*taken],
      &taken_flag, &message_info_sequence->data[*taken], allocation);

    if (ret != RMW_RET_OK) {
      break;
    }

    if (taken_flag) {
      (*taken)++;
    }
  }

  message_sequence->size = *taken;
  message_info_sequence->size = *taken;

  return ret;
}

rmw_ret_t
__rmw_take_event(
  const char * identifier,
  const rmw_event_t * event_handle,
  void * event_info,
  bool * taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(event_handle, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(event_info, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  *taken = false;

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    event handle,
    event_handle->implementation_identifier,
    identifier,
    return RMW_RET_ERROR);

  auto event = static_cast<CustomEventInfo *>(event_handle->data);
  if (event->getListener()->takeNextEvent(event_handle->event_type, event_info)) {
    *taken = true;
    return RMW_RET_OK;
  }

  return RMW_RET_ERROR;
}

rmw_ret_t
__rmw_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(taken, "boolean flag for taken is null", return RMW_RET_ERROR);

  return _take(identifier, subscription, ros_message, taken, nullptr, allocation);
}

rmw_ret_t
__rmw_take_sequence(
  const char * identifier,
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_sequence, "message_sequence pointer is null", return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info_sequence, "message_info_sequence pointer is null",
    return RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    taken, "size_t flag for count is null", return RMW_RET_INVALID_ARGUMENT);

  if (count > message_sequence->capacity) {
    RMW_SET_ERROR_MSG("Insufficient capacity in message_sequence");
    return RMW_RET_ERROR;
  }

  if (count > message_info_sequence->capacity) {
    RMW_SET_ERROR_MSG("Insufficient capacity in message_info_sequence");
    return RMW_RET_ERROR;
  }

  return _take_sequence(
    identifier, subscription, count, message_sequence, message_info_sequence,
    taken, allocation);
}

rmw_ret_t
__rmw_take_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(taken, "boolean flag for taken is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info, "message info pointer is null", return RMW_RET_ERROR);

  return _take(identifier, subscription, ros_message, taken, message_info, allocation);
}

rmw_ret_t
_take_serialized_message(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void) allocation;
  *taken = false;

  if (subscription->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  rmw_fastrtps_shared_cpp::SerializedData data;
  data.is_cdr_buffer = true;
  data.data = &buffer;
  data.impl = nullptr;    // not used when is_cdr_buffer is true
  if (info->subscriber_->takeNextData(&data, &sinfo)) {
    info->listener_->update_unread_count(info->subscriber_);

    if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
      auto buffer_size = static_cast<size_t>(buffer.getBufferSize());
      if (serialized_message->buffer_capacity < buffer_size) {
        auto ret = rmw_serialized_message_resize(serialized_message, buffer_size);
        if (ret != RMW_RET_OK) {
          return ret;  // Error message already set
        }
      }
      serialized_message->buffer_length = buffer_size;
      memcpy(serialized_message->buffer, buffer.getBuffer(), serialized_message->buffer_length);

      if (message_info) {
        _assign_message_info(identifier, message_info, &sinfo);
      }
      *taken = true;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_take_serialized_message(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(taken, "boolean flag for taken is null", return RMW_RET_ERROR);

  return _take_serialized_message(
    identifier, subscription, serialized_message, taken, nullptr, allocation);
}

rmw_ret_t
__rmw_take_serialized_message_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(taken, "boolean flag for taken is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info, "message info pointer is null", return RMW_RET_ERROR);

  return _take_serialized_message(
    identifier, subscription, serialized_message, taken, message_info, allocation);
}
}  // namespace rmw_fastrtps_shared_cpp
