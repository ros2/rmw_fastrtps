#include "rmw_fastrtps_cpp/MessageTypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>

#include <cassert>
#include <memory>

using namespace rmw_fastrtps_cpp;

MessageTypeSupport::MessageTypeSupport(const rosidl_typesupport_introspection_cpp::MessageMembers *members)
{
    assert(members);
    members_ = members;

    if(strcmp(members->package_name_, "rcl_interfaces") == 0 && strcmp(members->message_name_, "ParameterEvent") == 0)
        typeTooLarge_ = true;

    std::string name = std::string(members->package_name_) + "::msg::dds_::" + members->message_name_ + "_";
    setName(strdup(name.c_str()));

    if(members->member_count_ != 0)
        m_typeSize = static_cast<uint32_t>(calculateMaxSerializedSize(members, 0));
    else
        m_typeSize = 1;
}
