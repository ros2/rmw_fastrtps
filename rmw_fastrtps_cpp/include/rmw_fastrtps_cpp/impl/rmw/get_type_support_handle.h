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

#ifndef RMW_FASTRTPS_CPP__IMPL__RMW__GET_TYPE_SUPPORT_HANDLE_H_
#define RMW_FASTRTPS_CPP__IMPL__RMW__GET_TYPE_SUPPORT_HANDLE_H_

#include "rosidl_typesupport_introspection_cpp/MessageIntrospection.h"

namespace rmw
{

template<typename T>
const rosidl_message_type_support_t * get_type_support_handle()
{
  return rosidl_typesupport_introspection_cpp::get_type_support_handle<T>();
}

}  // namespace rmw

#endif  // RMW_FASTRTPS_CPP__IMPL__RMW__GET_TYPE_SUPPORT_HANDLE_H_
