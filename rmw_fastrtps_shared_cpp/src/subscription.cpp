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

#include <utility>
#include <string>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "fastrtps/Domain.h"
#include "fastrtps/TopicDataType.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/subscriber/Subscriber.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using TopicDataType = eprosima::fastrtps::TopicDataType;

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
destroy_subscription(
  const char * identifier,
  CustomParticipantInfo * participant_info,
  rmw_subscription_t * subscription)
{
  assert(subscription->implementation_identifier == identifier);
  static_cast<void>(identifier);

  rmw_ret_t ret = RMW_RET_OK;
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  if (!Domain::removeSubscriber(info->subscriber_)) {
    RMW_SET_ERROR_MSG("failed to remove subscriber");
    ret = RMW_RET_ERROR;
  }
  delete info->listener_;

  Participant * participant = participant_info->participant;
  _unregister_type(participant, info->type_support_);
  delete info;

  rmw_free(const_cast<char *>(subscription->topic_name));
  rmw_subscription_free(subscription);

  return ret;
}
}  // namespace rmw_fastrtps_shared_cpp
