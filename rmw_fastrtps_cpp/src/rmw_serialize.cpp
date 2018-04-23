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

#include "fastcdr/FastBuffer.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "./type_support_common.hpp"
#include "./ros_message_serialization.hpp"

extern "C"
{

rmw_ret_t
rmw_serialize(
    const void * ros_message,
    const rosidl_message_type_support_t * type_support,
    rmw_message_raw_t * raw_message)
{
  const rosidl_message_type_support_t * ts = get_message_typesupport_handle(
      type_support, rosidl_typesupport_introspection_c__identifier);
  if (!ts) {
    ts = get_message_typesupport_handle(
        type_support, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!ts) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return RMW_RET_ERROR;
    }
  }

  auto tss = _create_message_type_support(ts->data, ts->typesupport_identifier);
  eprosima::fastcdr::FastBuffer buffer(raw_message->buffer, raw_message->buffer_capacity);
  eprosima::fastcdr::Cdr ser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR);
  auto ret = _serialize_ros_message(ros_message, ser, tss, ts->typesupport_identifier);
  raw_message->buffer_length = ser.getSerializedDataLength();
  raw_message->buffer_capacity = buffer.getBufferSize();

  return ret == true ? RMW_RET_OK : RMW_RET_ERROR;
}

rmw_ret_t
rmw_deserialize(
    const rmw_message_raw_t * raw_message,
    const rosidl_message_type_support_t * type_support,
    void * ros_message)
{
  const rosidl_message_type_support_t * ts = get_message_typesupport_handle(
      type_support, rosidl_typesupport_introspection_c__identifier);
  if (!ts) {
    ts = get_message_typesupport_handle(
        type_support, rosidl_typesupport_introspection_cpp::typesupport_identifier);
    if (!ts) {
      RMW_SET_ERROR_MSG("type support not from this implementation");
      return RMW_RET_ERROR;
    }
  }

  auto tss = _create_message_type_support(ts->data, ts->typesupport_identifier);
  eprosima::fastcdr::FastBuffer buffer(raw_message->buffer, raw_message->buffer_length);
  eprosima::fastcdr::Cdr deser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::Cdr::DDS_CDR);

  auto ret = _deserialize_ros_message(deser, ros_message, tss, ts->typesupport_identifier);
  const_cast<rmw_message_raw_t *>(raw_message)->buffer_length = deser.getSerializedDataLength();

  return ret == true ? RMW_RET_OK : RMW_RET_ERROR;
}

}  // extern "C"
