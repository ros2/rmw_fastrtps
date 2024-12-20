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
#include "fastdds/rtps/common/InstanceHandle.hpp"
#include "fastdds/rtps/common/SerializedPayload.hpp"

#include "fastcdr/FastBuffer.h"
#include "fastcdr/Cdr.h"

#include "rcutils/logging_macros.h"

#include "rosidl_runtime_c/message_type_support_struct.h"

#include "./visibility_control.h"

namespace rmw_fastrtps_shared_cpp
{

enum SerializedDataType
{
  FASTDDS_SERIALIZED_DATA_TYPE_CDR_BUFFER,
  FASTDDS_SERIALIZED_DATA_TYPE_DYNAMIC_MESSAGE,
  FASTDDS_SERIALIZED_DATA_TYPE_ROS_MESSAGE
};

// Publishers write method will receive a pointer to this struct
struct SerializedData
{
  SerializedDataType type;  // The type of the next field
  void * data;
  const void * impl;  // RMW implementation specific data
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
  bool compute_key(
    const void * const data,
    eprosima::fastdds::rtps::InstanceHandle_t & ihandle,
    bool force_md5 = false) override
  {
    (void)data; (void)ihandle; (void)force_md5;
    return false;
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool compute_key(
    eprosima::fastdds::rtps::SerializedPayload_t & data,
    eprosima::fastdds::rtps::InstanceHandle_t & ihandle,
    bool force_md5 = false) override
  {
    (void)data; (void)ihandle; (void)force_md5;
    return false;
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool serialize(
    const void * const data,
    eprosima::fastdds::rtps::SerializedPayload_t & payload,
    eprosima::fastdds::dds::DataRepresentationId_t data_representation) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  bool deserialize(eprosima::fastdds::rtps::SerializedPayload_t & payload, void * data) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  uint32_t calculate_serialized_size(
    const void * const data,
    eprosima::fastdds::dds::DataRepresentationId_t data_representation)
  override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void * create_data() override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  void delete_data(void * data) override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  inline bool is_bounded() const
#ifdef TOPIC_DATA_TYPE_API_HAS_IS_BOUNDED
  override
#endif
  {
    return max_size_bound_;
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  inline bool is_plain(eprosima::fastdds::dds::DataRepresentationId_t rep) const override
  {
    return is_plain_ && rep == eprosima::fastdds::dds::XCDR_DATA_REPRESENTATION;
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC void register_type_object_representation() override;

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  inline const rosidl_message_type_support_t * ros_message_type_supports() const
  {
    return type_supports_;
  }

  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  virtual ~TypeSupport() {}

protected:
  RMW_FASTRTPS_SHARED_CPP_PUBLIC
  TypeSupport(
    const rosidl_message_type_support_t * type_supports
  );

  bool max_size_bound_ {false};
  bool is_plain_ {false};
  const rosidl_message_type_support_t * type_supports_ {nullptr};
};

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__TYPESUPPORT_HPP_
