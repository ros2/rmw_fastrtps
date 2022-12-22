// Copyright 2022 Open Source Robotics Foundation, Inc.
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

#include "rmw/features.h"

#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"

bool
rmw_fastrtps_shared_cpp::__rmw_feature_supported(rmw_feature_t feature)
{
  switch (feature) {
    case RMW_FEATURE_MESSAGE_INFO_PUBLICATION_SEQUENCE_NUMBER:
      return true;

    // NOTE(methylDragon): The dynamic type deferred case is !! NOT SUPPORTED !!
    //                     This is because it's difficult as-is to create a subscription without
    //                     already having the type. Too much restructuring is needed elsewhere to
    //                     support deferral...
    //
    //                     This is noting that type discovery IS a thing that FastRTPS supports,
    //                     but the structure of the create_subscription implementations don't lend
    //                     themselves currently to integrating that type discovery support yet.
    case RMW_MIDDLEWARE_SUPPORTS_TYPE_DISCOVERY:
      return false;

    case RMW_MIDDLEWARE_CAN_TAKE_DYNAMIC_DATA:
      return true;

    default:
      return false;
  }
}
