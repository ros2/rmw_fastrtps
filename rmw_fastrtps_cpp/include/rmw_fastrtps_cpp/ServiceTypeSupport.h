#ifndef _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_
#define _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_

#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"

#include <fastrtps/TopicDataType.h>
#include <rpcdds/protocols/dds/MessageHeader.h>
#include <rpcdds/protocols/Protocol.h>


struct CustomServiceInfo;

namespace eprosima { namespace fastcdr {
    class Cdr;
}}

namespace rmw_fastrtps_cpp
{
    class ServiceTypeSupport : public eprosima::fastrtps::TopicDataType
    {
        public:

            virtual bool serialize(void *data, SerializedPayload_t *payload) = 0;

            virtual bool deserialize(SerializedPayload_t *payload, void *data) = 0;

            virtual void* createData() = 0;

            void deleteData(void* data);

            virtual bool serializeROSmessage(const void *ros_message, void *data) = 0;

            virtual bool deserializeROSmessage(const void* data, void *ros_message) = 0;

        protected:

            ServiceTypeSupport();

            bool serializeROSmessage(eprosima::fastcdr::Cdr &ser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
                    const void *ros_message);

            bool deserializeROSmessage(eprosima::fastcdr::Cdr &deser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
                    void *ros_message);

            size_t calculateMaxSerializedSize(const rosidl_typesupport_introspection_cpp::MessageMembers *members, size_t current_alignment);

            const rosidl_typesupport_introspection_cpp::MessageMembers *members_;
    };

    class RequestTypeSupport : public ServiceTypeSupport
    {
        public:

            typedef struct RequestBuffer
            {
                eprosima::rpc::protocol::dds::rpc::RequestHeader header;
                uint32_t length;
                char *pointer;
            } Buffer;

            RequestTypeSupport(const rosidl_typesupport_introspection_cpp::ServiceMembers *members);

            bool serialize(void *data, SerializedPayload_t *payload);

            bool deserialize(SerializedPayload_t *payload, void *data);

            void* createData();

            bool serializeROSmessage(const void *ros_message, void *data);

            bool deserializeROSmessage(const void* data, void *ros_message);

            static void* create_data(size_t dataSize);

            static void delete_data(void *data);

            static void copy_data(RequestBuffer *dst, const RequestBuffer *src);
    };

    class ResponseTypeSupport : public ServiceTypeSupport
    {
        public:

            typedef struct ResponseBuffer
            {
                eprosima::rpc::protocol::dds::rpc::ReplyHeader header;
                uint32_t length;
                char *pointer;
            } Buffer;

            ResponseTypeSupport(const rosidl_typesupport_introspection_cpp::ServiceMembers *members);

            bool serialize(void *data, SerializedPayload_t *payload);

            bool deserialize(SerializedPayload_t *payload, void *data);

            void* createData();

            bool serializeROSmessage(const void *ros_message, void *data);

            bool deserializeROSmessage(const void* data, void *ros_message);

            static void* create_data(size_t dataSize);

            static void delete_data(void *data);

            static void copy_data(ResponseBuffer *dst, const ResponseBuffer *src);
    };

    class Protocol : public eprosima::rpc::protocol::Protocol
    {
        public:

            Protocol(CustomServiceInfo *info) : info_(info) {}

            struct CustomServiceInfo* getInfo() const { return info_; }

        private:

            bool setTransport(eprosima::rpc::transport::Transport&) {}

            struct CustomServiceInfo *info_;
    };
}

#endif // _RMW_FASTRTPS_CPP_SERVICETYPESUPPORT_H_
