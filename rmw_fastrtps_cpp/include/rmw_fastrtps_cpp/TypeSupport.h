#ifndef _RMW_FASTRTPS_CPP_TYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_TYPESUPPORT_H_

#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"

#include <fastrtps/TopicDataType.h>

namespace eprosima { namespace fastcdr {
    class Cdr;
}}

namespace rmw_fastrtps_cpp
{
    class TypeSupport : public eprosima::fastrtps::TopicDataType
    {
        public:

            virtual bool serialize(void *data, SerializedPayload_t *payload) = 0;

            virtual bool deserialize(SerializedPayload_t *payload, void *data) = 0;

            virtual void* createData() = 0;

            void deleteData(void* data);

        protected:

            TypeSupport();

            bool serializeROSmessage(eprosima::fastcdr::Cdr &ser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
                    const void *ros_message);

            bool deserializeROSmessage(eprosima::fastcdr::Cdr &deser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
                    void *ros_message, bool call_new);

            size_t calculateMaxSerializedSize(const rosidl_typesupport_introspection_cpp::MessageMembers *members, size_t current_alignment);

            const rosidl_typesupport_introspection_cpp::MessageMembers *members_;
    };
}

#endif // _RMW_FASTRTPS_CPP_TYPESUPPORT_H_

