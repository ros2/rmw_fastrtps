// Copyright 2020 Canonical Ltd.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__RMW_SECURITY_HPP_
#define RMW_FASTRTPS_SHARED_CPP__RMW_SECURITY_HPP_

#include <string>

#include "rmw/security_options.h"
#include "fastrtps/rtps/attributes/PropertyPolicy.h"

bool apply_security_options(
  const rmw_security_options_t & security_options,
  eprosima::fastrtps::rtps::PropertyPolicy & policy);

bool apply_logging_configuration_from_file(
  const std::string & xml_file_path,
  eprosima::fastrtps::rtps::PropertyPolicy & policy);

#endif  // RMW_FASTRTPS_SHARED_CPP__RMW_SECURITY_HPP_
