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

#include <mutex>
#include <string>

#include "fastdds/dds/topic/TopicDescription.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include "fastrtps/types/TypesBase.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

#include "rmw/rmw.h"


namespace rmw_fastrtps_shared_cpp
{

/**
* Convert a Fast DDS return code into the corresponding rmw_ret_t
* \param[in] code The Fast DDS return code to be translated
* \return the corresponding rmw_ret_t value
*/
RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
cast_error_dds_to_rmw(eprosima::fastrtps::types::ReturnCode_t code);

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

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_
