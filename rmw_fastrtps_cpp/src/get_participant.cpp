// Copyright 2017 Open Source Robotics Foundation, Inc.
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

#include "rmw_fastrtps_cpp/get_participant.hpp"

#include "rmw_fastrtps_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_cpp/identifier.hpp"

namespace rmw_fastrtps_cpp
{

eprosima::fastrtps::Participant *
get_participant(rmw_node_t * node)
{
  if (!node) {
    return NULL;
  }
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    return NULL;
  }
  CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
  return impl->participant;
}

}  // namespace rmw_fastrtps_cpp
