// Copyright 2017-2019 Open Source Robotics Foundation, Inc.
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

#ifndef PUBLISHER_HPP_
#define PUBLISHER_HPP_

#include "rmw/rmw.h"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"

namespace rmw_fastrtps_dynamic_cpp
{

rmw_publisher_t *
create_publisher(
  CustomParticipantInfo * participant_info,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options);

}  // namespace rmw_fastrtps_dynamic_cpp

#endif  // PUBLISHER_HPP_
