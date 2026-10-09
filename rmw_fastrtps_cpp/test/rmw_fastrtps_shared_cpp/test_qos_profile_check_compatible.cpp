// Copyright 2021 Open Source Robotics Foundation, Inc.
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

#include <gtest/gtest.h>

#include "rmw/qos_profiles.h"
#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

TEST(TestQoSProfileCheckCompatible, compatible)
{
  rmw_qos_profile_t qos_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_LAST,
    5,  // history depth
    RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT,
    RMW_QOS_POLICY_DURABILITY_VOLATILE,
    {1, 0},   // deadline
    {1, 0},   // lifespan
    RMW_QOS_POLICY_LIVELINESS_AUTOMATIC,
    {1, 0},   // liveliness lease duration
    false  // avoid_ros_namespace_conventions
  };

  rmw_qos_compatibility_type_t compatibility;
  rmw_ret_t ret = rmw_fastrtps_shared_cpp::__rmw_qos_profile_check_compatible(
    qos_profile,
    qos_profile,
    &compatibility,
    nullptr,
    0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatibility, RMW_QOS_COMPATIBILITY_OK);
}

TEST(TestQoSProfileCheckCompatible, incompatible)
{
  rmw_qos_profile_t pub_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_LAST,
    5,  // history depth
    RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT,
    RMW_QOS_POLICY_DURABILITY_VOLATILE,
    {1, 0},   // deadline
    {1, 0},   // lifespan
    RMW_QOS_POLICY_LIVELINESS_AUTOMATIC,
    {1, 0},   // liveliness lease duration
    false  // avoid_ros_namespace_conventions
  };

  rmw_qos_profile_t sub_profile = pub_profile;
  sub_profile.reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;

  rmw_qos_compatibility_type_t compatibility;
  rmw_ret_t ret = rmw_fastrtps_shared_cpp::__rmw_qos_profile_check_compatible(
    pub_profile,
    sub_profile,
    &compatibility,
    nullptr,
    0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatibility, RMW_QOS_COMPATIBILITY_ERROR);
}

TEST(TestQoSProfileCheckCompatible, warn_compatible)
{
  rmw_qos_profile_t pub_profile = {
    RMW_QOS_POLICY_HISTORY_KEEP_LAST,
    5,  // history depth
    RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT,
    RMW_QOS_POLICY_DURABILITY_VOLATILE,
    {1, 0},   // deadline
    {1, 0},   // lifespan
    RMW_QOS_POLICY_LIVELINESS_AUTOMATIC,
    {1, 0},   // liveliness lease duration
    false  // avoid_ros_namespace_conventions
  };

  rmw_qos_profile_t sub_profile = pub_profile;
  sub_profile.reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;

  rmw_qos_compatibility_type_t compatibility;
  rmw_ret_t ret = rmw_fastrtps_shared_cpp::__rmw_qos_profile_check_compatible(
    pub_profile,
    sub_profile,
    &compatibility,
    nullptr,
    0u);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(compatibility, RMW_QOS_COMPATIBILITY_WARNING);
}
