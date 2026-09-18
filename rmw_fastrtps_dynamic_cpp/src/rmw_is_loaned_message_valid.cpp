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
#include "rmw/rmw.h"
#include "rmw_fastrtps_shared_cpp/rmw_is_loaned_message_valid.hpp"
#include "rmw_fastrtps_dynamic_cpp/identifier.hpp"
extern "C" {
rmw_ret_t
rmw_is_loaned_message_valid(
  const rmw_subscription_t * subscription,
  const void * loaned_message,
  const rmw_message_info_t * message_info,
  bool * is_valid)
{
  return rmw_fastrtps_shared_cpp::rmw_is_loaned_message_valid(
    subscription, loaned_message, message_info, is_valid);
}
}  // extern "C"
