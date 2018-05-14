// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
#include "rmw/raw_message.h"
#include "rmw/rmw.h"

#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "rmw_fastrtps_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/macros.hpp"

#include "./ros_message_serialization.hpp"

extern "C"
{
rmw_ret_t
rmw_take(const rmw_subscription_t * subscription, void * ros_message, bool * taken)
{
  RETURN_ERROR_ON_NULL(subscription);
  RETURN_ERROR_ON_NULL(ros_message);
  RETURN_ERROR_ON_NULL(subscription);

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RETURN_ERROR_ON_NULL(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
      eprosima::fastcdr::Cdr deser(
        buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR);
      _deserialize_ros_message(deser, ros_message, info->type_support_,
        info->typesupport_identifier_);
      *taken = true;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RETURN_ERROR_ON_NULL(subscription);
  RETURN_ERROR_ON_NULL(ros_message);
  RETURN_ERROR_ON_NULL(taken);
  RETURN_ERROR_ON_NULL(message_info);

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RETURN_ERROR_ON_NULL(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
      eprosima::fastcdr::Cdr deser(
        buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR);
      _deserialize_ros_message(deser, ros_message, info->type_support_,
        info->typesupport_identifier_);
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = eprosima_fastrtps_identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      memcpy(sender_gid->data, &sinfo.sample_identity.writer_guid(),
        sizeof(eprosima::fastrtps::rtps::GUID_t));
      *taken = true;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_take_raw(
  const rmw_subscription_t * subscription,
  rmw_message_raw_t * raw_message,
  bool * taken)
{
  RETURN_ERROR_ON_NULL(subscription);
  RETURN_ERROR_ON_NULL(raw_message);
  RETURN_ERROR_ON_NULL(taken);

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RETURN_ERROR_ON_NULL(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
      auto buffer_size = static_cast<unsigned int>(buffer.getBufferSize());
      if (raw_message->buffer_capacity < buffer_size) {
        auto ret = rmw_raw_message_resize(raw_message, buffer_size);
        if (ret != RMW_RET_OK) {
          return ret;  // Error message already set
        }
        fprintf(stderr, "had to resize to %u\n", buffer_size);
      }
      raw_message->buffer_length = buffer_size;
      // check for capacity and realloc if needed with allocator
      memcpy(raw_message->buffer, buffer.getBuffer(), raw_message->buffer_length);
      *taken = true;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_take_raw_with_info(
  const rmw_subscription_t * subscription,
  rmw_message_raw_t * raw_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RETURN_ERROR_ON_NULL(subscription);
  RETURN_ERROR_ON_NULL(raw_message);
  RETURN_ERROR_ON_NULL(taken);
  RETURN_ERROR_ON_NULL(message_info);

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RETURN_ERROR_ON_NULL(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
      auto buffer_size = static_cast<unsigned int>(buffer.getBufferSize());
      if (raw_message->buffer_capacity < buffer_size) {
        auto ret = rmw_raw_message_resize(raw_message, buffer_size);
        if (ret != RMW_RET_OK) {
          return ret;  // Error message already set
        }
      }
      raw_message->buffer_length = buffer_size;
      memcpy(raw_message->buffer, buffer.getBuffer(), raw_message->buffer_length);
      rmw_gid_t * sender_gid = &message_info->publisher_gid;
      sender_gid->implementation_identifier = eprosima_fastrtps_identifier;
      memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
      memcpy(sender_gid->data, &sinfo.sample_identity.writer_guid(),
        sizeof(eprosima::fastrtps::rtps::GUID_t));
      *taken = true;
    }
  }

  return RMW_RET_OK;
}
}  // extern "C"
