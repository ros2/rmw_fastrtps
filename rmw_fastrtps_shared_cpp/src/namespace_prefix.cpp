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

#include <string>
#include <vector>

#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"

extern "C"
{
const char * const ros_topic_prefix = "rt";
const char * const ros_service_requester_prefix = "rq";
const char * const ros_service_response_prefix = "rr";

const std::vector<std::string> _ros_prefixes =
{ros_topic_prefix, ros_service_requester_prefix, ros_service_response_prefix};
}  // extern "C"

/// Returns `name` stripped of `prefix`.
std::string
_resolve_prefix(const std::string & name, const std::string & prefix)
{
  if (name.rfind(prefix + "/", 0) == 0) {
    return name.substr(prefix.length());
  }
  return "";
}

/// Return the ROS specific prefix if it exists, otherwise "".
std::string
_get_ros_prefix_if_exists(const std::string & topic_name)
{
  for (const auto & prefix : _ros_prefixes) {
    if (topic_name.rfind(prefix + "/", 0) == 0) {
      return prefix;
    }
  }
  return "";
}

/// Strip the ROS specific prefix if it exists from the topic name.
std::string
_strip_ros_prefix_if_exists(const std::string & topic_name)
{
  for (const auto & prefix : _ros_prefixes) {
    if (topic_name.rfind(prefix + "/", 0) == 0) {
      return topic_name.substr(prefix.length());
    }
  }
  return topic_name;
}

/// Returns the list of ros prefixes
const std::vector<std::string> &
_get_all_ros_prefixes()
{
  return _ros_prefixes;
}
