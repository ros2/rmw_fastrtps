// Copyright 2026 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_FASTRTPS_SHARED_CPP__RMW_IS_LOANED_MESSAGE_VALID_HPP_
#define RMW_FASTRTPS_SHARED_CPP__RMW_IS_LOANED_MESSAGE_VALID_HPP_

#include "rmw_fastrtps_shared_cpp/visibility_control.h"
#include "rmw/rmw.h"

namespace rmw_fastrtps_shared_cpp
{
/**
 * Check if a loaned message is still valid (not overwritten by middleware).
 *
 * This function checks if the loaned message memory has been invalidated
 * by the middleware (e.g., when using shared memory with Fast-DDS data-sharing).
 *
 * \param[in] subscription The rmw subscription handle
 * \param[in] loaned_message Pointer to the loaned message data
 * \param[in] message_info Message info containing publisher_gid and sequence_number
 * \param[out] is_valid Output parameter indicating if the message is valid
 * \return RMW_RET_OK if successful, RMW_RET_INVALID_ARGUMENT if parameters are invalid
 */
RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_ret_t
rmw_is_loaned_message_valid(
  const rmw_subscription_t * subscription,
  const void * loaned_message,
  const rmw_message_info_t * message_info,
  bool * is_valid);
}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__RMW_IS_LOANED_MESSAGE_VALID_HPP_
