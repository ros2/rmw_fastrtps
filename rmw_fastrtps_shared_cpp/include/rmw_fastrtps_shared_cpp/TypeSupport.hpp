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

#ifndef RMW_FASTRTPS_SHARED_CPP__TYPESUPPORT_HPP_
#define RMW_FASTRTPS_SHARED_CPP__TYPESUPPORT_HPP_

#include <cassert>
#include <string>

#include "fastdds/dds/topic/TopicDataType.hpp"

#include "fastdds/rtps/common/InstanceHandle.h"
#include "fastdds/rtps/common/SerializedPayload.h"

#include "fastcdr/FastBuffer.h"
#include "fastcdr/Cdr.h"

#include "rcutils/logging_macros.h"

#include "rosidl_runtime_c/message_type_support_struct.h"

#include "./visibility_control.h"

namespace rmw_fastrtps_shared_cpp
{

// Publishers write method will receive a pointer to this struct
struct SerializedData
{
  bool is_cdr_buffer;  // Whether next field is a pointer to a Cdr or to a plain ros message
  void * data;
  const void * impl;   // RMW implementation specific data
};

class TypeSupport : public eprosima::fastdds::dds::TopicDataType
{
public:
  virtual size_t getEstimatedSerializedSize(const void * ros_message, const void * impl) const = 0;

  virtual bool serializeROSmessage(
    const void * ros_message, eprosima::fastcdr::Cdr & ser, const void * impl) const = 0;

  virtual bool deserializeROSmessage(
    eprosima::fastcdr::Cdr & deser, void * ros_message, const void * impl) const = 0;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool getKey(
    void * data,
    eprosima::fastrtps::rtps::InstanceHandle_t * ihandle,
    bool force_md5 = false) override
  {
    (void)data; (void)ihandle; (void)force_md5;
    return false;
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool serialize(void * data, eprosima::fastrtps::rtps::SerializedPayload_t * payload) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t * payload, void * data) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  std::function<uint32_t()> getSerializedSizeProvider(void * data) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void * createData() override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void deleteData(void * data) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  inline bool is_bounded() const
#ifdef TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED
  override
#endif
  {
    return max_size_bound_;
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  inline bool is_plain() const
#ifdef TOPIC_DATA_TYPE_API_HAS_IS_PLAIN
  override
#endif
  {
    return is_plain_;
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  virtual ~TypeSupport() {}

protected:
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  TypeSupport();

  bool max_size_bound_;
  bool is_plain_;
};

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool register_type_object(
  const rosidl_message_type_support_t * type_supports,
  const std::string & type_name);

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__TYPESUPPORT_HPP_
