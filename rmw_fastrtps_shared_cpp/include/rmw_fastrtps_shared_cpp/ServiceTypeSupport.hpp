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

#ifndef RMW_FASTRTPS_SHARED_CPP__SERVICETYPESUPPORT_HPP_
#define RMW_FASTRTPS_SHARED_CPP__SERVICETYPESUPPORT_HPP_

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>

#include "TypeSupport.hpp"

struct CustomServiceInfo;

namespace rmw_fastrtps_shared_cpp
{

template<typename MembersType>
class ServiceTypeSupport : public TypeSupport<MembersType>
{
protected:
  ServiceTypeSupport();
};

template<typename ServiceMembersType, typename MessageMembersType>
class RequestTypeSupport : public ServiceTypeSupport<MessageMembersType>
{
public:
  explicit RequestTypeSupport(const ServiceMembersType * members);
};

template<typename ServiceMembersType, typename MessageMembersType>
class ResponseTypeSupport : public ServiceTypeSupport<MessageMembersType>
{
public:
  explicit ResponseTypeSupport(const ServiceMembersType * members);
};

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__SERVICETYPESUPPORT_HPP_
