#include "rmw_fastrtps_cpp/ServiceTypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>

using namespace rmw_fastrtps_cpp;

ServiceTypeSupport::ServiceTypeSupport()
{
}

RequestTypeSupport::RequestTypeSupport(const rosidl_typesupport_introspection_cpp::ServiceMembers *members)
{
    assert(members);
    members_ = members->request_members_;

    if(strcmp(members->package_name_, "rcl_interfaces") == 0 && (strcmp(members->service_name_, "SetParameters") == 0 ||
            strcmp(members->service_name_, "SetParametersAtomically") == 0))
        typeTooLarge_ = true;

    std::string name = std::string(members->package_name_) + "::srv::dds_::" + members->service_name_ + "_Request_";
    setName(name.c_str());

    if(members_->member_count_ != 0)
        m_typeSize = static_cast<uint32_t>(calculateMaxSerializedSize(members_, 0));
    else
        m_typeSize = 1;
}

ResponseTypeSupport::ResponseTypeSupport(const rosidl_typesupport_introspection_cpp::ServiceMembers *members)
{
    assert(members);
    members_ = members->response_members_;

    std::string name = std::string(members->package_name_) + "::srv::dds_::" + members->service_name_ + "_Response_";
    setName(name.c_str());

    if(members_->member_count_ != 0)
        m_typeSize = static_cast<uint32_t>(calculateMaxSerializedSize(members_, 0));
    else
        m_typeSize = 1;
}
