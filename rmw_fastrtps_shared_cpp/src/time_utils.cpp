// Copyright 2021 Open Source Robotics Foundation, Inc.
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

#include "rmw_dds_common/time_utils.hpp"

#include "time_utils.hpp"

namespace rmw_fastrtps_shared_cpp
{

eprosima::fastrtps::Duration_t
rmw_time_to_fastrtps(const rmw_time_t & time)
{
  if (rmw_time_equal(time, RMW_DURATION_INFINITE)) {
    return eprosima::fastrtps::rtps::c_RTPSTimeInfinite.to_duration_t();
  }

  rmw_time_t clamped_time = rmw_dds_common::clamp_rmw_time_to_dds_time(time);
  return eprosima::fastrtps::Duration_t(
    static_cast<int32_t>(clamped_time.sec),
    static_cast<uint32_t>(clamped_time.nsec));
}

}  // namespace rmw_fastrtps_shared_cpp
