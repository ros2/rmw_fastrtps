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

#ifndef RMW_FASTRTPS_SHARED_CPP__PARTICIPANT_HPP_
#define RMW_FASTRTPS_SHARED_CPP__PARTICIPANT_HPP_

#include "rmw/types.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/visibility_control.h"

namespace rmw_fastrtps_shared_cpp
{

// This method will create a DDS DomainParticipant along with
// a DDS Publisher and a DDS Subscriber
// For the creation of DDS DataWriter see method create_publisher
// For the creation of DDS DataReader see method create_subscription
// Note that ROS 2 Publishers and Subscriptions correspond with DDS DataWriters
// and DataReaders respectively and not with DDS Publishers and Subscribers.
RMW_FASTRTPS_SHARED_CPP_PUBLIC
CustomParticipantInfo *
create_participant(
  const char * identifier,
  size_t domain_id,
  const rmw_security_options_t * security_options,
  bool localhost_only,
  const char * enclave,
  rmw_dds_common::Context * common_context);

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
destroy_participant(CustomParticipantInfo * info);

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__PARTICIPANT_HPP_
