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

#include <string>

#include "rmw/error_handling.h"

#include "type_support_common.hpp"

namespace rmw_fastrtps_cpp
{

TypeSupport::TypeSupport()
{
  m_isGetKeyDefined = false;
  max_size_bound_ = false;
}

void TypeSupport::set_members(const message_type_support_callbacks_t * members)
{
  members_ = members;

  // Fully bound by default
  max_size_bound_ = true;
  auto data_size = static_cast<uint32_t>(members->max_serialized_size(max_size_bound_));

  // A fully bound message of size 0 is an empty message
  if (max_size_bound_ && (data_size == 0) ) {
    has_data_ = false;
    ++data_size;  // Dummy byte
  } else {
    has_data_ = true;
  }

  // Total size is encapsulation size + data size
  m_typeSize = 4 + data_size;
}

size_t TypeSupport::getEstimatedSerializedSize(const void * ros_message)
{
  if (max_size_bound_) {
    return m_typeSize;
  }

  assert(ros_message);

  // Encapsulation size + message size
  return 4 + members_->get_serialized_size(ros_message);
}

bool TypeSupport::serializeROSmessage(const void * ros_message, eprosima::fastcdr::Cdr & ser)
{
  assert(ros_message);

  // Serialize encapsulation
  ser.serialize_encapsulation();

  // If type is not empty, serialize message
  if (has_data_) {
    return members_->cdr_serialize(ros_message, ser);
  }

  // Otherwise, add a dummy byte
  ser << (uint8_t)0;
  return true;
}

bool TypeSupport::deserializeROSmessage(eprosima::fastcdr::Cdr & deser, void * ros_message)
{
  assert(ros_message);

  // Deserialize encapsulation.
  deser.read_encapsulation();

  // If type is not empty, deserialize message
  if (has_data_) {
    return members_->cdr_deserialize(deser, ros_message);
  }

  // Otherwise, consume dummy byte
  uint8_t dump = 0;
  deser >> dump;
  (void)dump;

  return true;
}

MessageTypeSupport::MessageTypeSupport(const message_type_support_callbacks_t * members)
{
  assert(members);

  std::string name = _create_type_name(members);
  this->setName(name.c_str());

  set_members(members);
}

ServiceTypeSupport::ServiceTypeSupport()
{
}

RequestTypeSupport::RequestTypeSupport(const service_type_support_callbacks_t * members)
{
  assert(members);

  auto msg = static_cast<const message_type_support_callbacks_t *>(
    members->request_members_->data);
  std::string name = _create_type_name(msg);  // + "Request_";
  this->setName(name.c_str());

  set_members(msg);
}

ResponseTypeSupport::ResponseTypeSupport(const service_type_support_callbacks_t * members)
{
  assert(members);

  auto msg = static_cast<const message_type_support_callbacks_t *>(
    members->response_members_->data);
  std::string name = _create_type_name(msg);  // + "Response_";
  this->setName(name.c_str());

  set_members(msg);
}

}  // namespace rmw_fastrtps_cpp
