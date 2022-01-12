// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__QOS_HPP_
#define RMW_FASTRTPS_SHARED_CPP__QOS_HPP_

#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/qos/TopicQos.hpp>

#include "fastrtps/qos/QosPolicies.h"

#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/visibility_control.h"

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
is_valid_qos(const rmw_qos_profile_t & qos_policies);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastdds::dds::DataReaderQos & reader_qos);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastdds::dds::DataWriterQos & writer_qos);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
get_topic_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastdds::dds::TopicQos & topic_qos);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_time_t
dds_duration_to_rmw(const eprosima::fastrtps::Duration_t & duration);

/*
 * Converts the DDS QOS Policy; of type DataWriterQos or DataReaderQos into rmw_qos_profile_t.
 *
 * \param[in] dds_qos of type DataWriterQos or DataReaderQos
 * \param[out] qos the equivalent of the data in dds_qos as a rmw_qos_profile_t
 */
template<typename DDSQoSPolicyT>
void
dds_qos_to_rmw_qos(
  const DDSQoSPolicyT & dds_qos,
  rmw_qos_profile_t * qos)
{
  switch (dds_qos.reliability().kind) {
    case eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
      break;
    case eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
      break;
    default:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
      break;
  }

  switch (dds_qos.durability().kind) {
    case eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
      break;
    case eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
      break;
    default:
      qos->durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
      break;
  }

  qos->deadline = dds_duration_to_rmw(dds_qos.deadline().period);
  qos->lifespan = dds_duration_to_rmw(dds_qos.lifespan().duration);

  switch (dds_qos.liveliness().kind) {
    case eprosima::fastdds::dds::AUTOMATIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
      break;
    case eprosima::fastdds::dds::MANUAL_BY_TOPIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
      break;
    default:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
      break;
  }

  qos->liveliness_lease_duration = dds_duration_to_rmw(dds_qos.liveliness().lease_duration);

  switch (dds_qos.history().kind) {
    case eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS:
      qos->history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
      break;
    case eprosima::fastdds::dds::KEEP_ALL_HISTORY_QOS:
      qos->history = RMW_QOS_POLICY_HISTORY_KEEP_ALL;
      break;
    default:
      qos->history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
      break;
  }
  qos->depth = static_cast<size_t>(dds_qos.history().depth);
}

/*
 * Converts the RTPS QOS Policy; of type WriterQos or ReaderQos into rmw_qos_profile_t.
 * Since WriterQos or ReaderQos do not have information about history and depth,
 * these values are not set by this function.
 *
 * \param[in] rtps_qos of type WriterQos or ReaderQos
 * \param[out] qos the equivalent of the data in rtps_qos as a rmw_qos_profile_t
 */
template<typename RTPSQoSPolicyT>
void
rtps_qos_to_rmw_qos(
  const RTPSQoSPolicyT & rtps_qos,
  rmw_qos_profile_t * qos)
{
  switch (rtps_qos.m_reliability.kind) {
    case eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
      break;
    case eprosima::fastrtps::RELIABLE_RELIABILITY_QOS:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
      break;
    default:
      qos->reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
      break;
  }

  switch (rtps_qos.m_durability.kind) {
    case eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
      break;
    case eprosima::fastrtps::VOLATILE_DURABILITY_QOS:
      qos->durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
      break;
    default:
      qos->durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
      break;
  }

  qos->deadline = dds_duration_to_rmw(rtps_qos.m_deadline.period);
  qos->lifespan = dds_duration_to_rmw(rtps_qos.m_lifespan.duration);

  switch (rtps_qos.m_liveliness.kind) {
    case eprosima::fastrtps::AUTOMATIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
      break;
    case eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
      break;
    default:
      qos->liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
      break;
  }
  qos->liveliness_lease_duration = dds_duration_to_rmw(rtps_qos.m_liveliness.lease_duration);
}

extern template RMW_FASTRTPS_SHARED_CPP_PUBLIC
void dds_qos_to_rmw_qos<eprosima::fastdds::dds::DataWriterQos>(
  const eprosima::fastdds::dds::DataWriterQos & dds_qos,
  rmw_qos_profile_t * qos);

extern template RMW_FASTRTPS_SHARED_CPP_PUBLIC
void dds_qos_to_rmw_qos<eprosima::fastdds::dds::DataReaderQos>(
  const eprosima::fastdds::dds::DataReaderQos & dds_qos,
  rmw_qos_profile_t * qos);


template<typename AttributeT>
void
dds_attributes_to_rmw_qos(
  const AttributeT & dds_qos,
  rmw_qos_profile_t * qos);

extern template RMW_FASTRTPS_SHARED_CPP_PUBLIC
void
dds_attributes_to_rmw_qos<eprosima::fastrtps::PublisherAttributes>(
  const eprosima::fastrtps::PublisherAttributes & dds_qos,
  rmw_qos_profile_t * qos);

extern template RMW_FASTRTPS_SHARED_CPP_PUBLIC
void
dds_attributes_to_rmw_qos<eprosima::fastrtps::SubscriberAttributes>(
  const eprosima::fastrtps::SubscriberAttributes & dds_qos,
  rmw_qos_profile_t * qos);

#endif  // RMW_FASTRTPS_SHARED_CPP__QOS_HPP_
