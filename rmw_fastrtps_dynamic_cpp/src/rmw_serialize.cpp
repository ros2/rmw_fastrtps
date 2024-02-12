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

#include "fastcdr/FastBuffer.h"

#include "rmw/error_handling.h"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "./type_support_common.hpp"
#include "./type_support_registry.hpp"

extern "C"
{
rmw_ret_t
rmw_serialize(
  const void * ros_message,
  const rosidl_message_type_support_t * type_support,
  rmw_serialized_message_t * serialized_message)
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

  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto tss = type_registry.get_message_type_support(ts);
  auto data_length = tss->getEstimatedSerializedSize(ros_message, ts->data);
  if (serialized_message->buffer_capacity < data_length) {
    if (rmw_serialized_message_resize(serialized_message, data_length) != RMW_RET_OK) {
      RMW_SET_ERROR_MSG("unable to dynamically resize serialized message");
      type_registry.return_message_type_support(ts);
      return RMW_RET_ERROR;
    }
  }

  eprosima::fastcdr::FastBuffer buffer(
    reinterpret_cast<char *>(serialized_message->buffer),
    data_length);
  eprosima::fastcdr::Cdr ser(
    buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::CdrVersion::XCDRv1);
  ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

  auto ret = tss->serializeROSmessage(ros_message, ser, ts->data);
  serialized_message->buffer_length = data_length;
  serialized_message->buffer_capacity = data_length;
  type_registry.return_message_type_support(ts);
  return ret == true ? RMW_RET_OK : RMW_RET_ERROR;
}

rmw_ret_t
rmw_deserialize(
  const rmw_serialized_message_t * serialized_message,
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

  TypeSupportRegistry & type_registry = TypeSupportRegistry::get_instance();
  auto tss = type_registry.get_message_type_support(ts);
  eprosima::fastcdr::FastBuffer buffer(
    reinterpret_cast<char *>(serialized_message->buffer), serialized_message->buffer_length);
  eprosima::fastcdr::Cdr deser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN);

  auto ret = tss->deserializeROSmessage(deser, ros_message, ts->data);
  type_registry.return_message_type_support(ts);
  return ret == true ? RMW_RET_OK : RMW_RET_ERROR;
}

rmw_ret_t
rmw_get_serialized_message_size(
  const rosidl_message_type_support_t * /*type_support*/,
  const rosidl_runtime_c__Sequence__bound * /*message_bounds*/,
  size_t * /*size*/)
{
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"
