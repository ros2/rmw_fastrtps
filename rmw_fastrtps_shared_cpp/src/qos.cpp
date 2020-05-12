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

#include "fastrtps/attributes/PublisherAttributes.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "rmw/error_handling.h"

static
eprosima::fastrtps::Duration_t
rmw_time_to_fastrtps(const rmw_time_t & time)
{
  return eprosima::fastrtps::Duration_t(
    static_cast<int32_t>(time.sec),
    static_cast<uint32_t>(time.nsec));
}

static
bool
is_time_default(const rmw_time_t & time)
{
  return time.sec == 0 && time.nsec == 0;
}

template<typename DDSEntityQos>
bool fill_entity_qos_from_profile(
  const rmw_qos_profile_t & qos_policies,
  DDSEntityQos & entity_qos,
  eprosima::fastrtps::HistoryQosPolicy & history_qos)
{
  switch (qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      history_qos.kind = eprosima::fastrtps::KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      history_qos.kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  switch (qos_policies.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      entity_qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      entity_qos.m_durability.kind = eprosima::fastrtps::VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  switch (qos_policies.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      entity_qos.m_reliability.kind = eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      entity_qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  if (qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
    history_qos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  // ensure the history depth is at least the requested queue size
  assert(history_qos.depth >= 0);
  if (
    eprosima::fastrtps::KEEP_LAST_HISTORY_QOS == history_qos.kind &&
    static_cast<size_t>(history_qos.depth) < qos_policies.depth)
  {
    if (qos_policies.depth > static_cast<size_t>((std::numeric_limits<int32_t>::max)())) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    history_qos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  if (!is_time_default(qos_policies.lifespan)) {
    entity_qos.m_lifespan.duration = rmw_time_to_fastrtps(qos_policies.lifespan);
  }

  if (!is_time_default(qos_policies.deadline)) {
    entity_qos.m_deadline.period = rmw_time_to_fastrtps(qos_policies.deadline);
  }

  switch (qos_policies.liveliness) {
    case RMW_QOS_POLICY_LIVELINESS_AUTOMATIC:
      entity_qos.m_liveliness.kind = eprosima::fastrtps::AUTOMATIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC:
      entity_qos.m_liveliness.kind = eprosima::fastrtps::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS Liveliness policy");
      return false;
  }
  if (!is_time_default(qos_policies.liveliness_lease_duration)) {
    entity_qos.m_liveliness.lease_duration =
      rmw_time_to_fastrtps(qos_policies.liveliness_lease_duration);

    // Docs suggest setting no higher than 0.7 * lease_duration, choosing 2/3 to give safe buffer.
    // See doc at https://github.com/eProsima/Fast-RTPS/blob/
    //   a8691a40be6b8460b01edde36ad8563170a3a35a/include/fastrtps/qos/QosPolicies.h#L223-L232
    double period_in_ns = entity_qos.m_liveliness.lease_duration.to_ns() * 2.0 / 3.0;
    double period_in_s = RCUTILS_NS_TO_S(period_in_ns);
    entity_qos.m_liveliness.announcement_period = eprosima::fastrtps::Duration_t(period_in_s);
  }

  return true;
}

bool
get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastrtps::SubscriberAttributes & sattr)
{
  return fill_entity_qos_from_profile(qos_policies, sattr.qos, sattr.topic.historyQos);
}

bool
get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies, eprosima::fastrtps::PublisherAttributes & pattr)
{
  return fill_entity_qos_from_profile(qos_policies, pattr.qos, pattr.topic.historyQos);
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
  dds_qos_to_rmw_qos(dds_qos.qos, qos);
}

template
void dds_attributes_to_rmw_qos<eprosima::fastrtps::PublisherAttributes>(
  const eprosima::fastrtps::PublisherAttributes & dds_qos,
  rmw_qos_profile_t * qos);

template
void dds_attributes_to_rmw_qos<eprosima::fastrtps::SubscriberAttributes>(
  const eprosima::fastrtps::SubscriberAttributes & dds_qos,
  rmw_qos_profile_t * qos);
