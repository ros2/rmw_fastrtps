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

#include "rmw_fastrtps_dynamic_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_dynamic_cpp
{

TypeSupportProxy::TypeSupportProxy(rmw_fastrtps_shared_cpp::TypeSupport * inner_type)
{
  setName(inner_type->getName());
  m_typeSize = inner_type->m_typeSize;
  is_plain_ = inner_type->is_plain();
  max_size_bound_ = inner_type->is_bounded();
  m_isGetKeyDefined = inner_type->m_isGetKeyDefined;
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

bool TypeSupportProxy::get_key_hash_from_ros_message(
    void * ros_message, eprosima::fastrtps::rtps::InstanceHandle_t * ihandle, bool force_md5, const void * impl) const
{
  auto type_impl = static_cast<const rmw_fastrtps_shared_cpp::TypeSupport *>(impl);
  return type_impl->get_key_hash_from_ros_message(ros_message, ihandle, force_md5, impl);
}

}  // namespace rmw_fastrtps_dynamic_cpp
