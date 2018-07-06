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

#ifndef QOS_HPP_
#define QOS_HPP_

#include "rmw/rmw.h"

namespace eprosima
{
namespace fastrtps
{
class SubscriberAttributes;
class PublisherAttributes;
}  // namespace fastrtps
}  // namespace eprosima

extern "C"
{
RMW_LOCAL
bool
get_datareader_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastrtps::SubscriberAttributes & sattr);

RMW_LOCAL
bool
get_datawriter_qos(
  const rmw_qos_profile_t & qos_policies,
  eprosima::fastrtps::PublisherAttributes & pattr);
}
// extern "C"

#endif  // QOS_HPP_
