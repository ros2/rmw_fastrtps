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

#ifndef RMW_FASTRTPS_SHARED_CPP__RMW_SECURITY_LOGGING_HPP_
#define RMW_FASTRTPS_SHARED_CPP__RMW_SECURITY_LOGGING_HPP_

#include "fastdds/rtps/attributes/PropertyPolicy.h"

#include "rmw_fastrtps_shared_cpp/visibility_control.h"

/// Apply any requested security logging configuration to the policy.
/**
 * \param policy policy to which security logging properties may be added.
 * \returns false if the requested configuration could not be applied (rmw error will be set).
 * \returns true if the requested configuration was applied (or no configuration was requested).
 */
RMW_FASTRTPS_SHARED_CPP_PUBLIC
bool apply_security_logging_configuration(eprosima::fastrtps::rtps::PropertyPolicy & policy);

#endif  // RMW_FASTRTPS_SHARED_CPP__RMW_SECURITY_LOGGING_HPP_
