// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <string>

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"
#include "rcutils/types.h"

#include "rmw/allocators.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/sanity_checks.h"

#include "fastrtps/Domain.h"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

using Participant = eprosima::fastrtps::Participant;

extern "C"
{
rmw_ret_t
rmw_get_node_and_locator_names(
  const rmw_node_t * node,
  rcutils_string_map_t * names)
{
  if (!node) {
    RMW_SET_ERROR_MSG("null node handle");
    return RMW_RET_ERROR;
  }
  /*if (rmw_check_zero_rmw_string_array(node_names) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }*/

  // Get participant pointer from node
  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  auto participant_names = impl->listener->get_discovered_node_and_locator_names();

  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rcutils_ret_t rcutils_ret =
    rcutils_string_map_init(names, participant_names.size() + 1, allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string_safe())
    return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
  }

  // show content:
  for (std::map<std::string,std::string>::iterator it=participant_names.begin(); it!=participant_names.end(); ++it) {
    rcutils_ret_t ret =rcutils_string_map_set(names, ((it)->first).c_str(), ((it)->second).c_str());
    (void)ret;
  }
  return RMW_RET_OK;
}
}  // extern "C"
