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

#ifndef RMW_FASTRTPS_CPP__SERVICETYPESUPPORT_IMPL_H_
#define RMW_FASTRTPS_CPP__SERVICETYPESUPPORT_IMPL_H_

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>
#include <string>

#include "rmw_fastrtps_cpp/ServiceTypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

namespace rmw_fastrtps_cpp
{

template<typename MembersType>
ServiceTypeSupport<MembersType>::ServiceTypeSupport()
{
}

template<typename ServiceMembersType, typename MessageMembersType>
RequestTypeSupport<ServiceMembersType, MessageMembersType>::RequestTypeSupport(
  const ServiceMembersType * members)
{
  assert(members);
  this->members_ = members->request_members_;

  std::string name = std::string(members->package_name_) + "::srv::dds_::" +
    members->service_name_ + "_Request_";
  this->setName(name.c_str());

  // TODO(wjwwood): this could be more intelligent, setting m_typeSize to the
  // maximum serialized size of the message, when the message is a bounded one.
  if (this->members_->member_count_ != 0) {
    this->m_typeSize = static_cast<uint32_t>(this->calculateMaxSerializedSize(this->members_, 0));
  } else {
    this->m_typeSize = 4;
  }
}

template<typename ServiceMembersType, typename MessageMembersType>
ResponseTypeSupport<ServiceMembersType, MessageMembersType>::ResponseTypeSupport(
  const ServiceMembersType * members)
{
  assert(members);
  this->members_ = members->response_members_;

  std::string name = std::string(members->package_name_) + "::srv::dds_::" +
    members->service_name_ + "_Response_";
  this->setName(name.c_str());

  // TODO(wjwwood): this could be more intelligent, setting m_typeSize to the
  // maximum serialized size of the message, when the message is a bounded one.
  if (this->members_->member_count_ != 0) {
    this->m_typeSize = static_cast<uint32_t>(this->calculateMaxSerializedSize(this->members_, 0));
  } else {
    this->m_typeSize = 4;
  }
}

}  // namespace rmw_fastrtps_cpp

#endif  // RMW_FASTRTPS_CPP__SERVICETYPESUPPORT_IMPL_H_
