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

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/publisher.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

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

  {
    std::lock_guard<std::mutex> lck(participant_info->entity_creation_mutex_);

    // Get RMW Publisher
    auto info = static_cast<CustomPublisherInfo *>(publisher->data);

    // Delete DataWriter
    ReturnCode_t ret = participant_info->publisher_->delete_datawriter(info->data_writer_);
    if (ReturnCode_t::RETCODE_OK != ret) {
      RMW_SET_ERROR_MSG("Failed to delete datawriter");
      // This is the first failure on this function, and we have not changed state.
      // This means it should be safe to return an error
      return RMW_RET_ERROR;
    }

    // Delete DataWriter listener
    delete info->data_writer_listener_;

    // Delete topic and unregister type
    remove_topic_and_type(
      participant_info, info->publisher_event_, info->topic_, info->type_support_);

    delete info->publisher_event_;

    // Delete CustomPublisherInfo structure
    delete info;
  }

  rmw_free(const_cast<char *>(publisher->topic_name));
  rmw_publisher_free(publisher);

  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RMW_RET_ERROR);  // on completion
  return RMW_RET_OK;
}
}  // namespace rmw_fastrtps_shared_cpp
