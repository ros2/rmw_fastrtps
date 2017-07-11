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

#ifndef ASSIGN_PARTITIONS_HPP_
#define ASSIGN_PARTITIONS_HPP_

#include <string>

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/split.h"
#include "rcutils/types.h"

#include "rmw/error_handling.h"

template<typename AttributeT>
inline
rcutils_ret_t
_assign_partitions_to_attributes(
  const char * const topic_name,
  const char * const prefix,
  bool avoid_ros_namespace_conventions,
  AttributeT * attributes)
{
  rcutils_ret_t ret = RCUTILS_RET_ERROR;
  auto allocator = rcutils_get_default_allocator();

  // set topic and partitions
  rcutils_string_array_t name_tokens = rcutils_get_zero_initialized_string_array();
  ret = rcutils_split_last(topic_name, '/', allocator, &name_tokens);
  if (ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string_safe());
    return ret;
  }
  if (name_tokens.size == 1) {
    if (!avoid_ros_namespace_conventions) {
      attributes->qos.m_partition.push_back(prefix);
    }
    attributes->topic.topicName = name_tokens.data[0];
    ret = RCUTILS_RET_OK;
  } else if (name_tokens.size == 2) {
    std::string partition;
    if (avoid_ros_namespace_conventions) {
      // no prefix to be used, just assign the user's namespace
      partition = name_tokens.data[0];
    } else {
      // concat the prefix with the user's namespace
      partition = std::string(prefix) + "/" + name_tokens.data[0];
    }
    attributes->qos.m_partition.push_back(partition.c_str());
    attributes->topic.topicName = name_tokens.data[1];
    ret = RCUTILS_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("Malformed topic name");
    ret = RCUTILS_RET_ERROR;
  }
  if (rcutils_string_array_fini(&name_tokens) != RCUTILS_RET_OK) {
    fprintf(stderr, "Failed to destroy the token string array\n");
    ret = RCUTILS_RET_ERROR;
  }
  return ret;
}

#endif  // ASSIGN_PARTITIONS_HPP_
