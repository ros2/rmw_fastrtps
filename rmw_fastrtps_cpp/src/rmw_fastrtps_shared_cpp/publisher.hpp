// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__PUBLISHER_HPP_
#define RMW_FASTRTPS_SHARED_CPP__PUBLISHER_HPP_

#include "rmw/rmw.h"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/visibility_control.h"

namespace rmw_fastrtps_shared_cpp
{

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
destroy_publisher(
  const char * identifier,
  CustomParticipantInfo * participant_info,
  rmw_publisher_t * publisher);

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__PUBLISHER_HPP_
