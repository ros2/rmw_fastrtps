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

#ifndef RMW_FASTRTPS_SHARED_CPP__CREATE_RMW_GID_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CREATE_RMW_GID_HPP_

#include "fastrtps/rtps/common/Guid.h"

#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/visibility_control.h"

namespace rmw_fastrtps_shared_cpp
{

RMW_FASTRTPS_SHARED_CPP_PUBLIC
rmw_gid_t
create_rmw_gid(const char * identifier, const eprosima::fastrtps::rtps::GUID_t & guid);

}  // namespace rmw_fastrtps_shared_cpp

#endif  // RMW_FASTRTPS_SHARED_CPP__CREATE_RMW_GID_HPP_
