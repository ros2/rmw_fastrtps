// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#include "gtest/gtest.h"

#include "rmw/qos_profiles.h"

#include "rmw_fastrtps_shared_cpp/names.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"

TEST(NamespaceTest, get_prefix) {
  EXPECT_EQ("", _get_ros_prefix_if_exists(""));
  EXPECT_EQ("", _get_ros_prefix_if_exists("not/a_ros_prefix"));
  for (const auto & prefix : _get_all_ros_prefixes()) {
    EXPECT_EQ("", _get_ros_prefix_if_exists(prefix));
    EXPECT_EQ("", _get_ros_prefix_if_exists(prefix + "_should_not_match"));
    EXPECT_EQ("", _get_ros_prefix_if_exists("th/is_should_not_match/" + prefix));
    EXPECT_EQ(prefix, _get_ros_prefix_if_exists(prefix + "/"));
    EXPECT_EQ(prefix, _get_ros_prefix_if_exists(prefix + "/should_match"));
  }
}

TEST(NamespaceTest, strip_prefix) {
  EXPECT_EQ("", _strip_ros_prefix_if_exists(""));
  EXPECT_EQ("/no_ros_prefix/test", _strip_ros_prefix_if_exists("/no_ros_prefix/test"));
  for (const auto & prefix : _get_all_ros_prefixes()) {
    EXPECT_EQ(prefix, _strip_ros_prefix_if_exists(prefix));
    EXPECT_EQ("/", _strip_ros_prefix_if_exists(prefix + "/"));
    EXPECT_EQ(
      prefix + "should_not_be_stripped",
      _strip_ros_prefix_if_exists(prefix + "should_not_be_stripped"));
    EXPECT_EQ(
      "th/is_should_not_be_stripped/" + prefix,
      _strip_ros_prefix_if_exists("th/is_should_not_be_stripped/" + prefix));
    EXPECT_EQ("/should_be_stripped", _strip_ros_prefix_if_exists(prefix + "/should_be_stripped"));
  }
}

TEST(NamespaceTest, resolve_prefix) {
  EXPECT_EQ("", _resolve_prefix("", ""));
  EXPECT_EQ("", _resolve_prefix("", "some_ros_prefix"));
  EXPECT_EQ("", _resolve_prefix("/test", "some_ros_prefix"));
  EXPECT_EQ("/test", _resolve_prefix("/test", ""));
  EXPECT_EQ("", _resolve_prefix("some_ros_prefix", "some_ros_prefix"));
  EXPECT_EQ("/", _resolve_prefix("some_ros_prefix/", "some_ros_prefix"));
  EXPECT_EQ(
    "/test_some_ros_prefix",
    _resolve_prefix("some_ros_prefix/test_some_ros_prefix", "some_ros_prefix"));
  EXPECT_EQ(
    "", _resolve_prefix("some_ros_prefix_test", "some_ros_prefix"));
  EXPECT_EQ(
    "", _resolve_prefix("this_ros_prefix/test/some_ros_prefix", "some_ros_prefix"));
}

TEST(NamespaceTest, name_mangling) {
  rmw_qos_profile_t qos_profile = rmw_qos_profile_unknown;
  qos_profile.avoid_ros_namespace_conventions = false;

#ifndef NDEBUG
  EXPECT_DEATH(_create_topic_name(nullptr, "", "", ""), "");

  EXPECT_DEATH(_create_topic_name(&qos_profile, "", nullptr, ""), "");
#endif

  EXPECT_STREQ(
    "some_ros_prefix/test__suffix", _create_topic_name(
      &qos_profile, "some_ros_prefix", "/test", "__suffix").c_str());

  EXPECT_STREQ(
    "/test__suffix", _create_topic_name(
      &qos_profile, nullptr, "/test", "__suffix").c_str());

  EXPECT_STREQ(
    "some_ros_prefix/test", _create_topic_name(
      &qos_profile, "some_ros_prefix", "/test", nullptr).c_str());

  qos_profile.avoid_ros_namespace_conventions = true;
  EXPECT_STREQ(
    "/test__suffix", _create_topic_name(
      &qos_profile, "some_ros_prefix", "/test", "__suffix").c_str());
}
