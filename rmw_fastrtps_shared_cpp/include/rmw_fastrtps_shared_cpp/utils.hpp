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

#ifndef RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_
#define RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_

#include "fastrtps/types/TypesBase.h"

#include "rmw/rmw.h"


namespace rmw_fastrtps_shared_cpp
{

rmw_ret_t cast_error_dds_to_rmw(eprosima::fastrtps::types::ReturnCode_t);

}

#endif  // RMW_FASTRTPS_SHARED_CPP__UTILS_HPP_
