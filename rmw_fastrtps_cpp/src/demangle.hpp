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

#ifndef DEMANGLE_HPP_
#define DEMANGLE_HPP_

#include <string>

/// Return the demangle ROS topic or the original if not a ROS topic.
std::string
_demangle_if_ros_topic(const std::string & topic_name);

/// Return the demangled ROS type or the original if not a ROS type.
std::string
_demangle_if_ros_type(const std::string & dds_type_string);

/// Return the service name for a given topic if it is part of one, else "".
std::string
_demangle_service_from_topic(const std::string & topic_name);

/// Return the demangled service type if it is a ROS srv type, else "".
std::string
_demangle_service_type_only(const std::string & dds_type_name);

#endif  // DEMANGLE_HPP_
