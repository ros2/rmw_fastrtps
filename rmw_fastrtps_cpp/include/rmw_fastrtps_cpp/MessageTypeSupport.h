#ifndef _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_

#include <fastrtps/TopicDataType.h>
#include "rosidl_typesupport_introspection_cpp/MessageIntrospection.h"

namespace eprosima { namespace fastcdr {
    class Cdr;
}}

namespace rmw_fastrtps_cpp
{
    class MessageTypeSupport : public eprosima::fastrtps::TopicDataType
    {
        public:

            typedef struct Buffer
            {
                uint32_t length;
                char *pointer;
            } Buffer;

            MessageTypeSupport(const rosidl_typesupport_introspection_cpp::MessageMembers *members);

            bool serialize(void *data, SerializedPayload_t *payload);

            bool deserialize(SerializedPayload_t *payload, void *data);

            void* createData();

            void deleteData(void* data);

            bool serializeROSmessage(const void *ros_message, void *data);

            bool deserializeROSmessage(const void* data, void *ros_message);

        private:

            size_t calculateMaxSerializedSize(const rosidl_typesupport_introspection_cpp::MessageMembers *members, size_t current_alignment);

            bool serializeROSmessage(eprosima::fastcdr::Cdr &ser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
                    const void *ros_message);

            bool deserializeROSmessage(eprosima::fastcdr::Cdr &deser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
                    void *ros_message);

            const rosidl_typesupport_introspection_cpp::MessageMembers *members_;
    };
}

#endif // _RMW_FASTRTPS_CPP_MESSAGETYPESUPPORT_H_
