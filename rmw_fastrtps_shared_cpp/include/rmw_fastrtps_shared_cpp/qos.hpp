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

#include "rmw/rmw.h"

#include "./visibility_control.h"

namespace eprosima
{
namespace fastrtps
{
class SubscriberAttributes;
class PublisherAttributes;
}  // namespace fastrtps
}  // namespace eprosima

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
is_valid_qos(const rmw_qos_profile_t & qos_policies);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastrtps::SubscriberAttributes & sattr);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool
get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastrtps::PublisherAttributes & pattr);

template<typename AttributeT>
void
dds_qos_to_rmw_qos(
  const AttributeT & dds_qos,
  rmw_qos_profile_t * qos);

extern template RMW_FASTRTPS_SHARED_CPP_PUBLIC
void
dds_qos_to_rmw_qos<eprosima::fastrtps::PublisherAttributes>(
  const eprosima::fastrtps::PublisherAttributes & dds_qos,
  rmw_qos_profile_t * qos);

extern template RMW_FASTRTPS_SHARED_CPP_PUBLIC
void
dds_qos_to_rmw_qos<eprosima::fastrtps::SubscriberAttributes>(
  const eprosima::fastrtps::SubscriberAttributes & dds_qos,
  rmw_qos_profile_t * qos);

#endif  // RMW_FASTRTPS_SHARED_CPP__QOS_HPP_
