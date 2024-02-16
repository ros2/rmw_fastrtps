// Copyright 2016 Open Source Robotics Foundation, Inc.
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

#ifndef SERIALIZATION_HELPERS_HPP_
#define SERIALIZATION_HELPERS_HPP_

#include <limits>
#include <string>

#define SPECIALIZE_GENERIC_C_SEQUENCE(C_NAME, C_TYPE) \
  template<> \
  struct GenericCSequence<C_TYPE> \
  { \
    using type = rosidl_runtime_c__ ## C_NAME ## __Sequence; \
 \
    static void fini(type * array) { \
      rosidl_runtime_c__ ## C_NAME ## __Sequence__fini(array); \
    } \
 \
    static bool init(type * array, size_t size) { \
      return rosidl_runtime_c__ ## C_NAME ## __Sequence__init(array, size); \
    } \
  };

namespace eprosima
{
namespace fastcdr
{

inline eprosima::fastcdr::Cdr & operator<<(
  eprosima::fastcdr::Cdr & cdr, const std::u16string & u16str)
{
  auto ptr = u16str.c_str();
  auto len = static_cast<uint32_t>(u16str.size());
  cdr << len;
  for (; len > 0; --len) {
    cdr << static_cast<uint32_t>(*ptr++);
  }
  return cdr;
}

inline eprosima::fastcdr::Cdr & operator<<(
  eprosima::fastcdr::Cdr & cdr, const rosidl_runtime_c__U16String & u16str)
{
  auto ptr = u16str.data;
  auto len = static_cast<uint32_t>(u16str.size);
  cdr << len;
  for (; len > 0; --len) {
    cdr << static_cast<uint32_t>(*ptr++);
  }
  return cdr;
}

inline eprosima::fastcdr::Cdr & operator>>(
  eprosima::fastcdr::Cdr & cdr, std::u16string & u16str)
{
  uint32_t len;
  cdr >> len;
  u16str.resize(len);
  for (uint32_t i = 0; i < len; ++i) {
    uint32_t c;
    cdr >> c;
    u16str[i] = static_cast<char16_t>(c);
  }

  return cdr;
}

inline eprosima::fastcdr::Cdr & operator>>(
  eprosima::fastcdr::Cdr & cdr, rosidl_runtime_c__U16String & u16str)
{
  uint32_t len;
  cdr >> len;
  if (!rosidl_runtime_c__U16String__resize(&u16str, len)) {
    throw std::bad_alloc();
  }

  for (uint32_t i = 0; i < len; ++i) {
    uint32_t c;
    cdr >> c;
    u16str.data[i] = static_cast<char16_t>(c);
  }

  return cdr;
}

}  // namespace fastcdr
}  // namespace eprosima

#endif  // SERIALIZATION_HELPERS_HPP_
