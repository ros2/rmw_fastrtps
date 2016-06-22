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

#include "ServiceTypeSupport_impl.h"

#endif // _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_
