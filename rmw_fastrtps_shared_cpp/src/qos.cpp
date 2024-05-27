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
#include <vector>

#include "rcutils/logging_macros.h"

#include "rmw_fastrtps_shared_cpp/qos.hpp"

#include "fastdds/dds/publisher/qos/DataWriterQos.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/dds/topic/qos/TopicQos.hpp"

#include "rmw/error_handling.h"

#include "rmw_dds_common/qos.hpp"

#include "rosidl_runtime_c/type_hash.h"

#include "time_utils.hpp"

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

// Private function to encapsulate DataReader and DataWriter, together with Topic, filling
// entities DDS QoS from the RMW QoS profile.
template<typename DDSEntityQos>
bool fill_entity_qos_from_profile(
  const rmw_qos_profile_t & qos_policies,
  DDSEntityQos & entity_qos)
{
  switch (qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      entity_qos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      entity_qos.history().kind = eprosima::fastdds::dds::KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  switch (qos_policies.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      entity_qos.durability().kind = eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      entity_qos.durability().kind = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  switch (qos_policies.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      entity_qos.reliability().kind = eprosima::fastdds::dds::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      entity_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  // ensure the history depth is at least the requested queue size
  assert(entity_qos.history().depth >= 0);
  if (
    qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT &&
    static_cast<size_t>(entity_qos.history().depth) < qos_policies.depth)
  {
    if (qos_policies.depth > static_cast<size_t>((std::numeric_limits<int32_t>::max)())) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    entity_qos.history().depth = static_cast<int32_t>(qos_policies.depth);
  }

  if (!is_rmw_duration_unspecified(qos_policies.lifespan)) {
    entity_qos.lifespan().duration =
      rmw_fastrtps_shared_cpp::rmw_time_to_fastrtps(qos_policies.lifespan);
  }

  if (!is_rmw_duration_unspecified(qos_policies.deadline)) {
    entity_qos.deadline().period =
      rmw_fastrtps_shared_cpp::rmw_time_to_fastrtps(qos_policies.deadline);
  }

  switch (qos_policies.liveliness) {
    case RMW_QOS_POLICY_LIVELINESS_AUTOMATIC:
      entity_qos.liveliness().kind = eprosima::fastdds::dds::AUTOMATIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC:
      entity_qos.liveliness().kind = eprosima::fastdds::dds::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS Liveliness policy");
      return false;
  }
  if (!is_rmw_duration_unspecified(qos_policies.liveliness_lease_duration)) {
    entity_qos.liveliness().lease_duration =
      rmw_fastrtps_shared_cpp::rmw_time_to_fastrtps(qos_policies.liveliness_lease_duration);

    // Docs suggest setting no higher than 0.7 * lease_duration, choosing 2/3 to give safe buffer.
    // See doc at https://github.com/eProsima/Fast-RTPS/blob/
    //   a8691a40be6b8460b01edde36ad8563170a3a35a/include/fastrtps/qos/QosPolicies.h#L223-L232
    double period_in_ns = entity_qos.liveliness().lease_duration.to_ns() * 2.0 / 3.0;
    double period_in_s = RCUTILS_NS_TO_S(period_in_ns);
    entity_qos.liveliness().announcement_period = eprosima::fastrtps::Duration_t(period_in_s);
  }

  return true;
}

template<typename DDSEntityQos>
bool
fill_data_entity_qos_from_profile(
  const rmw_qos_profile_t & qos_policies,
  const rosidl_type_hash_t & type_hash,
  DDSEntityQos & entity_qos)
{
  if (!fill_entity_qos_from_profile(qos_policies, entity_qos)) {
    return false;
  }
  std::string user_data_str;
  if (RMW_RET_OK != rmw_dds_common::encode_type_hash_for_user_data_qos(type_hash, user_data_str)) {
    RCUTILS_LOG_WARN_NAMED(
      "rmw_fastrtps_shared_cpp",
      "Failed to encode type hash for topic, will not distribute it in USER_DATA.");
    user_data_str.clear();
    // Since we are going to go on without a hash, we clear the error so other
    // code won't overwrite it.
    rmw_reset_error();
  }
  std::vector<uint8_t> user_data(user_data_str.begin(), user_data_str.end());
  entity_qos.user_data().resize(user_data.size());
  entity_qos.user_data().setValue(user_data);
  return true;
}

bool
get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  const rosidl_type_hash_t & type_hash,
  eprosima::fastdds::dds::DataReaderQos & datareader_qos)
{
  if (fill_data_entity_qos_from_profile(qos_policies, type_hash, datareader_qos)) {
    // The type support in the RMW implementation is always XCDR1.
    constexpr auto rep = eprosima::fastdds::dds::XCDR_DATA_REPRESENTATION;
    datareader_qos.type_consistency().representation.clear();
    datareader_qos.type_consistency().representation.m_value.push_back(rep);
    return true;
  }

  return false;
}

bool
get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies,
  const rosidl_type_hash_t & type_hash,
  eprosima::fastdds::dds::DataWriterQos & datawriter_qos)
{
  if (fill_data_entity_qos_from_profile(qos_policies, type_hash, datawriter_qos)) {
    // The type support in the RMW implementation is always XCDR1.
    constexpr auto rep = eprosima::fastdds::dds::XCDR_DATA_REPRESENTATION;
    datawriter_qos.representation().clear();
    datawriter_qos.representation().m_value.push_back(rep);
    return true;
  }

  return false;
}

bool
get_topic_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastdds::dds::TopicQos & topic_qos)
{
  return fill_entity_qos_from_profile(qos_policies, topic_qos);
}

bool
is_valid_qos(const rmw_qos_profile_t & /* qos_policies */)
{
  return true;
}

template<typename AttributeT>
void
dds_attributes_to_rmw_qos(
  const AttributeT & dds_qos,
  rmw_qos_profile_t * qos)
{
  switch (dds_qos.topic.historyQos.kind) {
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
  qos->depth = static_cast<size_t>(dds_qos.topic.historyQos.depth);
  rtps_qos_to_rmw_qos(dds_qos.qos, qos);
}

template
void dds_attributes_to_rmw_qos<eprosima::fastrtps::PublisherAttributes>(
  const eprosima::fastrtps::PublisherAttributes & dds_qos,
  rmw_qos_profile_t * qos);

template
void dds_attributes_to_rmw_qos<eprosima::fastrtps::SubscriberAttributes>(
  const eprosima::fastrtps::SubscriberAttributes & dds_qos,
  rmw_qos_profile_t * qos);

template
void dds_qos_to_rmw_qos<eprosima::fastdds::dds::DataWriterQos>(
  const eprosima::fastdds::dds::DataWriterQos & dds_qos,
  rmw_qos_profile_t * qos);

template
void dds_qos_to_rmw_qos<eprosima::fastdds::dds::DataReaderQos>(
  const eprosima::fastdds::dds::DataReaderQos & dds_qos,
  rmw_qos_profile_t * qos);
