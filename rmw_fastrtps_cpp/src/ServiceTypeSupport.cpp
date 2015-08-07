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
    std::string name = std::string(members->package_name_) + "::srv::dds_::" + members->service_name_ + "_Request_";
    setName(strdup(name.c_str()));
    m_typeSize = eprosima::rpc::protocol::dds::rpc::RequestHeader::getMaxCdrSerializedSize(0);
    m_typeSize += calculateMaxSerializedSize(members_, m_typeSize);
}

bool RequestTypeSupport::serialize(void *data, SerializedPayload_t *payload)
{
    assert(data);
    assert(payload);

    RequestBuffer *buffer = static_cast<RequestBuffer*>(data);
    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), m_typeSize);
    eprosima::fastcdr::Cdr ser(fastbuffer);

    // TODO First version. Review cause the alignment.
    ser << buffer->header;
    memcpy(ser.getCurrentPosition(), buffer->pointer, buffer->length);
    payload->length = ser.getSerializedDataLength() + buffer->length;
    return true;
}

bool RequestTypeSupport::deserialize(SerializedPayload_t *payload, void *data)
{
    assert(data);
    assert(payload);

    RequestBuffer *buffer = static_cast<RequestBuffer*>(data);
    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->length);
    eprosima::fastcdr::Cdr deser(fastbuffer);

    // TODO First version. Review cause the alignment.
    deser >> buffer->header;
    buffer->length = payload->length - deser.getSerializedDataLength();
    memcpy(buffer->pointer, deser.getCurrentPosition(), buffer->length);
    return true;
}

void* RequestTypeSupport::createData()
{
    RequestBuffer *buffer = static_cast<RequestBuffer*>(malloc(sizeof(Buffer) + m_typeSize));

    if(buffer)
    {
        new(buffer) RequestBuffer;
        buffer->length = 0;
        buffer->pointer = (char*)(buffer + 1);
    }

    return buffer;
}

bool RequestTypeSupport::serializeROSmessage(const void *ros_message, void *data)
{
    assert(data);
    assert(ros_message);

    RequestBuffer *buffer = static_cast<RequestBuffer*>(data);
    eprosima::fastcdr::FastBuffer fastbuffer(buffer->pointer, m_typeSize);
    eprosima::fastcdr::Cdr ser(fastbuffer);

    TypeSupport::serializeROSmessage(ser, members_, ros_message);

    buffer->length = (uint32_t)ser.getSerializedDataLength();
    return true;
}

bool RequestTypeSupport::deserializeROSmessage(const void *data, void *ros_message)
{
    assert(data);
    assert(ros_message);

    const RequestBuffer *buffer = static_cast<const RequestBuffer*>(data);
    eprosima::fastcdr::FastBuffer fastbuffer(buffer->pointer, buffer->length);
    eprosima::fastcdr::Cdr deser(fastbuffer);

    TypeSupport::deserializeROSmessage(deser, members_, ros_message, false);

    return true;
}

void* RequestTypeSupport::create_data(size_t dataSize)
{
    RequestBuffer *buffer = static_cast<RequestBuffer*>(malloc(sizeof(Buffer) + dataSize));

    if(buffer)
    {
        new(buffer) RequestBuffer;
        buffer->length = 0;
        buffer->pointer = (char*)(buffer + 1);
    }

    return buffer;
}

void RequestTypeSupport::delete_data(void *data)
{
    assert(data);
    free(data);
}

void RequestTypeSupport::copy_data(RequestBuffer *dst, const RequestBuffer *src)
{
    dst->header = src->header;
    dst->length = src->length;
    memcpy(dst->pointer, src->pointer, src->length);
}

ResponseTypeSupport::ResponseTypeSupport(const rosidl_typesupport_introspection_cpp::ServiceMembers *members)
{
    assert(members);
    members_ = members->response_members_;
    std::string name = std::string(members->package_name_) + "::srv::dds_::" + members->service_name_ + "_Response";
    setName(strdup(name.c_str()));
    m_typeSize = eprosima::rpc::protocol::dds::rpc::ReplyHeader::getMaxCdrSerializedSize(0);
    m_typeSize += calculateMaxSerializedSize(members_, m_typeSize);
}

bool ResponseTypeSupport::serialize(void *data, SerializedPayload_t *payload)
{
    assert(data);
    assert(payload);

    ResponseBuffer *buffer = static_cast<ResponseBuffer*>(data);
    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), m_typeSize);
    eprosima::fastcdr::Cdr ser(fastbuffer);

    ser << buffer->header;
    memcpy(ser.getCurrentPosition(), buffer->pointer, buffer->length);
    payload->length = ser.getSerializedDataLength() + buffer->length;
    return true;
}

bool ResponseTypeSupport::deserialize(SerializedPayload_t *payload, void *data)
{
    assert(data);
    assert(payload);

    ResponseBuffer *buffer = static_cast<ResponseBuffer*>(data);
    eprosima::fastcdr::FastBuffer fastbuffer(reinterpret_cast<char*>(payload->data), payload->length);
    eprosima::fastcdr::Cdr deser(fastbuffer);

    // TODO First version. Review cause the alignment.
    deser >> buffer->header;
    buffer->length = payload->length - deser.getSerializedDataLength();
    memcpy(buffer->pointer, deser.getCurrentPosition(), buffer->length);
    return true;
}

void* ResponseTypeSupport::createData()
{
    ResponseBuffer *buffer = static_cast<ResponseBuffer*>(malloc(sizeof(Buffer) + m_typeSize));

    if(buffer)
    {
        new(buffer) ResponseBuffer;
        buffer->length = 0;
        buffer->pointer = (char*)(buffer + 1);
    }

    return buffer;
}

bool ResponseTypeSupport::serializeROSmessage(const void *ros_message, void *data)
{
    assert(data);
    assert(ros_message);

    ResponseBuffer *buffer = static_cast<ResponseBuffer*>(data);
    eprosima::fastcdr::FastBuffer fastbuffer(buffer->pointer, m_typeSize);
    eprosima::fastcdr::Cdr ser(fastbuffer);

    TypeSupport::serializeROSmessage(ser, members_, ros_message);

    buffer->length = (uint32_t)ser.getSerializedDataLength();
    return true;
}

bool ResponseTypeSupport::deserializeROSmessage(const void *data, void *ros_message)
{
    assert(data);
    assert(ros_message);

    const ResponseBuffer *buffer = static_cast<const ResponseBuffer*>(data);
    eprosima::fastcdr::FastBuffer fastbuffer(buffer->pointer, buffer->length);
    eprosima::fastcdr::Cdr deser(fastbuffer);

    TypeSupport::deserializeROSmessage(deser, members_, ros_message, false);

    return true;
}

void* ResponseTypeSupport::create_data(size_t dataSize)
{
    ResponseBuffer *buffer = static_cast<ResponseBuffer*>(malloc(sizeof(Buffer) + dataSize));

    if(buffer)
    {
        new(buffer) ResponseBuffer;
        buffer->length = 0;
        buffer->pointer = (char*)(buffer + 1);
    }

    return buffer;
}

void ResponseTypeSupport::delete_data(void *data)
{
    assert(data);
    free(data);
}

void ResponseTypeSupport::copy_data(ResponseBuffer *dst, const ResponseBuffer *src)
{
    dst->header = src->header;
    dst->length = src->length;
    memcpy(dst->pointer, src->pointer, src->length);
}
