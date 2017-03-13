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

#ifndef RMW_FASTRTPS_CPP__TYPESUPPORT_H_
#define RMW_FASTRTPS_CPP__TYPESUPPORT_H_

#include <rosidl_generator_c/string.h>
#include <rosidl_generator_c/string_functions.h>

#include <fastrtps/TopicDataType.h>

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>
#include <string>

namespace rmw_fastrtps_common
{

// Helper class that uses template specialization to read/write string types to/from a
// eprosima::fastcdr::Cdr
template<typename MembersType>
struct TypeSupportHelper;

template<typename MembersType>
class TypeSupport : public eprosima::fastrtps::TopicDataType
{
public:
  bool serializeROSmessage(const void * ros_message, eprosima::fastcdr::Cdr & ser);

  bool deserializeROSmessage(eprosima::fastcdr::FastBuffer * data, void * ros_message);

  bool serialize(void * data, SerializedPayload_t * payload);

  bool deserialize(SerializedPayload_t * payload, void * data);

  std::function<uint32_t()> getSerializedSizeProvider(void * data);

  void * createData();

  void deleteData(void * data);

protected:
  TypeSupport();

  size_t calculateMaxSerializedSize(const MembersType * members, size_t current_alignment);

  const MembersType * members_;

private:
  bool serializeROSmessage(eprosima::fastcdr::Cdr & ser, const MembersType * members,
    const void * ros_message);

  bool deserializeROSmessage(eprosima::fastcdr::Cdr & deser, const MembersType * members,
    void * ros_message, bool call_new);
};

}  // namespace rmw_fastrtps_common

#endif  // RMW_FASTRTPS_CPP__TYPESUPPORT_H_
