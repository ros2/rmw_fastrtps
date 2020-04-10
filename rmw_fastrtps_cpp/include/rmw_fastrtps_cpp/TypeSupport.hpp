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

#ifndef RMW_FASTRTPS_CPP__TYPESUPPORT_HPP_
#define RMW_FASTRTPS_CPP__TYPESUPPORT_HPP_

#include <rosidl_runtime_c/string.h>
#include <rosidl_runtime_c/string_functions.h>

#include <fastrtps/TopicDataType.h>

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>
#include <string>

#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_cpp
{

class TypeSupport : public rmw_fastrtps_shared_cpp::TypeSupport
{
public:
  size_t getEstimatedSerializedSize(const void * ros_message, const void * impl) const override;

  bool serializeROSmessage(
    const void * ros_message, eprosima::fastcdr::Cdr & ser, const void * impl) const override;

  bool deserializeROSmessage(
    eprosima::fastcdr::Cdr & deser, void * ros_message, const void * impl) const override;

protected:
  TypeSupport();

  void set_members(const message_type_support_callbacks_t * members);

private:
  const message_type_support_callbacks_t * members_;
  bool has_data_;
};

}  // namespace rmw_fastrtps_cpp

#endif  // RMW_FASTRTPS_CPP__TYPESUPPORT_HPP_
