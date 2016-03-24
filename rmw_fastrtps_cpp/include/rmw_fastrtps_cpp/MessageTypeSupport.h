#ifndef _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_

#include "TypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>

#include <cassert>
#include <memory>


namespace rmw_fastrtps_cpp
{
    template <typename MembersType>
    class MessageTypeSupport : public TypeSupport<MembersType>
    {
        public:

            MessageTypeSupport(const MembersType *members);
    };
}

template <typename MembersType>
rmw_fastrtps_cpp::MessageTypeSupport<MembersType>::MessageTypeSupport(const MembersType *members)
{
    assert(members);
    this->members_ = members;

    if(strcmp(members->package_name_, "rcl_interfaces") == 0 && strcmp(members->message_name_, "ParameterEvent") == 0)
        this->typeTooLarge_ = true;

    std::string name = std::string(members->package_name_) + "::msg::dds_::" + members->message_name_ + "_";
    this->setName(strdup(name.c_str()));

    if(members->member_count_ != 0)
        this->m_typeSize = this->calculateMaxSerializedSize(members, 0);
    else
        this->m_typeSize = 1;
}
#endif // _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
