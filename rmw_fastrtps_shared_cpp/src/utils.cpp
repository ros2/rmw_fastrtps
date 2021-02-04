// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "rmw/rmw.h"
#include "fastrtps/types/TypesBase.h"

using ReturnCode_t = eprosima::fastrtps::types::ReturnCode_t;

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t cast_error_dds_to_rmw(ReturnCode_t code)
{
  // not switch because it is not an enum class
  if (code == ReturnCode_t::RETCODE_OK) {
    return RMW_RET_OK;
  } else if (code == ReturnCode_t::RETCODE_ERROR) {
    // repeats the error to avoid too much if comparations
    return RMW_RET_ERROR;
  } else if (code == ReturnCode_t::RETCODE_TIMEOUT) {
    return RMW_RET_TIMEOUT;
  } else if (code == ReturnCode_t::RETCODE_UNSUPPORTED) {
    return RMW_RET_UNSUPPORTED;
  } else if (code == ReturnCode_t::RETCODE_BAD_PARAMETER) {
    return RMW_RET_INVALID_ARGUMENT;
  } else if (code == ReturnCode_t::RETCODE_OUT_OF_RESOURCES) {
    // Could be that out of resources comes from a different source than a bad allocation
    return RMW_RET_BAD_ALLOC;
  } else {
    return RMW_RET_ERROR;
  }
}

eprosima::fastdds::dds::TopicDescription * create_topic_rmw(
  const CustomParticipantInfo * participant_info,
  const std::string & topic_name,
  const std::string & type_name,
  const eprosima::fastdds::dds::TopicQos & qos)
{
  // This block will lock the topic creations in this participant
  std::lock_guard<std::mutex> lck(participant_info->topic_creation_mutex_);

  // Searchs for an already existing topic
  eprosima::fastdds::dds::TopicDescription * des_topic =
    participant_info->participant_->lookup_topicdescription(topic_name);
  if (nullptr != des_topic) {
    // TODO the already existing topic type must fit with the desired type for new topic
    return des_topic;
  }

  // Creates topic
  eprosima::fastdds::dds::Topic * topic = participant_info->participant_->create_topic(
    topic_name,
    type_name,
    qos);
  participant_info->topics_list_.push_back(topic);
  return topic;
}

}  // namespace rmw_fastrtps_shared_cpp
