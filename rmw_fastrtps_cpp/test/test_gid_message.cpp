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

#include <gtest/gtest.h>
#include <string.h>

#include "rmw_dds_common/msg/gid.hpp"

#include "rosidl_generator_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_fastrtps_cpp/identifier.hpp"

using rmw_dds_common::msg::Gid;


TEST(Test_gid, constructor_destructor) {
  Gid gid;
  ASSERT_EQ(24u, gid.data.size());
  gid.data[0] = 'a';
}

TEST(Test_gid, get_typesupport) {
  const rosidl_message_type_support_t * type_support_1 =
    rosidl_typesupport_cpp::get_message_type_support_handle<Gid>();
  const rosidl_message_type_support_t * type_support_2 = get_message_typesupport_handle(
    type_support_1, rosidl_typesupport_fastrtps_cpp::typesupport_identifier);

  ASSERT_EQ(type_support_1->typesupport_identifier, type_support_2->typesupport_identifier);
  ASSERT_EQ(type_support_1->data, type_support_2->data);
  ASSERT_EQ(type_support_1->func, type_support_2->func);
}
