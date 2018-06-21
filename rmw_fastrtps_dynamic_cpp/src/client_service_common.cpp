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

#include "client_service_common.hpp"
#include "type_support_common.hpp"

const void *
get_request_ptr(const void * untyped_service_members, const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return get_request_ptr<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_service_members);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return get_request_ptr<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_service_members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}

const void *
get_response_ptr(const void * untyped_service_members, const char * typesupport)
{
  if (using_introspection_c_typesupport(typesupport)) {
    return get_response_ptr<rosidl_typesupport_introspection_c__ServiceMembers>(
      untyped_service_members);
  } else if (using_introspection_cpp_typesupport(typesupport)) {
    return get_response_ptr<rosidl_typesupport_introspection_cpp::ServiceMembers>(
      untyped_service_members);
  }
  RMW_SET_ERROR_MSG("Unknown typesupport identifier");
  return nullptr;
}
