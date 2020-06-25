// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#include "gtest/gtest.h"

#include "rcutils/allocator.h"
#include "rcutils/strdup.h"
#include "rmw/init_options.h"
#include "rmw/security_options.h"
#include "rmw/types.h"

#include "osrf_testing_tools_cpp/scope_exit.hpp"

#include "rmw_fastrtps_shared_cpp/rmw_init.hpp"

using rmw_fastrtps_shared_cpp::rmw_init_options_init;
using rmw_fastrtps_shared_cpp::rmw_init_options_copy;
using rmw_fastrtps_shared_cpp::rmw_init_options_fini;

static const char * const some_identifier = "some_identifier";

TEST(RMWInitOptionsTest, init_w_invalid_args_fails) {
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  // Cannot initialize a null options instance.
  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT,
    rmw_init_options_init(some_identifier, nullptr, allocator));
  rcutils_reset_error();

  rmw_init_options_t options = rmw_get_zero_initialized_init_options();
  rcutils_allocator_t invalid_allocator = rcutils_get_zero_initialized_allocator();
  // Cannot initialize using an invalid allocator.
  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT,
    rmw_init_options_init(some_identifier, &options, invalid_allocator));
  rcutils_reset_error();
}


TEST(RMWInitOptionsTest, init_twice_fails) {
  rmw_init_options_t options = rmw_get_zero_initialized_init_options();
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  ASSERT_EQ(RMW_RET_OK, rmw_init_options_init(some_identifier, &options, allocator)) <<
    rcutils_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    rcutils_reset_error();
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(some_identifier, &options)) <<
      rcutils_get_error_string().str;
    rcutils_reset_error();
  });

  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT,
    rmw_init_options_init(some_identifier, &options, allocator));
  rcutils_reset_error();
}


TEST(RMWInitOptionsTest, init) {
  rmw_init_options_t options = rmw_get_zero_initialized_init_options();
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  ASSERT_EQ(RMW_RET_OK, rmw_init_options_init(some_identifier, &options, allocator)) <<
    rcutils_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    rcutils_reset_error();
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(some_identifier, &options)) <<
      rcutils_get_error_string().str;
    rcutils_reset_error();
  });

  EXPECT_EQ(some_identifier, options.implementation_identifier);
}


TEST(RMWInitOptionsTest, copy_w_invalid_args_fails) {
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rmw_init_options_t not_initialized_options = rmw_get_zero_initialized_init_options();
  rmw_init_options_t initialized_options = rmw_get_zero_initialized_init_options();

  ASSERT_EQ(
    RMW_RET_OK,
    rmw_init_options_init(
      some_identifier,
      &initialized_options,
      allocator)) <<
    rcutils_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    rcutils_reset_error();
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(some_identifier, &initialized_options)) <<
      rcutils_get_error_string().str;
    rcutils_reset_error();
  });

  // Cannot copy from a null options instance.
  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT,
    rmw_init_options_copy(
      some_identifier,
      nullptr,
      &not_initialized_options));
  rcutils_reset_error();

  // Cannot copy to a null options instance.
  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT,
    rmw_init_options_copy(
      some_identifier,
      &initialized_options,
      nullptr));
  rcutils_reset_error();

  // Cannot copy an options instance if implementation identifiers do not match.
  EXPECT_EQ(
    RMW_RET_INCORRECT_RMW_IMPLEMENTATION,
    rmw_init_options_copy(
      "another_identifier",
      &initialized_options,
      &not_initialized_options));
  rcutils_reset_error();

  // Cannot copy to an already initialized options instance.
  EXPECT_EQ(
    RMW_RET_INVALID_ARGUMENT, rmw_init_options_copy(
      some_identifier,
      &initialized_options,
      &initialized_options));
  rcutils_reset_error();
}


TEST(RMWInitOptionsTest, copy) {
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  rmw_init_options_t preset_options = rmw_get_zero_initialized_init_options();

  rcutils_reset_error();
  ASSERT_EQ(RMW_RET_OK, rmw_init_options_init(some_identifier, &preset_options, allocator)) <<
    rcutils_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    rcutils_reset_error();
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(some_identifier, &preset_options)) <<
      rcutils_get_error_string().str;
    rcutils_reset_error();
  });
  preset_options.instance_id = 23lu;
  preset_options.enclave = rcutils_strdup("/test", allocator);
  ASSERT_TRUE(preset_options.enclave != nullptr);
  preset_options.security_options.security_root_path = rcutils_strdup("/root", allocator);
  ASSERT_TRUE(preset_options.security_options.security_root_path != nullptr);

  rmw_init_options_t options = rmw_get_zero_initialized_init_options();
  ASSERT_EQ(RMW_RET_OK, rmw_init_options_copy(some_identifier, &preset_options, &options)) <<
    rcutils_get_error_string().str;

  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    rcutils_reset_error();
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(some_identifier, &options)) <<
      rcutils_get_error_string().str;
    rcutils_reset_error();
  });
  EXPECT_EQ(23lu, options.instance_id);
  EXPECT_STREQ("/test", options.enclave);
  EXPECT_STREQ("/root", options.security_options.security_root_path);
}

static void * failing_allocate(size_t size, void * state)
{
  (void)size;
  (void)state;
  return NULL;
}

TEST(RMWInitOptionsTest, bad_alloc_on_copy) {
  rcutils_allocator_t failing_allocator = rcutils_get_default_allocator();
  failing_allocator.allocate = failing_allocate;

  rmw_init_options_t preset_options = rmw_get_zero_initialized_init_options();
  ASSERT_EQ(
    RMW_RET_OK,
    rmw_init_options_init(some_identifier, &preset_options, failing_allocator)) <<
    rcutils_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    rcutils_reset_error();
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(some_identifier, &preset_options)) <<
      rcutils_get_error_string().str;
    rcutils_reset_error();
  });
  preset_options.enclave = rcutils_strdup("/test", rcutils_get_default_allocator());
  ASSERT_TRUE(preset_options.enclave != nullptr);

  rmw_init_options_t options = rmw_get_zero_initialized_init_options();
  EXPECT_EQ(
    RMW_RET_BAD_ALLOC,
    rmw_init_options_copy(some_identifier, &preset_options, &options));
}

TEST(RMWInitOptionsTest, fini_w_invalid_args_fails) {
  // Cannot finalize a null options instance.
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, rmw_init_options_fini(some_identifier, nullptr));
  rcutils_reset_error();

  rmw_init_options_t options = rmw_get_zero_initialized_init_options();
  // Cannot finalize an options instance that has not been initialized.
  EXPECT_EQ(RMW_RET_INVALID_ARGUMENT, rmw_init_options_fini(some_identifier, &options));
  rcutils_reset_error();

  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  ASSERT_EQ(RMW_RET_OK, rmw_init_options_init(some_identifier, &options, allocator)) <<
    rcutils_get_error_string().str;
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT(
  {
    rcutils_reset_error();
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(some_identifier, &options)) <<
      rcutils_get_error_string().str;
    rcutils_reset_error();
  });

  // Cannot finalize an options instance if implementation identifiers do not match.
  const char * const another_identifier = "another_identifier";
  EXPECT_EQ(
    RMW_RET_INCORRECT_RMW_IMPLEMENTATION,
    rmw_init_options_fini(another_identifier, &options));
  rcutils_reset_error();
}
