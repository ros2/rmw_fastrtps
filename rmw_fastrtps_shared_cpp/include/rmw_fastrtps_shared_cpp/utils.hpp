// Copyright 2021 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_
#define RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_

#include <mutex>
#include <string>

#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/topic/TopicDescription.hpp"
#include "fastdds/dds/topic/qos/TopicQos.hpp"

#include "fastrtps/types/TypesBase.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "rmw/rmw.h"


namespace rmw_fastrtps_shared_cpp
{

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
cast_error_dds_to_rmw(eprosima::fastrtps::types::ReturnCode_t);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
find_and_check_topic_and_type(
  const CustomParticipantInfo * participant_info,
  const std::string & topic_name,
  const std::string & type_name,
  eprosima::fastdds::dds::TopicDescription * & returned_topic,
  eprosima::fastdds::dds::TypeSupport & returned_type);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
void
remove_topic_and_type(
  const CustomParticipantInfo * participant_info,
  const eprosima::fastdds::dds::TopicDescription * topic,
  const eprosima::fastdds::dds::TypeSupport & type);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
eprosima::fastdds::dds::TopicDescription *
create_topic_rmw(
  const CustomParticipantInfo * participant_info,
  const std::string & topic_name,
  const std::string & type_name,
  const eprosima::fastdds::dds::TopicQos & qos);

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_
