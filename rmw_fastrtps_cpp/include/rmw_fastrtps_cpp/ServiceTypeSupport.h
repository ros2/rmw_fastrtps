#ifndef _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_

#include "TypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>


struct CustomServiceInfo;

namespace rmw_fastrtps_cpp
{
    template <typename MembersType>
    class ServiceTypeSupport : public TypeSupport<MembersType>
    {
        protected:

            ServiceTypeSupport();
    };

    template <typename ServiceMembersType, typename MessageMembersType>
    class RequestTypeSupport : public ServiceTypeSupport<MessageMembersType>
    {
        public:

            RequestTypeSupport(const ServiceMembersType *members);
    };

    template <typename ServiceMembersType, typename MessageMembersType>
    class ResponseTypeSupport : public ServiceTypeSupport<MessageMembersType>
    {
        public:

            ResponseTypeSupport(const ServiceMembersType *members);
    };
}

template <typename MembersType>
rmw_fastrtps_cpp::ServiceTypeSupport<MembersType>::ServiceTypeSupport()
{
}

template <typename ServiceMembersType, typename MessageMembersType>
rmw_fastrtps_cpp::RequestTypeSupport<ServiceMembersType, MessageMembersType>::RequestTypeSupport(const ServiceMembersType *members)
{
    assert(members);
    this->members_ = members->request_members_;

    if(strcmp(members->package_name_, "rcl_interfaces") == 0 && (strcmp(members->service_name_, "SetParameters") == 0 ||
            strcmp(members->service_name_, "SetParametersAtomically") == 0))
        this->typeTooLarge_ = true;

    std::string name = std::string(members->package_name_) + "::srv::dds_::" + members->service_name_ + "_Request_";
    this->setName(strdup(name.c_str()));

    if(this->members_->member_count_ != 0)
        this->m_typeSize = this->calculateMaxSerializedSize(this->members_, 0);
    else
        this->m_typeSize = 1;
}

template <typename ServiceMembersType, typename MessageMembersType>
rmw_fastrtps_cpp::ResponseTypeSupport<ServiceMembersType, MessageMembersType>::ResponseTypeSupport(const ServiceMembersType *members)
{
    assert(members);
    this->members_ = members->response_members_;

    std::string name = std::string(members->package_name_) + "::srv::dds_::" + members->service_name_ + "_Response_";
    this->setName(strdup(name.c_str()));

    if(this->members_->member_count_ != 0)
        this->m_typeSize = this->calculateMaxSerializedSize(this->members_, 0);
    else
        this->m_typeSize = 1;
}

#endif // _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_
