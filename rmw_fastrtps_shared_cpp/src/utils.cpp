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

#include <string>

#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TopicDescription.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include "fastrtps/types/TypesBase.h"

#include "rmw/rmw.h"

using ReturnCode_t = eprosima::fastrtps::types::ReturnCode_t;

namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t cast_error_dds_to_rmw(ReturnCode_t code)
{
  // not switch because it is not an enum class
  if (ReturnCode_t::RETCODE_OK == code) {
    return RMW_RET_OK;
  } else if (ReturnCode_t::RETCODE_ERROR == code) {
    // repeats the error to avoid too many 'if' comparisons
    return RMW_RET_ERROR;
  } else if (ReturnCode_t::RETCODE_TIMEOUT == code) {
    return RMW_RET_TIMEOUT;
  } else if (ReturnCode_t::RETCODE_UNSUPPORTED == code) {
    return RMW_RET_UNSUPPORTED;
  } else if (ReturnCode_t::RETCODE_BAD_PARAMETER == code) {
    return RMW_RET_INVALID_ARGUMENT;
  } else if (ReturnCode_t::RETCODE_OUT_OF_RESOURCES == code) {
    // Could be that out of resources comes from a different source than a bad allocation
    return RMW_RET_BAD_ALLOC;
  } else {
    return RMW_RET_ERROR;
  }
}

bool
cast_or_create_topic(
  eprosima::fastdds::dds::DomainParticipant * participant,
  eprosima::fastdds::dds::TopicDescription * desc,
  const std::string & topic_name,
  const std::string & type_name,
  const eprosima::fastdds::dds::TopicQos & topic_qos,
  bool is_writer_topic,
  TopicHolder * topic_holder)
{
  topic_holder->should_be_deleted = false;
  topic_holder->participant = participant;
  topic_holder->desc = desc;
  topic_holder->topic = nullptr;

  if (nullptr == desc) {
    topic_holder->topic = participant->create_topic(
      topic_name,
      type_name,
      topic_qos);

    if (!topic_holder->topic) {
      return false;
    }

    topic_holder->desc = topic_holder->topic;
    topic_holder->should_be_deleted = true;
  } else {
    if (is_writer_topic) {
      topic_holder->topic = dynamic_cast<eprosima::fastdds::dds::Topic *>(desc);
      assert(nullptr != topic_holder->topic);
    }
  }

  return true;
}

bool
find_and_check_topic_and_type(
  const CustomParticipantInfo * participant_info,
  const std::string & topic_name,
  const std::string & type_name,
  eprosima::fastdds::dds::TopicDescription ** returned_topic,
  eprosima::fastdds::dds::TypeSupport * returned_type)
{
  // Searchs for an already existing topic
  *returned_topic = participant_info->participant_->lookup_topicdescription(topic_name);
  if (nullptr != *returned_topic) {
    if ((*returned_topic)->get_type_name() != type_name) {
      return false;
    }
  }

  *returned_type = participant_info->participant_->find_type(type_name);
  return true;
}

void
remove_topic_and_type(
  const CustomParticipantInfo * participant_info,
  const eprosima::fastdds::dds::TopicDescription * topic_desc,
  const eprosima::fastdds::dds::TypeSupport & type)
{
  // TODO(MiguelCompany): We only create Topic instances at the moment, but this may
  // change in the future if we start supporting other kinds of TopicDescription
  // (like ContentFilteredTopic)
  auto topic = dynamic_cast<const eprosima::fastdds::dds::Topic *>(topic_desc);
  if (nullptr != topic) {
    participant_info->participant_->delete_topic(topic);
  }

  if (type) {
    participant_info->participant_->unregister_type(type.get_type_name());
  }
}

}  // namespace rmw_fastrtps_shared_cpp
