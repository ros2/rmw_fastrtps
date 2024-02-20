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

#include "TypeSupport.hpp"

namespace rmw_fastrtps_dynamic_cpp
{

TypeSupportProxy::TypeSupportProxy(rmw_fastrtps_shared_cpp::TypeSupport * inner_type)
{
  setName(inner_type->getName());
  m_typeSize = inner_type->m_typeSize;
  is_plain_ = inner_type->is_plain();
  max_size_bound_ = inner_type->is_bounded();
}

size_t TypeSupportProxy::getEstimatedSerializedSize(
  const void * ros_message, const void * impl) const
{
  auto type_impl = static_cast<const rmw_fastrtps_shared_cpp::TypeSupport *>(impl);
  return type_impl->getEstimatedSerializedSize(ros_message, impl);
}

bool TypeSupportProxy::serializeROSmessage(
  const void * ros_message, eprosima::fastcdr::Cdr & ser, const void * impl) const
{
  auto type_impl = static_cast<const rmw_fastrtps_shared_cpp::TypeSupport *>(impl);
  return type_impl->serializeROSmessage(ros_message, ser, impl);
}

bool TypeSupportProxy::deserializeROSmessage(
  eprosima::fastcdr::Cdr & deser, void * ros_message, const void * impl) const
{
  auto type_impl = static_cast<const rmw_fastrtps_shared_cpp::TypeSupport *>(impl);
  return type_impl->deserializeROSmessage(deser, ros_message, impl);
}

}  // namespace rmw_fastrtps_dynamic_cpp
