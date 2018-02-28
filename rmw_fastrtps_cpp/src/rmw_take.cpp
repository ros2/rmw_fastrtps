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
#include "rmw/rmw.h"

#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "fastcdr/FastBuffer.h"
#include "rmw_fastrtps_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_cpp/identifier.hpp"
#include "ros_message_serialization.hpp"

extern "C"
{
rmw_ret_t
rmw_take(const rmw_subscription_t * subscription, void * ros_message, bool * taken)
{
  assert(subscription);
  assert(ros_message);
  assert(taken);

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
      _deserialize_ros_message(&buffer, ros_message, info->type_support_,
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
  assert(subscription);
  assert(ros_message);
  assert(taken);

  if (!message_info) {
    RMW_SET_ERROR_MSG("message info is null");
    return RMW_RET_ERROR;
  }

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (eprosima::fastrtps::rtps::ALIVE == sinfo.sampleKind) {
      _deserialize_ros_message(&buffer, ros_message, info->type_support_,
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
  assert(subscription);
  assert(raw_message);
  assert(taken);

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (sinfo.sampleKind == ALIVE) {
      raw_message->buffer_length = buffer.getBufferSize();
      raw_message->buffer =
        reinterpret_cast<char *>(malloc(sizeof(char) * raw_message->buffer_length));
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
  assert(subscription);
  assert(raw_message);
  assert(taken);

  *taken = false;

  if (subscription->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CustomSubscriberInfo * info = static_cast<CustomSubscriberInfo *>(subscription->data);
  assert(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastrtps::SampleInfo_t sinfo;

  if (info->subscriber_->takeNextData(&buffer, &sinfo)) {
    info->listener_->data_taken();

    if (sinfo.sampleKind == ALIVE) {
      raw_message->buffer_length = buffer.getBufferSize();
      raw_message->buffer =
        reinterpret_cast<char *>(malloc(sizeof(char) * raw_message->buffer_length));
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
