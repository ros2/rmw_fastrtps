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


#ifndef QOS_CONVERTER_HPP_
#define QOS_CONVERTER_HPP_

namespace eprosima
{
namespace fastrtps
{
class ReaderQos;
class WriterQos;
}  // namespace fastrtps
}  // namespace eprosima

struct rmw_qos_profile_t;

/*
 * Converts the low-level QOS Policy; of type WriterQos or ReaderQos into rmw_qos_profile_t.
 * Since WriterQos or ReaderQos does not have information about history and depth, these values are not set
 * by this function.
 *
 * \param[in] dds_qos of type WriterQos or ReaderQos
 * \param[out] qos the equivalent of the data in WriterQos or ReaderQos in rmw_qos_profile_t
 */
template<typename DDSQoSPolicyT>
void
dds_qos_policy_to_rmw_qos(
  const DDSQoSPolicyT & dds_qos,
  rmw_qos_profile_t * qos);

extern template
void dds_qos_policy_to_rmw_qos<eprosima::fastrtps::WriterQos>(
  const eprosima::fastrtps::WriterQos & dds_qos,
  rmw_qos_profile_t * qos);

extern template
void dds_qos_policy_to_rmw_qos<eprosima::fastrtps::ReaderQos>(
  const eprosima::fastrtps::ReaderQos & dds_qos,
  rmw_qos_profile_t * qos);

#endif  // QOS_CONVERTER_HPP_
