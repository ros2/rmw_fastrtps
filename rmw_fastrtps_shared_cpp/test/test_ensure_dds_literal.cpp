// Copyright 2026 Open Source Robotics Foundation, Inc.
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

#include <string>

#include "rmw_fastrtps_shared_cpp/utils.hpp"

using rmw_fastrtps_shared_cpp::ensure_dds_literal;

// Bare strings should be wrapped in single quotes
TEST(EnsureDdsLiteral, bare_string_gets_quoted)
{
  EXPECT_EQ("'hello'", ensure_dds_literal("hello"));
  EXPECT_EQ("'RED'", ensure_dds_literal("RED"));
  EXPECT_EQ("'some value'", ensure_dds_literal("some value"));
}

// Already single-quoted strings should pass through unchanged
TEST(EnsureDdsLiteral, already_single_quoted)
{
  EXPECT_EQ("'hello'", ensure_dds_literal("'hello'"));
  EXPECT_EQ("'with spaces'", ensure_dds_literal("'with spaces'"));
}

// Backtick-quoted strings should pass through unchanged
TEST(EnsureDdsLiteral, already_backtick_quoted)
{
  EXPECT_EQ("`hello`", ensure_dds_literal("`hello`"));
}

// Boolean literals should pass through unchanged
TEST(EnsureDdsLiteral, boolean_literals)
{
  EXPECT_EQ("TRUE", ensure_dds_literal("TRUE"));
  EXPECT_EQ("FALSE", ensure_dds_literal("FALSE"));
}

// Integer literals should pass through unchanged
TEST(EnsureDdsLiteral, integer_literals)
{
  EXPECT_EQ("42", ensure_dds_literal("42"));
  EXPECT_EQ("0", ensure_dds_literal("0"));
  EXPECT_EQ("-1", ensure_dds_literal("-1"));
  EXPECT_EQ("+5", ensure_dds_literal("+5"));
}

// Float literals should pass through unchanged
TEST(EnsureDdsLiteral, float_literals)
{
  EXPECT_EQ("3.14", ensure_dds_literal("3.14"));
  EXPECT_EQ(".5", ensure_dds_literal(".5"));
  EXPECT_EQ("-0.1", ensure_dds_literal("-0.1"));
}

// Hex literals should pass through unchanged
TEST(EnsureDdsLiteral, hex_literals)
{
  EXPECT_EQ("0xFF", ensure_dds_literal("0xFF"));
  EXPECT_EQ("0X1A", ensure_dds_literal("0X1A"));
}

// Empty string should be quoted
TEST(EnsureDdsLiteral, empty_string)
{
  EXPECT_EQ("''", ensure_dds_literal(""));
}
