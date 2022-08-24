// Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef TYPES__EVENT_TYPES_HPP_
#define TYPES__EVENT_TYPES_HPP_

#include "rmw/event.h"

#include "fastdds/dds/core/status/StatusMask.hpp"

namespace rmw_fastrtps_shared_cpp
{
namespace internal
{

bool is_event_supported(rmw_event_type_t event_type);

eprosima::fastdds::dds::StatusMask rmw_event_to_dds_statusmask(
  const rmw_event_type_t event_type);

}  // namespace internal
}  // namespace rmw_fastrtps_shared_cpp

#endif  // TYPES__EVENT_TYPES_HPP_
