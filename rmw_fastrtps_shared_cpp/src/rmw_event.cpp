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

#include <unordered_set>

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

static const std::unordered_set<rmw_event_type_t> __rmw_event_type_set{
  RMW_EVENT_LIVELINESS_CHANGED,
  RMW_EVENT_REQUESTED_DEADLINE_MISSED,
  RMW_EVENT_LIVELINESS_LOST,
  RMW_EVENT_OFFERED_DEADLINE_MISSED
};

namespace rmw_fastrtps_shared_cpp
{

bool
__rmw_event_type_is_supported(rmw_event_type_t event_type)
{
  return __rmw_event_type_set.count(event_type) > 0;
}

}  // namespace rmw_fastrtps_shared_cpp
