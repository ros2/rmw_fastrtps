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

#include "qos.hpp"

#include "fastrtps/attributes/PublisherAttributes.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "rmw/error_handling.h"

extern "C"
{
bool
get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastrtps::SubscriberAttributes & sattr)
{
  switch (qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      sattr.topic.historyQos.kind = eprosima::fastrtps::KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      sattr.topic.historyQos.kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  switch (qos_policies.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      sattr.qos.m_reliability.kind = eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      sattr.qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  switch (qos_policies.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      sattr.qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      sattr.qos.m_durability.kind = eprosima::fastrtps::VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  if (qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
    sattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  // ensure the history depth is at least the requested queue size
  assert(sattr.topic.historyQos.depth >= 0);
  if (
    eprosima::fastrtps::KEEP_LAST_HISTORY_QOS == sattr.topic.historyQos.kind &&
    static_cast<size_t>(sattr.topic.historyQos.depth) < qos_policies.depth)
  {
    if (qos_policies.depth > (std::numeric_limits<int32_t>::max)()) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    sattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  return true;
}

bool
get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies, eprosima::fastrtps::PublisherAttributes & pattr)
{
  switch (qos_policies.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      pattr.topic.historyQos.kind = eprosima::fastrtps::KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      pattr.topic.historyQos.kind = eprosima::fastrtps::KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  switch (qos_policies.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      pattr.qos.m_durability.kind = eprosima::fastrtps::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      pattr.qos.m_durability.kind = eprosima::fastrtps::VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  switch (qos_policies.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      pattr.qos.m_reliability.kind = eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      pattr.qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  if (qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
    pattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  // ensure the history depth is at least the requested queue size
  assert(pattr.topic.historyQos.depth >= 0);
  if (
    eprosima::fastrtps::KEEP_LAST_HISTORY_QOS == pattr.topic.historyQos.kind &&
    static_cast<size_t>(pattr.topic.historyQos.depth) < qos_policies.depth)
  {
    if (qos_policies.depth > (std::numeric_limits<int32_t>::max)()) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    pattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
  }

  return true;
}
}  // extern "C"
