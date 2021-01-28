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

#include <limits>

#include "rmw_fastrtps_shared_cpp/qos.hpp"

#include "fastdds/dds/publisher/DataWriter.hpp"
#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/rtps/common/Time_t.h"

#include "rmw/error_handling.h"
#include "rmw_dds_common/time_utils.hpp"

static
eprosima::fastrtps::Duration_t
rmw_time_to_fastrtps(const rmw_time_t & time)
{
  if (rmw_time_equal(time, RMW_DURATION_INFINITE)) {
    return eprosima::fastrtps::rtps::c_RTPSTimeInfinite.to_duration_t();
  }

  rmw_time_t clamped_time = rmw_dds_common::clamp_rmw_time_to_dds_time(time);
  return eprosima::fastrtps::Duration_t(
    static_cast<int32_t>(clamped_time.sec),
    static_cast<uint32_t>(clamped_time.nsec));
}

static
bool
is_rmw_duration_unspecified(const rmw_time_t & time)
{
  return rmw_time_equal(time, RMW_DURATION_UNSPECIFIED);
}

rmw_time_t
dds_duration_to_rmw(const eprosima::fastrtps::Duration_t & duration)
{
  if (duration == eprosima::fastrtps::rtps::c_RTPSTimeInfinite) {
    return RMW_DURATION_INFINITE;
  }
  rmw_time_t result = {(uint64_t)duration.seconds, (uint64_t)duration.nanosec};
  return result;
}

// Private function to encapsulate DataReader and DataWriter together with TopicQos fill entities
template<typename DDSEntityQos>
bool fill_entity_qos_from_profile(
  const rmw_qos_profile_t & qos_policies,
  DDSEntityQos & entity_qos)
{
  switch (qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      entity_qos.history().kind = eprosima::fastrtps::KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      entity_qos.history().kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  switch (qos_policies.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      entity_qos.durability().kind = eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      entity_qos.durability().kind = eprosima::fastrtps::VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  switch (qos_policies.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      entity_qos.reliability().kind = eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      entity_qos.reliability().kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  // ensure the history depth is at least the requested queue size
  assert(entity_qos.depth >= 0); // TODO eprosima should not be after initialization?
  if (
    qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT &&
    static_cast<size_t>(entity_qos.depth) < qos_policies.depth)
  {
    if (qos_policies.depth > static_cast<size_t>((std::numeric_limits<int32_t>::max)())) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    entity_qos.history().depth = static_cast<int32_t>(qos_policies.depth);
  }

  if (!is_rmw_duration_unspecified(qos_policies.lifespan)) {
    entity_qos.lifespan().duration = rmw_time_to_fastrtps(qos_policies.lifespan);
  }

  if (!is_rmw_duration_unspecified(qos_policies.deadline)) {
    entity_qos.deadline().period = rmw_time_to_fastrtps(qos_policies.deadline);
  }

  switch (qos_policies.liveliness) {
    case RMW_QOS_POLICY_LIVELINESS_AUTOMATIC:
      entity_qos.liveliness().kind = eprosima::fastrtps::AUTOMATIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC:
      entity_qos.liveliness().kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS Liveliness policy");
      return false;
  }
  if (!is_rmw_duration_unspecified(qos_policies.liveliness_lease_duration)) {
    entity_qos.liveliness().lease_duration =
      rmw_time_to_fastrtps(qos_policies.liveliness_lease_duration);

    // Docs suggest setting no higher than 0.7 * lease_duration, choosing 2/3 to give safe buffer.
    // See doc at https://github.com/eProsima/Fast-RTPS/blob/
    //   a8691a40be6b8460b01edde36ad8563170a3a35a/include/fastrtps/qos/QosPolicies.h#L223-L232
    double period_in_ns = entity_qos.liveliness().lease_duration.to_ns() * 2.0 / 3.0;
    double period_in_s = RCUTILS_NS_TO_S(period_in_ns);
    entity_qos.liveliness().announcement_period = eprosima::fastrtps::Duration_t(period_in_s);
  }

  return true;
}

bool
get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastdds::dds::DataReaderQos & datareader_qos)
{
  return fill_entity_qos_from_profile(qos_policies, datareader_qos);
}

bool
get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastdds::dds::DataWriterQos & datawriter_qos)
{
  return fill_entity_qos_from_profile(qos_policies, datawriter_qos);
}

bool
get_topic_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastdds::dds::TopicQos & topicQos)
{
  switch (qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      entity_qos.history().kind = eprosima::fastrtps::KEEP_LAST_HISTORY_QOS;
      topic_qos.history().kind = eprosima::fastrtps::KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      entity_qos.history().kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
      topic_qos.history().kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  // ensure the history depth is at least the requested queue size
  assert(topic_qos.depth >= 0); // TODO eprosima should not be after initialization?
  if (
    qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT &&
    static_cast<size_t>(topic_qos.depth) < qos_policies.depth)
  {
    if (qos_policies.depth > static_cast<size_t>((std::numeric_limits<int32_t>::max)())) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    topic_qos.history().depth = static_cast<int32_t>(qos_policies.depth);
  }
}

bool
is_valid_qos(const rmw_qos_profile_t & /* qos_policies */)
{
  return true;
}


template<typename DDSQoSPolicyT>
void
dds_qos_to_rmw_qos
  const DDSQoSPolicyT & dds_qos,
  rmw_qos_profile_t * qos)
{
  switch (dds_qos.history().kind) {
    case eprosima::fastrtps::KEEP_LAST_HISTORY_QOS:
      qos->history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
      break;
    case eprosima::fastrtps::KEEP_ALL_HISTORY_QOS:
      qos->history = RMW_QOS_POLICY_HISTORY_KEEP_ALL;
      break;
    default:
      qos->history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
      break;
  }
  qos->depth = static_cast<size_t>(dds_qos.history().depth);
  dds_qos_to_rmw_qos(dds_qos, qos);
}

// TODO eprosima
// template
// void dds_attributes_to_rmw_qos<eprosima::fastdds::dds::DataWriterQos>(
//   const eprosima::fastdds::dds::DataWriterQos & dds_qos,
//   rmw_qos_profile_t * qos);

// template
// void dds_attributes_to_rmw_qos<eprosima::fastdds::dds::DataReaderQos>(
//   const eprosima::fastdds::dds::DataReaderQos & dds_qos,
//   rmw_qos_profile_t * qos);
