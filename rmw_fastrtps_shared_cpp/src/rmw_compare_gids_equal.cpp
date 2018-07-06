// Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include "fastrtps/rtps/common/Guid.h"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"
#include "rmw/types.h"

#include "rmw_fastrtps_cpp/identifier.hpp"

extern "C"
{
rmw_ret_t
rmw_compare_gids_equal(const rmw_gid_t * gid1, const rmw_gid_t * gid2, bool * result)
{
  if (!gid1) {
    RMW_SET_ERROR_MSG("gid1 is null");
    return RMW_RET_ERROR;
  }

  if (gid1->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("guid1 handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!gid2) {
    RMW_SET_ERROR_MSG("gid2 is null");
    return RMW_RET_ERROR;
  }

  if (gid2->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("gid1 handle not from this implementation");
    return RMW_RET_ERROR;
  }

  if (!result) {
    RMW_SET_ERROR_MSG("result is null");
    return RMW_RET_ERROR;
  }

  *result =
    memcmp(gid1->data, gid2->data, sizeof(eprosima::fastrtps::rtps::GUID_t)) == 0;

  return RMW_RET_OK;
}
}  // extern "C"
