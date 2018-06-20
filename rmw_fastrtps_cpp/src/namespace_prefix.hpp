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

#ifndef NAMESPACE_PREFIX_HPP_
#define NAMESPACE_PREFIX_HPP_

#include <vector>
#include <string>

extern "C"
{
extern const char * const ros_topic_prefix;
extern const char * const ros_service_requester_prefix;
extern const char * const ros_service_response_prefix;

extern const std::vector<std::string> _ros_prefixes;
}  // extern "C"

/// Return the ROS specific prefix if it exists, otherwise "".
std::string
_get_ros_prefix_if_exists(const std::string & topic_name);

/// Returns the topic name stripped of and ROS specific prefix if exists.
std::string
_strip_ros_prefix_if_exists(const std::string & topic_name);
#endif  // NAMESPACE_PREFIX_HPP_
