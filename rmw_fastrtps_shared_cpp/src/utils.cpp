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

#include <atomic>
#include <cstdint>
#include <string>

#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TopicDescription.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include "fastrtps/types/TypesBase.h"

#include "rmw/rmw.h"

using ReturnCode_t = eprosima::fastrtps::types::ReturnCode_t;

const char * const CONTENT_FILTERED_TOPIC_POSTFIX = "_filtered_name";

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

  // NOTE(methylDragon): This only finds a type that's been previously registered to the participant
  *returned_type = participant_info->participant_->find_type(type_name);
  return true;
}

void
remove_topic_and_type(
  CustomParticipantInfo * participant_info,
  EventListenerInterface * event_listener,
  const eprosima::fastdds::dds::TopicDescription * topic_desc,
  const eprosima::fastdds::dds::TypeSupport & type)
{
  // TODO(MiguelCompany): We only create Topic instances at the moment, but this may
  // change in the future if we start supporting other kinds of TopicDescription
  // (like ContentFilteredTopic)
  auto topic = dynamic_cast<const eprosima::fastdds::dds::Topic *>(topic_desc);

  if (nullptr != topic) {
    participant_info->delete_topic(topic, event_listener);
  }

  if (type) {
    participant_info->participant_->unregister_type(type.get_type_name());
  }
}

bool
create_content_filtered_topic(
  eprosima::fastdds::dds::DomainParticipant * participant,
  eprosima::fastdds::dds::TopicDescription * topic_desc,
  const std::string & topic_name_mangled,
  const rmw_subscription_content_filter_options_t * options,
  eprosima::fastdds::dds::ContentFilteredTopic ** content_filtered_topic)
{
  std::vector<std::string> expression_parameters;
  for (size_t i = 0; i < options->expression_parameters.size; ++i) {
    expression_parameters.push_back(options->expression_parameters.data[i]);
  }

  auto topic = dynamic_cast<eprosima::fastdds::dds::Topic *>(topic_desc);
  static std::atomic<uint32_t> cft_counter{0};
  std::string cft_topic_name = topic_name_mangled + CONTENT_FILTERED_TOPIC_POSTFIX + "_" +
    std::to_string(cft_counter.fetch_add(1));
  eprosima::fastdds::dds::ContentFilteredTopic * filtered_topic =
    participant->create_contentfilteredtopic(
    cft_topic_name,
    topic,
    options->filter_expression,
    expression_parameters);
  if (filtered_topic == nullptr) {
    return false;
  }

  *content_filtered_topic = filtered_topic;
  return true;
}

bool
create_datareader(
  const eprosima::fastdds::dds::DataReaderQos & datareader_qos,
  const rmw_subscription_options_t * subscription_options,
  eprosima::fastdds::dds::Subscriber * subscriber,
  eprosima::fastdds::dds::TopicDescription * des_topic,
  CustomDataReaderListener * listener,
  eprosima::fastdds::dds::DataReader ** data_reader
)
{
  eprosima::fastdds::dds::DataReaderQos updated_qos = datareader_qos;
  switch (subscription_options->require_unique_network_flow_endpoints) {
    default:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_SYSTEM_DEFAULT:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_NOT_REQUIRED:
      // Unique network flow endpoints not required. We leave the decission to the XML profile.
      break;

    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED:
    case RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_STRICTLY_REQUIRED:
      // Ensure we request unique network flow endpoints
      using PropertyPolicyHelper = eprosima::fastrtps::rtps::PropertyPolicyHelper;
      if (nullptr ==
        PropertyPolicyHelper::find_property(
          updated_qos.properties(),
          "fastdds.unique_network_flows"))
      {
        updated_qos.properties().properties().emplace_back("fastdds.unique_network_flows", "");
      }
      break;
  }

  // Creates DataReader (with subscriber name to not change name policy)
  *data_reader = subscriber->create_datareader(
    des_topic,
    updated_qos,
    listener,
    eprosima::fastdds::dds::StatusMask::subscription_matched());
  if (!(*data_reader) &&
    (RMW_UNIQUE_NETWORK_FLOW_ENDPOINTS_OPTIONALLY_REQUIRED ==
    subscription_options->require_unique_network_flow_endpoints))
  {
    *data_reader = subscriber->create_datareader(
      des_topic,
      datareader_qos,
      listener,
      eprosima::fastdds::dds::StatusMask::subscription_matched());
  }

  if (!(*data_reader)) {
    return false;
  }

  return true;
}


}  // namespace rmw_fastrtps_shared_cpp
