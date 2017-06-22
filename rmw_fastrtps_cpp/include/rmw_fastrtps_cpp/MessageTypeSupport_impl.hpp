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

#ifndef RMW_FASTRTPS_CPP__MESSAGETYPESUPPORT_IMPL_HPP_
#define RMW_FASTRTPS_CPP__MESSAGETYPESUPPORT_IMPL_HPP_

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>

#include <cassert>
#include <memory>
#include <string>

#include "rmw_fastrtps_cpp/MessageTypeSupport.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

namespace rmw_fastrtps_cpp
{

template<typename MembersType>
MessageTypeSupport<MembersType>::MessageTypeSupport(const MembersType * members)
{
  assert(members);
  this->members_ = members;

  std::string name = std::string(members->package_name_) + "::msg::dds_::" +
    members->message_name_ + "_";
  this->setName(name.c_str());

  // TODO(wjwwood): this could be more intelligent, setting m_typeSize to the
  // maximum serialized size of the message, when the message is a bounded one.
  if (members->member_count_ != 0) {
    this->m_typeSize = static_cast<uint32_t>(this->calculateMaxSerializedSize(members, 0));
  } else {
    this->m_typeSize = 4;
  }
}

}  // namespace rmw_fastrtps_cpp

#endif  // RMW_FASTRTPS_CPP__MESSAGETYPESUPPORT_IMPL_HPP_
