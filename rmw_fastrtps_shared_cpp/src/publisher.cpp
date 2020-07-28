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

#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/publisher.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
destroy_publisher(
  const char * identifier,
  CustomParticipantInfo * participant_info,
  rmw_publisher_t * publisher)
{
  assert(publisher->implementation_identifier == identifier);
  static_cast<void>(identifier);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  Domain::removePublisher(info->publisher_);
  delete info->listener_;

  _unregister_type(participant_info->participant, info->type_support_);
  delete info;

  rmw_free(const_cast<char *>(publisher->topic_name));
  rmw_publisher_free(publisher);

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
