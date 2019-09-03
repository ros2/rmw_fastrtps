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
  if (!subscription) {
    RMW_SET_ERROR_MSG("subscription handle is null");
    return RMW_RET_ERROR;
  }
  if (subscription->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("subscription handle not from this implementation");
    return RMW_RET_ERROR;
  }
  if (!participant_info) {
    RMW_SET_ERROR_MSG("participant_info is null");
    return RMW_RET_ERROR;
  }

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  if (info != nullptr) {
    if (info->subscriber_ != nullptr) {
      Domain::removeSubscriber(info->subscriber_);
    }
    delete info->listener_;
    if (info->type_support_ != nullptr) {
      Participant * participant = participant_info->participant;
      _unregister_type(participant, info->type_support_);
    }
    delete info;
  }
  rmw_free(const_cast<char *>(subscription->topic_name));
  subscription->topic_name = nullptr;
  rmw_subscription_free(subscription);

  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
