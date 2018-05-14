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

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/macros.hpp"

#include "./ros_message_serialization.hpp"

extern "C"
{
rmw_ret_t
rmw_publish(const rmw_publisher_t * publisher, const void * ros_message)
{
  RETURN_ERROR_ON_NULL(publisher);
  RETURN_ERROR_ON_NULL(ros_message);

  if (publisher->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  RETURN_ERROR_ON_NULL(info);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastcdr::Cdr ser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::Cdr::DDS_CDR);

  if (!_serialize_ros_message(ros_message, ser, info->type_support_,
    info->typesupport_identifier_))
  {
    RMW_SET_ERROR_MSG("cannot serialize data");
    return RMW_RET_ERROR;
  }
  if (!info->publisher_->write(&ser)) {
    RMW_SET_ERROR_MSG("cannot publish data");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publish_raw(const rmw_publisher_t * publisher, const rmw_message_raw_t * raw_message)
{
  RETURN_ERROR_ON_NULL(publisher);
  RETURN_ERROR_ON_NULL(raw_message);

  if (publisher->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("publisher handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  RETURN_ERROR_ON_NULL(info);

  eprosima::fastcdr::FastBuffer buffer(raw_message->buffer, raw_message->buffer_length);
  eprosima::fastcdr::Cdr ser(
    buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR);
  if (!ser.jump(raw_message->buffer_length)) {
    RMW_SET_ERROR_MSG("cannot correctly set raw buffer");
    return RMW_RET_ERROR;
  }

  if (!info->publisher_->write(&ser)) {
    RMW_SET_ERROR_MSG("cannot publish data");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}
}  // extern "C"
