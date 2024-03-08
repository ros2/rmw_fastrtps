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

#include "rmw/error_handling.h"

#include "rmw_fastrtps_dynamic_cpp/TypeSupport.hpp"

#include "rosidl_typesupport_introspection_cpp/identifier.hpp"

#include "rosidl_typesupport_introspection_c/identifier.h"

#include "type_support_common.hpp"

bool
using_introspection_c_typesupport(const char * typesupport_identifier, uint8_t &abi_version)
{
  bool ret = false;
  if (strcmp(typesupport_identifier, rosidl_typesupport_introspection_c__identifier) == 0)
  {
    abi_version = rmw_fastrtps_shared_cpp::TypeSupport::AbiVersion::ABI_V1;
    ret = true;

  } else if (strcmp(typesupport_identifier, rosidl_typesupport_introspection_c__identifier_v2) == 0)
  {
    abi_version = rmw_fastrtps_shared_cpp::TypeSupport::AbiVersion::ABI_V2;
    ret = true;
  }
  return ret;
}

bool
using_introspection_cpp_typesupport(const char * typesupport_identifier, uint8_t &abi_version)
{
  bool ret = false;
  if (strcmp(typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_identifier) == 0)
  {
    abi_version = rmw_fastrtps_shared_cpp::TypeSupport::AbiVersion::ABI_V1;
    ret = true;

  } else if (strcmp(typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_identifier_v2) == 0)
  {
    abi_version = rmw_fastrtps_shared_cpp::TypeSupport::AbiVersion::ABI_V2;
    ret = true;
  }
  return ret;
}
