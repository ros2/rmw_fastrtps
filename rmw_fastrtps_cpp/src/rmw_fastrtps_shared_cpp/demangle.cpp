// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#include <algorithm>
#include <string>
#include <vector>

#include "rcpputils/find_and_replace.hpp"
#include "rcutils/logging_macros.h"
#include "rcutils/types.h"

#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"

#include "demangle.hpp"

/// Return the demangle ROS topic or the original if not a ROS topic.
std::string
_demangle_if_ros_topic(const std::string & topic_name)
{
  return _strip_ros_prefix_if_exists(topic_name);
}

/// Return the demangled ROS type or the original if not a ROS type.
std::string
_demangle_if_ros_type(const std::string & dds_type_string)
{
  if (dds_type_string[dds_type_string.size() - 1] != '_') {
    // not a ROS type
    return dds_type_string;
  }

  std::string substring = "dds_::";
  size_t substring_position = dds_type_string.find(substring);
  if (substring_position == std::string::npos) {
    // not a ROS type
    return dds_type_string;
  }

  std::string type_namespace = dds_type_string.substr(0, substring_position);
  type_namespace = rcpputils::find_and_replace(type_namespace, "::", "/");
  size_t start = substring_position + substring.size();
  std::string type_name = dds_type_string.substr(start, dds_type_string.length() - 1 - start);
  return type_namespace + type_name;
}

/// Return the topic name for a given topic if it is part of one, else "".
std::string
_demangle_ros_topic_from_topic(const std::string & topic_name)
{
  return _resolve_prefix(topic_name, ros_topic_prefix);
}

/// Return the service name for a given topic if it is part of one, else "".
std::string
_demangle_service_from_topic(
  const std::string & prefix, const std::string & topic_name, std::string suffix)
{
  std::string service_name = _resolve_prefix(topic_name, prefix);
  if ("" == service_name) {
    return "";
  }

  size_t suffix_position = service_name.rfind(suffix);
  if (suffix_position != std::string::npos) {
    if (service_name.length() - suffix_position - suffix.length() != 0) {
      RCUTILS_LOG_WARN_NAMED(
        "rmw_fastrtps_shared_cpp",
        "service topic has service prefix and a suffix, but not at the end"
        ", report this: '%s'", topic_name.c_str());
      return "";
    }
  } else {
    RCUTILS_LOG_WARN_NAMED(
      "rmw_fastrtps_shared_cpp",
      "service topic has prefix but no suffix"
      ", report this: '%s'", topic_name.c_str());
    return "";
  }
  return service_name.substr(0, suffix_position);
}

std::string
_demangle_service_from_topic(const std::string & topic_name)
{
  const std::string demangled_topic = _demangle_service_reply_from_topic(topic_name);
  if ("" != demangled_topic) {
    return demangled_topic;
  }
  return _demangle_service_request_from_topic(topic_name);
}


std::string
_demangle_service_request_from_topic(const std::string & topic_name)
{
  return _demangle_service_from_topic(ros_service_requester_prefix, topic_name, "Request");
}

std::string
_demangle_service_reply_from_topic(const std::string & topic_name)
{
  return _demangle_service_from_topic(ros_service_response_prefix, topic_name, "Reply");
}

/// Return the demangled service type if it is a ROS srv type, else "".
std::string
_demangle_service_type_only(const std::string & dds_type_name)
{
  std::string ns_substring = "dds_::";
  size_t ns_substring_position = dds_type_name.find(ns_substring);
  if (std::string::npos == ns_substring_position) {
    // not a ROS service type
    return "";
  }
  auto suffixes = {
    std::string("_Response_"),
    std::string("_Request_"),
  };
  std::string found_suffix = "";
  size_t suffix_position = 0;
  for (auto suffix : suffixes) {
    suffix_position = dds_type_name.rfind(suffix);
    if (suffix_position != std::string::npos) {
      if (dds_type_name.length() - suffix_position - suffix.length() != 0) {
        RCUTILS_LOG_WARN_NAMED(
          "rmw_fastrtps_shared_cpp",
          "service type contains 'dds_::' and a suffix, but not at the end"
          ", report this: '%s'", dds_type_name.c_str());
        continue;
      }
      found_suffix = suffix;
      break;
    }
  }
  if (std::string::npos == suffix_position) {
    RCUTILS_LOG_WARN_NAMED(
      "rmw_fastrtps_shared_cpp",
      "service type contains 'dds_::' but does not have a suffix"
      ", report this: '%s'", dds_type_name.c_str());
    return "";
  }
  // everything checks out, reformat it from '[type_namespace::]dds_::<type><suffix>'
  // to '[type_namespace/]<type>'
  std::string type_namespace = dds_type_name.substr(0, ns_substring_position);
  type_namespace = rcpputils::find_and_replace(type_namespace, "::", "/");
  size_t start = ns_substring_position + ns_substring.length();
  std::string type_name = dds_type_name.substr(start, suffix_position - start);
  return type_namespace + type_name;
}

std::string
_identity_demangle(const std::string & name)
{
  return name;
}
