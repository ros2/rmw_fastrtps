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

#ifndef RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_
#define RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_

#include <cctype>
#include <mutex>
#include <string>
#include <vector>

#include "fastdds/dds/topic/TopicDescription.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include "fastdds/dds/core/ReturnCode.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "rmw/rmw.h"


namespace rmw_fastrtps_shared_cpp
{

/// Ensure a content filter parameter is a valid DDS SQL literal.
///
/// Fast DDS requires content filter parameters to be parseable as DDS SQL
/// literals.  Bare strings like "hello" must be wrapped in single quotes to
/// become valid string literals ('hello'), otherwise
/// DDSFilterParameter::set_value() fails with a parse error.
/// See https://github.com/eProsima/Fast-DDS/issues/4199
inline std::string
ensure_dds_literal(const std::string & value)
{
  if (value.empty()) {
    return "'" + value + "'";
  }

  // Already a quoted string
  if ((value.front() == '\'' || value.front() == '`') && value.size() >= 2 &&
    value.back() == value.front())
  {
    return value;
  }

  // Boolean literals
  if (value == "TRUE" || value == "FALSE") {
    return value;
  }

  // Numeric: optional leading sign, then digit or dot (covers int, float, hex)
  const char * p = value.c_str();
  if (*p == '+' || *p == '-') {
    ++p;
  }
  if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
    return value;  // hex
  }
  if (std::isdigit(static_cast<unsigned char>(*p)) || *p == '.') {
    return value;  // numeric
  }

  // Not a recognized literal — wrap in single quotes
  return "'" + value + "'";
}

/// Convert rmw content filter parameters, auto-quoting bare strings for Fast DDS.
inline std::vector<std::string>
prepare_content_filter_parameters(const rmw_subscription_content_filter_options_t * options)
{
  std::vector<std::string> expression_parameters;
  for (size_t i = 0; i < options->expression_parameters.size; ++i) {
    expression_parameters.push_back(ensure_dds_literal(options->expression_parameters.data[i]));
  }
  return expression_parameters;
}

/**
* Convert a Fast DDS return code into the corresponding rmw_ret_t
* \param[in] code The Fast DDS return code to be translated
* \return the corresponding rmw_ret_t value
*/
RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
cast_error_dds_to_rmw(eprosima::fastdds::dds::ReturnCode_t code);

/**
* Tries to find already registered topic and type.
*
* \param[in]  participant_info CustomParticipantInfo associated to the context.
* \param[in]  topic_name       Name of the topic for the entity being created.
* \param[in]  type_name        Name of the type for the entity being created.
* \param[out] returned_topic   TopicDescription for topic_name
* \param[out] returned_type    TypeSupport for type_name
*
* \return false if `topic_name` was previously created with a different type name.
* \return true when there is no such conflict. Returned topic and type may be null
*              if they were not previously registered on the participant.
*/
RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
find_and_check_topic_and_type(
  const CustomParticipantInfo * participant_info,
  const std::string & topic_name,
  const std::string & type_name,
  eprosima::fastdds::dds::TopicDescription ** returned_topic,
  eprosima::fastdds::dds::TypeSupport * returned_type);

/**
* Performs removal of associated topic and type.
*
* \param[in] participant_info CustomParticipantInfo associated to the context.
* \param[in] event_listener   The EventListenerInterface associated with the topic.
* \param[in] topic            Topic of the entity being deleted.
* \param[in] type             TypeSupport of the entity being deleted.
*/
RMW_FASTRTPS_SHARED_CPP_PUBLIC
void
remove_topic_and_type(
  CustomParticipantInfo * participant_info,
  EventListenerInterface * event_listener,
  const eprosima::fastdds::dds::TopicDescription * topic,
  const eprosima::fastdds::dds::TypeSupport & type);

/**
* Create content filtered topic.
*
* \param[in]  participant             DomainParticipant where the topic will be created.
* \param[in]  topic_desc              TopicDescription returned by find_and_check_topic_and_type.
* \param[in]  topic_name_mangled      Mangled Name of the topic.
* \param[in]  options                 Options of the content filtered topic.
* \param[out] content_filtered_topic  Will hold the pointer to the content filtered topic along
                                      with the necessary information for its deletion.
*
* \return true when the content filtered topic was created
* \return false when the content filtered topic could not be created
*/
RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
create_content_filtered_topic(
  eprosima::fastdds::dds::DomainParticipant * participant,
  eprosima::fastdds::dds::TopicDescription * topic_desc,
  const std::string & topic_name_mangled,
  const rmw_subscription_content_filter_options_t * options,
  eprosima::fastdds::dds::ContentFilteredTopic ** content_filtered_topic);


/**
* Create data reader.
*
* \param[in]  datareader_qos         QoS of data reader.
* \param[in]  subscription_options   Options of the subscription.
* \param[in]  subscriber             A subsciber to create the data reader.
* \param[in]  des_topic              TopicDescription returned by find_and_check_topic_and_type.
* \param[in]  listener               The listener of the data reader.
* \param[out] data_reader            Will hold the pointer to the data reader.
*
* \return true when the data reader was created
* \return false when the data reader could not be created
*/
RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
create_datareader(
  const eprosima::fastdds::dds::DataReaderQos & datareader_qos,
  const rmw_subscription_options_t * subscription_options,
  eprosima::fastdds::dds::Subscriber * subscriber,
  eprosima::fastdds::dds::TopicDescription * des_topic,
  CustomDataReaderListener * listener,
  eprosima::fastdds::dds::DataReader ** data_reader);

/**
* Apply specific resource limits when using keys.
* Max samples per instance is set to history depth if KEEP_LAST
* else UNLIMITED.
*
* \param[in]       history_qos      History entitiy QoS.
* \param[in, out]  res_limits_qos   Resource limits entitiy QoS.
*
*/
RMW_FASTRTPS_SHARED_CPP_PUBLIC
void
apply_qos_resource_limits_for_keys(
  const eprosima::fastdds::dds::HistoryQosPolicy & history_qos,
  eprosima::fastdds::dds::ResourceLimitsQosPolicy & res_limits_qos);

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_
