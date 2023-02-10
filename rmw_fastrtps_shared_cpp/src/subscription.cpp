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

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
destroy_subscription(
  const char * identifier,
  CustomParticipantInfo * participant_info,
  rmw_subscription_t * subscription,
  bool reset_cft)
{
  assert(subscription->implementation_identifier == identifier);
  static_cast<void>(identifier);

  {
    std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

    // Get RMW Subscriber
    auto info = static_cast<CustomSubscriberInfo *>(subscription->data);

    // Delete DataReader
    ReturnCode_t ret = participant_info->subscriber_->delete_datareader(info->data_reader_);
    if (ReturnCode_t::RETCODE_OK != ret) {
      RMW_SET_ERROR_MSG("Failed to delete datareader");
      // This is the first failure on this function, and we have not changed state.
      // This means it should be safe to return an error
      return RMW_RET_ERROR;
    }

    // Delete ContentFilteredTopic
    if (nullptr != info->filtered_topic_) {
      participant_info->participant_->delete_contentfilteredtopic(info->filtered_topic_);
      info->filtered_topic_ = nullptr;
    }

    if (reset_cft) {
      return RMW_RET_OK;
    }

    // Delete DataReader listener
    delete info->data_reader_listener_;

    // Delete topic and unregister type
    remove_topic_and_type(
      participant_info, info->subscription_event_, info->topic_, info->type_support_);

    delete info->subscription_event_;

    // Delete CustomSubscriberInfo structure
    delete info;
  }

  rmw_free(const_cast<char *>(subscription->topic_name));
  rmw_subscription_free(subscription);

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion
  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
