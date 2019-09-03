// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#include <cstddef>

#include "rosidl_generator_c/message_type_support_struct.h"
// rosidl_typesupport_cpp/message_type_support.hpp is installed by rosidl_generator_cpp
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rmw_dds_common/msg/node_entities_info__struct.hpp"
#include "rmw_dds_common/msg/node_entities_info__rosidl_typesupport_fastrtps_cpp.hpp"
#include "rmw_fastrtps_cpp/visibility_control.h"

namespace rosidl_typesupport_cpp
{

template<>
RMW_FASTRTPS_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<rmw_dds_common::msg::NodeEntitiesInfo>()
{
  return ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
    rosidl_typesupport_fastrtps_cpp, rmw_dds_common, msg, NodeEntitiesInfo)();
}

#ifdef __cplusplus
extern "C"
{
#endif

RMW_FASTRTPS_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(
  rosidl_typesupport_cpp, rmw_dds_common, msg, NodeEntitiesInfo)()
{
  return get_message_type_support_handle<rmw_dds_common::msg::NodeEntitiesInfo>();
}

#ifdef __cplusplus
}
#endif
}  // namespace rosidl_typesupport_cpp
