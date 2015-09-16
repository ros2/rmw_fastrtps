#ifndef _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_

#include "TypeSupport.h"

struct CustomServiceInfo;

namespace rmw_fastrtps_cpp
{
    class ServiceTypeSupport : public TypeSupport
    {
        protected:

            ServiceTypeSupport();
    };

    class RequestTypeSupport : public ServiceTypeSupport
    {
        public:

            RequestTypeSupport(const rosidl_typesupport_introspection_cpp::ServiceMembers *members);
    };

    class ResponseTypeSupport : public ServiceTypeSupport
    {
        public:

            ResponseTypeSupport(const rosidl_typesupport_introspection_cpp::ServiceMembers *members);
    };
}

#endif // _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_
