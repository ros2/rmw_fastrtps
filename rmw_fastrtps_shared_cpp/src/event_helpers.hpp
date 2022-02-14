// Copyright 2022 Open Source Robotics Foundation, Inc.
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

#ifndef EVENT_HELPERS_HPP_
#define EVENT_HELPERS_HPP_

#include "fastdds/dds/core/policy/QosPolicies.hpp"

#include "rmw/qos_policy_kind.h"

namespace rmw_fastrtps_shared_cpp
{
namespace internal
{

rmw_qos_policy_kind_t dds_qos_policy_to_rmw_qos_policy(
  eprosima::fastdds::dds::QosPolicyId_t policy_id);

}  // namespace internal
}  // namespace rmw_fastrtps_shared_cpp

#endif  // EVENT_HELPERS_HPP_
