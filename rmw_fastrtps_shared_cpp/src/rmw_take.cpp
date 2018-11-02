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

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
void
_assign_message_info(
  const char * identifier,
  rmw_message_info_t * message_info,
  const eprosima::fastrtps::SampleInfo_t * sinfo)
{
  rmw_gid_t * sender_gid = &message_info->publisher_gid;
  sender_gid->implementation_identifier = identifier;
  memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
  memcpy(sender_gid->data, &sinfo->sample_identity.writer_guid(),
    sizeof(eprosima::fastrtps::rtps::GUID_t));
}

rmw_ret_t
_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
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
  if (info->subscriber_->takeNextData(&data, &sinfo)) {
    info->listener_->data_taken();

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
__rmw_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(taken, "boolean flag for taken is null", return RMW_RET_ERROR);

  return _take(identifier, subscription, ros_message, taken, nullptr);
}

rmw_ret_t
__rmw_take_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(taken, "boolean flag for taken is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info, "message info pointer is null", return RMW_RET_ERROR);

  return _take(identifier, subscription, ros_message, taken, message_info);
}

rmw_ret_t
_take_serialized_message(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
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
  if (info->subscriber_->takeNextData(&data, &sinfo)) {
    info->listener_->data_taken();

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
  bool * taken)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(taken, "boolean flag for taken is null", return RMW_RET_ERROR);

  return _take_serialized_message(identifier, subscription, serialized_message, taken, nullptr);
}

rmw_ret_t
__rmw_take_serialized_message_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    subscription, "subscription pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "ros_message pointer is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(taken, "boolean flag for taken is null", return RMW_RET_ERROR);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    message_info, "message info pointer is null", return RMW_RET_ERROR);

  return _take_serialized_message(
    identifier, subscription, serialized_message, taken, message_info);
}
}  // namespace rmw_fastrtps_shared_cpp
