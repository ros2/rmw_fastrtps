#include "rmw_fastrtps_cpp/ServiceTypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>

using namespace rmw_fastrtps_cpp;

ServiceTypeSupport::ServiceTypeSupport()
{
    m_isGetKeyDefined = false;
}

void ServiceTypeSupport::deleteData(void* data)
{
    assert(data);
    free(data);
}

bool ServiceTypeSupport::serializeROSmessage(eprosima::fastcdr::Cdr &ser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
        const void *ros_message)
{
    assert(members);
    assert(ros_message);

    for(unsigned long i = 0; i < members->member_count_; ++i)
    {
        const rosidl_typesupport_introspection_cpp::MessageMember *member = members->members_ + i;
        void *field = (char*)ros_message + member->offset_;
       
        if(!member->is_array_)
        {
            switch(member->type_id_) 
            {
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                    ser << *(bool*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                    ser << *(uint8_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    ser << *(char*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                    ser << *(float*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                    ser << *(double*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                    ser << *(int16_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    ser << *(uint16_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                    ser << *(int32_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    ser << *(uint32_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                    ser << *(int64_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    ser << *(uint64_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    ser << *(std::string*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member->members_->data;
                        serializeROSmessage(ser, sub_members, field);
                    }
                    break;
                default:
                    printf("unknown type id %u\n", member->type_id_);
                    throw std::runtime_error("unknown type");
            }
        }
        else
        {
            switch(member->type_id_) 
            {
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                    ser.serializeArray((bool*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                    ser.serializeArray((uint8_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    ser.serializeArray((char*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                    ser.serializeArray((float*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                    ser.serializeArray((double*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                    ser.serializeArray((int16_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    ser.serializeArray((uint16_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                    ser.serializeArray((int32_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    ser.serializeArray((uint32_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                    ser.serializeArray((int64_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    ser.serializeArray((uint64_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    ser.serializeArray((std::string*)field, member->array_size_);
                    break;
                default:
                    printf("unknown type id %u\n", member->type_id_);
                    throw std::runtime_error("unknown type");
            }
        }
    }

    return true;
}

bool ServiceTypeSupport::deserializeROSmessage(eprosima::fastcdr::Cdr &deser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
        void *ros_message)
{
    assert(members);
    assert(ros_message);

    for(unsigned long i = 0; i < members->member_count_; ++i)
    {
        const rosidl_typesupport_introspection_cpp::MessageMember *member = members->members_ + i;
        void *field = (char*)ros_message + member->offset_;
       
        if(!member->is_array_)
        {
            switch(member->type_id_) 
            {
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                    deser >> *(bool*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                    deser >> *(uint8_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    deser >> *(char*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                    deser >> *(float*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                    deser >> *(double*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                    deser >> *(int16_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    deser >> *(uint16_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                    deser >> *(int32_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    deser >> *(uint32_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                    deser >> *(int64_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    deser >> *(uint64_t*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    deser >> *(std::string*)field;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member->members_->data;
                        deserializeROSmessage(deser, sub_members, field);
                    }
                    break;
                default:
                    printf("unknown type id %u\n", member->type_id_);
                    throw std::runtime_error("unknown type");
            }
        }
        else
        {
            switch(member->type_id_) 
            {
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                    deser.deserializeArray((bool*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                    deser.deserializeArray((uint8_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    deser.deserializeArray((char*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                    deser.deserializeArray((float*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                    deser.deserializeArray((double*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                    deser.deserializeArray((int16_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    deser.deserializeArray((uint16_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                    deser.deserializeArray((int32_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    deser.deserializeArray((uint32_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                    deser.deserializeArray((int64_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    deser.deserializeArray((uint64_t*)field, member->array_size_);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    deser.deserializeArray((std::string*)field, member->array_size_);
                    break;
                default:
                    printf("unknown type id %u\n", member->type_id_);
                    throw std::runtime_error("unknown type");
            }
        }
    }

    return true;
}

size_t ServiceTypeSupport::calculateMaxSerializedSize(const rosidl_typesupport_introspection_cpp::MessageMembers *members, size_t current_alignment)
{
    assert(members);

    size_t initial_alignment = current_alignment;

    for(unsigned long i = 0; i < members->member_count_; ++i)
    {
        const rosidl_typesupport_introspection_cpp::MessageMember *member = members->members_ + i;
       
        if(!member->is_array_)
        {
            switch(member->type_id_) 
            {
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    current_alignment += 1;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    current_alignment += 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    current_alignment += 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + (member->string_upper_bound_ ? member->string_upper_bound_ + 1 : 256);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member->members_->data;
                        current_alignment += calculateMaxSerializedSize(sub_members, current_alignment);
                    }
                    break;
                default:
                    printf("unknown type id %u\n", member->type_id_);
                    throw std::runtime_error("unknown type");
            }
        }
        else
        {
            switch(member->type_id_) 
            {
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    current_alignment += member->array_size_;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    current_alignment += member->array_size_ * 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    current_alignment += member->array_size_ * 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    current_alignment += member->array_size_ * 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    {
                        for(size_t index = 0; index < member->array_size_; ++index)
                            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + (member->string_upper_bound_ ? member->string_upper_bound_ + 1 : 256);
                    }
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member->members_->data;
                        for(size_t index = 0; index < member->array_size_; ++index)
                            current_alignment += calculateMaxSerializedSize(sub_members, current_alignment);
                    }
                    break;
                default:
                    printf("unknown type id %u\n", member->type_id_);
                    throw std::runtime_error("unknown type");
            }
        }
    }

    return current_alignment - initial_alignment;
}

RequestTypeSupport::RequestTypeSupport(const rosidl_typesupport_introspection_cpp::ServiceMembers *members)
{
    assert(members);
    members_ = members->request_members_;
    std::string name = std::string(members->package_name_) + "_" + members->service_name_ + "_Request";
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

    ServiceTypeSupport::serializeROSmessage(ser, members_, ros_message);

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

    ServiceTypeSupport::deserializeROSmessage(deser, members_, ros_message);

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
    std::string name = std::string(members->package_name_) + "_" + members->service_name_ + "_Reply";
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

    ServiceTypeSupport::serializeROSmessage(ser, members_, ros_message);

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

    ServiceTypeSupport::deserializeROSmessage(deser, members_, ros_message);

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
