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

#include "fastdds/rtps/common/Guid.h"

#include "rmw/types.h"

#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"

rmw_gid_t
rmw_fastrtps_shared_cpp::create_rmw_gid(
  const char * identifier, const eprosima::fastrtps::rtps::GUID_t & guid)
{
  rmw_gid_t rmw_gid = {};
  rmw_gid.implementation_identifier = identifier;
  static_assert(
    sizeof(eprosima::fastrtps::rtps::GUID_t) <= RMW_GID_STORAGE_SIZE,
    "RMW_GID_STORAGE_SIZE insufficient to store the fastrtps GUID_t."
  );
  rmw_fastrtps_shared_cpp::copy_from_fastrtps_guid_to_byte_array(
    guid,
    rmw_gid.data);
  return rmw_gid;
}
