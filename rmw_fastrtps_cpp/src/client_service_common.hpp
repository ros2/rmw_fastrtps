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

#ifndef CLIENT_SERVICE_COMMON_HPP_
#define CLIENT_SERVICE_COMMON_HPP_

#include "rmw/error_handling.h"

template<typename ServiceType>
const void * get_request_ptr(const void * untyped_service_members)
{
  auto service_members = static_cast<const ServiceType *>(untyped_service_members);
  if (!service_members) {
    RMW_SET_ERROR_MSG("service members handle is null");
    return nullptr;
  }
  return service_members->request_members_;
}

const void * get_request_ptr(const void * untyped_service_members, const char * typesupport);

template<typename ServiceType>
const void * get_response_ptr(const void * untyped_service_members)
{
  auto service_members = static_cast<const ServiceType *>(untyped_service_members);
  if (!service_members) {
    RMW_SET_ERROR_MSG("service members handle is null");
    return nullptr;
  }
  return service_members->response_members_;
}

const void * get_response_ptr(const void * untyped_service_members, const char * typesupport);

#endif  // CLIENT_SERVICE_COMMON_HPP_
