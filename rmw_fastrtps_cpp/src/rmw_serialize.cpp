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

#include <memory>

#include "fastcdr/FastBuffer.h"

#include "rmw/error_handling.h"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "./type_support_common.hpp"

namespace
{

// Per-thread cache: resolves and constructs the MessageTypeSupport object once per unique
// type_support pointer, avoiding repeated get_message_typesupport_handle dispatch and
// MessageTypeSupport_cpp construction on every serialize/deserialize call.
struct TypeSupportCache
{
  const rosidl_message_type_support_t * input_ts = nullptr;
  const message_type_support_callbacks_t * callbacks = nullptr;
  std::unique_ptr<MessageTypeSupport_cpp> tss;
};

// Returns nullptr and sets the RMW error on failure.
const TypeSupportCache * get_type_support_cache(
  const rosidl_message_type_support_t * type_support)
{
  thread_local TypeSupportCache cache;

  if (cache.input_ts != type_support) {
    const rosidl_message_type_support_t * ts = get_message_typesupport_handle(
      type_support, RMW_FASTRTPS_CPP_TYPESUPPORT_C);
    if (!ts) {
      ts = get_message_typesupport_handle(
        type_support, RMW_FASTRTPS_CPP_TYPESUPPORT_CPP);
      if (!ts) {
        RMW_SET_ERROR_MSG("type support not from this implementation");
        return nullptr;
      }
    }
    cache.input_ts = type_support;
    cache.callbacks = static_cast<const message_type_support_callbacks_t *>(ts->data);
    cache.tss = std::make_unique<MessageTypeSupport_cpp>(cache.callbacks, type_support);
  }

  return &cache;
}

}  // namespace

extern "C"
{
rmw_ret_t
rmw_serialize(
  const void * ros_message,
  const rosidl_message_type_support_t * type_support,
  rmw_serialized_message_t * serialized_message)
{
  const TypeSupportCache * cache = get_type_support_cache(type_support);
  if (!cache) {
    return RMW_RET_ERROR;
  }

  auto data_length = cache->tss->getEstimatedSerializedSize(ros_message, cache->callbacks);
  if (serialized_message->buffer_capacity < data_length) {
    if (rmw_serialized_message_resize(serialized_message, data_length) != RMW_RET_OK) {
      rmw_reset_error();
      RMW_SET_ERROR_MSG("unable to dynamically resize serialized message");
      return RMW_RET_ERROR;
    }
  }

  eprosima::fastcdr::FastBuffer buffer(
    reinterpret_cast<char *>(serialized_message->buffer), data_length);
  eprosima::fastcdr::Cdr ser(
    buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::CdrVersion::XCDRv1);
  ser.set_encoding_flag(eprosima::fastcdr::EncodingAlgorithmFlag::PLAIN_CDR);

  auto ret = cache->tss->serializeROSmessage(ros_message, ser, cache->callbacks);
  serialized_message->buffer_length = data_length;
  serialized_message->buffer_capacity = data_length;
  return ret == true ? RMW_RET_OK : RMW_RET_ERROR;
}

rmw_ret_t
rmw_deserialize(
  const rmw_serialized_message_t * serialized_message,
  const rosidl_message_type_support_t * type_support,
  void * ros_message)
{
  const TypeSupportCache * cache = get_type_support_cache(type_support);
  if (!cache) {
    return RMW_RET_ERROR;
  }

  eprosima::fastcdr::FastBuffer buffer(
    reinterpret_cast<char *>(serialized_message->buffer), serialized_message->buffer_length);
  eprosima::fastcdr::Cdr deser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN);

  auto ret = cache->tss->deserializeROSmessage(deser, ros_message, cache->callbacks);
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
