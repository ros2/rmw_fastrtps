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

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "fastdds/rtps/common/Time_t.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "tracetools/tracetools.h"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_publish(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);

  (void) allocation;
  RMW_CHECK_FOR_NULL_WITH_MSG(
    publisher, "publisher handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher, publisher->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    ros_message, "ros message handle is null",
    return RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "publisher info pointer is null", return RMW_RET_ERROR);

  rmw_fastrtps_shared_cpp::SerializedData data;
  data.type = FASTRTPS_SERIALIZED_DATA_TYPE_ROS_MESSAGE;
  data.data = const_cast<void *>(ros_message);
  data.impl = info->type_support_impl_;
  eprosima::fastrtps::Time_t stamp;
  eprosima::fastrtps::Time_t::now(stamp);
  TRACETOOLS_TRACEPOINT(rmw_publish, publisher, ros_message, stamp.to_ns());
  if (!info->data_writer_->write_w_timestamp(&data, eprosima::fastdds::dds::HANDLE_NIL, stamp)) {
    RMW_SET_ERROR_MSG("cannot publish data");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_publish_serialized_message(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);

  (void) allocation;
  RMW_CHECK_FOR_NULL_WITH_MSG(
    publisher, "publisher handle is null",
    return RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher, publisher->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_FOR_NULL_WITH_MSG(
    serialized_message, "serialized message handle is null",
    return RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "publisher info pointer is null", return RMW_RET_ERROR);

  eprosima::fastcdr::FastBuffer buffer(
    reinterpret_cast<char *>(serialized_message->buffer), serialized_message->buffer_length);
  eprosima::fastcdr::Cdr ser(
    buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::CdrVersion::XCDRv1);
  ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);
  if (!ser.jump(serialized_message->buffer_length)) {
    RMW_SET_ERROR_MSG("cannot correctly set serialized buffer");
    return RMW_RET_ERROR;
  }

  rmw_fastrtps_shared_cpp::SerializedData data;
  data.type = FASTRTPS_SERIALIZED_DATA_TYPE_CDR_BUFFER;
  data.data = &ser;
  data.impl = nullptr;  // not used when type is FASTRTPS_SERIALIZED_DATA_TYPE_CDR_BUFFER
  eprosima::fastrtps::Time_t stamp;
  eprosima::fastrtps::Time_t::now(stamp);
  TRACETOOLS_TRACEPOINT(rmw_publish, publisher, serialized_message, stamp.to_ns());
  if (!info->data_writer_->write_w_timestamp(&data, eprosima::fastdds::dds::HANDLE_NIL, stamp)) {
    RMW_SET_ERROR_MSG("cannot publish data");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}
rmw_ret_t
__rmw_publish_loaned_message(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  static_cast<void>(allocation);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);

  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher, publisher->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (!publisher->can_loan_messages) {
    RMW_SET_ERROR_MSG("Loaning is not supported");
    return RMW_RET_UNSUPPORTED;
  }

  RMW_CHECK_ARGUMENT_FOR_NULL(ros_message, RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  eprosima::fastrtps::Time_t stamp;
  eprosima::fastrtps::Time_t::now(stamp);
  TRACETOOLS_TRACEPOINT(rmw_publish, publisher, ros_message, stamp.to_ns());
  if (!info->data_writer_->write_w_timestamp(
      const_cast<void *>(ros_message),
      eprosima::fastdds::dds::HANDLE_NIL, stamp))
  {
    RMW_SET_ERROR_MSG("cannot publish data");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
