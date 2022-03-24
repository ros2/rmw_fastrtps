// Copyright 2022 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef RMW_FASTRTPS_DYNAMIC_CPP__TYPE_OBJECT_HPP_
#define RMW_FASTRTPS_DYNAMIC_CPP__TYPE_OBJECT_HPP_

#include <string>

#include "rosidl_runtime_c/message_type_support_struct.h"

#include "rmw_fastrtps_dynamic_cpp/visibility_control.h"

namespace rmw_fastrtps_dynamic_cpp
{

RMW_FASTRTPS_DYNAMIC_CPP_PUBLIC
bool register_type_object(
  const rosidl_message_type_support_t * type_supports,
  const std::string & type_name);

}  // namespace rmw_fastrtps_dynamic_cpp
#endif  // RMW_FASTRTPS_DYNAMIC_CPP__TYPE_OBJECT_HPP_

