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

#include <fastcdr/exceptions/Exception.h>

#include <string>

#include "rmw/error_handling.h"

#include "type_support_common.hpp"

namespace rmw_fastrtps_cpp
{

TypeSupport::TypeSupport(
  const rosidl_message_type_support_t * type_supports
)
: rmw_fastrtps_shared_cpp::TypeSupport(type_supports)
{
  is_compute_key_provided = false;
  max_size_bound_ = false;
  is_plain_ = false;
  key_is_unbounded_ = false;
  key_max_serialized_size_ = 0;
  members_ = nullptr;
  key_callbacks_ = nullptr;
  has_data_ = false;
}

void TypeSupport::set_members(const message_type_support_callbacks_t * members)
{
  members_ = members;

#ifdef ROSIDL_TYPESUPPORT_FASTRTPS_HAS_PLAIN_TYPES
  char bounds_info;
  auto data_size = static_cast<uint32_t>(members->max_serialized_size(bounds_info));
  max_size_bound_ = 0 != (bounds_info & ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE);
  is_plain_ = bounds_info == ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE;
#else
  is_plain_ = true;
  auto data_size = static_cast<uint32_t>(members->max_serialized_size(is_plain_));
  max_size_bound_ = is_plain_;
#endif

  // A plain message of size 0 is an empty message
  if (is_plain_ && (data_size == 0) ) {
    has_data_ = false;
    ++data_size;  // Dummy byte
  } else {
    has_data_ = true;
  }

  // Total size is encapsulation size + data size
  max_serialized_type_size = 4 + data_size;
  // Account for RTPS submessage alignment
  max_serialized_type_size = (max_serialized_type_size + 3) & ~3;

  if (nullptr != members->key_callbacks)
  {
    key_callbacks_ = members->key_callbacks;
    is_compute_key_provided = true;

    key_max_serialized_size_ = key_callbacks_->max_serialized_size_key(key_is_unbounded_);
    if (!key_is_unbounded_)
    {
      key_buffer_.reserve(key_max_serialized_size_);
    }
  }
}

size_t TypeSupport::getEstimatedSerializedSize(const void * ros_message, const void * impl) const
{
  if (is_plain_) {
    return max_serialized_type_size;
  }

  assert(ros_message);
  assert(impl);

  auto callbacks = static_cast<const message_type_support_callbacks_t *>(impl);

  // Encapsulation size + message size
  return 4 + callbacks->get_serialized_size(ros_message);
}

bool TypeSupport::serializeROSmessage(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, const void * impl) const
{
  assert(ros_message);
  assert(impl);

  // Serialize encapsulation
  ser.serialize_encapsulation();

  // If type is not empty, serialize message
  if (has_data_) {
    auto callbacks = static_cast<const message_type_support_callbacks_t *>(impl);
    return callbacks->cdr_serialize(ros_message, ser);
  }

  // Otherwise, add a dummy byte
  ser << (uint8_t)0;
  return true;
}

bool TypeSupport::deserializeROSmessage(
  eprosima::fastcdr::Cdr & deser, void * ros_message, const void * impl) const
{
  assert(ros_message);
  assert(impl);

  try {
    // Deserialize encapsulation.
    deser.read_encapsulation();

    // If type is not empty, deserialize message
    if (has_data_) {
      auto callbacks = static_cast<const message_type_support_callbacks_t *>(impl);
      return callbacks->cdr_deserialize(deser, ros_message);
    }

    // Otherwise, consume dummy byte
    uint8_t dump = 0;
    deser >> dump;
    (void)dump;
  } catch (const eprosima::fastcdr::exception::Exception &) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "Fast CDR exception deserializing message of type %s.",
      get_name().c_str());
    return false;
  } catch (const std::bad_alloc &) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "'Bad alloc' exception deserializing message of type %s.",
      get_name().c_str());
    return false;
  }

  return true;
}

bool TypeSupport::get_key_hash_from_ros_message(
    void * ros_message,
    eprosima::fastdds::rtps::InstanceHandle_t * ihandle,
    bool force_md5,
    const void * impl) const
{
  assert(ros_message);
  (void)impl;

  // retrieve estimated serialized size in case key is unbounded
  if (key_is_unbounded_)
  {
    key_max_serialized_size_ = (std::max) (
      key_max_serialized_size_,
      key_callbacks_->get_serialized_size_key(ros_message));
    key_buffer_.reserve(key_max_serialized_size_);
  }

  eprosima::fastcdr::FastBuffer fast_buffer(
    reinterpret_cast<char *>(key_buffer_.data()),
    key_max_serialized_size_);

  eprosima::fastcdr::Cdr ser(
    fast_buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::CdrVersion::XCDRv1);

  key_callbacks_->cdr_serialize_key(ros_message, ser);

  const size_t max_serialized_key_length = 16;

  auto ser_length = ser.get_serialized_data_length();

  // check for md5
  if (force_md5 || key_max_serialized_size_ > max_serialized_key_length)
  {
    md5_.init();
    md5_.update(key_buffer_.data(), static_cast<unsigned int>(ser_length));
    md5_.finalize();

    for (uint8_t i = 0; i < max_serialized_key_length; ++i)
    {
      ihandle->value[i] = md5_.digest[i];
    }
  }
  else
  {
    memset(ihandle->value, 0, max_serialized_key_length);
    for (uint8_t i = 0; i < ser_length; ++i)
    {
        ihandle->value[i] = key_buffer_[i];
    }
  }

  return true;
}

MessageTypeSupport::MessageTypeSupport(
  const message_type_support_callbacks_t * members,
  const rosidl_message_type_support_t * type_supports)
: rmw_fastrtps_cpp::TypeSupport(type_supports)
{
  assert(members);

  std::string name = _create_type_name(members);
  this->set_name(name.c_str());

  set_members(members);
}

ServiceTypeSupport::ServiceTypeSupport(const rosidl_message_type_support_t * type_supports)
: rmw_fastrtps_cpp::TypeSupport(type_supports)
{
}

RequestTypeSupport::RequestTypeSupport(
  const service_type_support_callbacks_t * members,
  const rosidl_message_type_support_t * type_supports)
: ServiceTypeSupport(type_supports)
{
  assert(members);

  auto msg = static_cast<const message_type_support_callbacks_t *>(
    members->request_members_->data);
  std::string name = _create_type_name(msg);  // + "Request_";
  this->set_name(name.c_str());

  set_members(msg);
}

ResponseTypeSupport::ResponseTypeSupport(
  const service_type_support_callbacks_t * members,
  const rosidl_message_type_support_t * type_supports)
: ServiceTypeSupport(type_supports)
{
  assert(members);

  auto msg = static_cast<const message_type_support_callbacks_t *>(
    members->response_members_->data);
  std::string name = _create_type_name(msg);  // + "Response_";
  this->set_name(name.c_str());

  set_members(msg);
}

}  // namespace rmw_fastrtps_cpp
