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

#include <gtest/gtest.h>

#include "fastrtps/log/Log.h"

#include "rmw/rmw.h"
#include "rmw/error_handling.h"

TEST(TestLogging, rmw_logging)
{
  rmw_ret_t ret = rmw_set_log_severity(RMW_LOG_SEVERITY_DEBUG);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(eprosima::fastrtps::Log::Kind::Info, eprosima::fastrtps::Log::GetVerbosity());
  ret = rmw_set_log_severity(RMW_LOG_SEVERITY_INFO);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(eprosima::fastrtps::Log::Kind::Info, eprosima::fastrtps::Log::GetVerbosity());
  ret = rmw_set_log_severity(RMW_LOG_SEVERITY_WARN);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(eprosima::fastrtps::Log::Kind::Warning, eprosima::fastrtps::Log::GetVerbosity());
  ret = rmw_set_log_severity(RMW_LOG_SEVERITY_ERROR);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(eprosima::fastrtps::Log::Kind::Error, eprosima::fastrtps::Log::GetVerbosity());
  ret = rmw_set_log_severity(RMW_LOG_SEVERITY_FATAL);
  EXPECT_EQ(ret, RMW_RET_OK);
  EXPECT_EQ(eprosima::fastrtps::Log::Kind::Error, eprosima::fastrtps::Log::GetVerbosity());
}

TEST(TestLogging, rmw_logging_bad_verbosity)
{
  rmw_ret_t ret = rmw_set_log_severity(static_cast<rmw_log_severity_t>(RMW_LOG_SEVERITY_FATAL + 1));
  EXPECT_EQ(ret, RMW_RET_ERROR);
}
