#include "rmw_fastrtps_cpp/TypeSupport.h"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"

#include <fastcdr/FastBuffer.h>
#include <fastcdr/Cdr.h>
#include <cassert>

using namespace rmw_fastrtps_cpp;

TypeSupport::TypeSupport() : typeTooLarge_(false)
{
    m_isGetKeyDefined = false;
}

void TypeSupport::deleteData(void* data)
{
    assert(data);
    free(data);
}

static inline void*
align_(size_t __align, size_t __size, void*& __ptr, size_t& __space) noexcept
{
    const auto __intptr = reinterpret_cast<uintptr_t>(__ptr);
    const auto __aligned = (__intptr - 1u + __align) & -__align;
    const auto __diff = __aligned - __intptr;
    if ((__size + __diff) > __space)
        return nullptr;
    else
    {
        __space -= __diff;
        return __ptr = reinterpret_cast<void*>(__aligned);
    }
}

static size_t calculateMaxAlign(const rosidl_typesupport_introspection_cpp::MessageMembers *members)
{
    size_t max_align = 0;

    for(unsigned long i = 0; i < members->member_count_; ++i)
    {
        size_t alignment = 0;
        const rosidl_typesupport_introspection_cpp::MessageMember &member = members->members_[i];

        if(member.is_array_ && (!member.array_size_ || member.is_upper_bound_))
        {
            alignment = alignof(std::vector<unsigned char>);
        }
        else
        {
            switch(member.type_id_)
            {
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                    alignment = alignof(bool);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                    alignment = alignof(uint8_t);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    alignment = alignof(char);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                    alignment = alignof(float);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                    alignment = alignof(double);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                    alignment = alignof(int16_t);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    alignment = alignof(uint16_t);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                    alignment = alignof(int32_t);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    alignment = alignof(uint32_t);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                    alignment = alignof(int64_t);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    alignment = alignof(uint64_t);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    alignment = alignof(std::string);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member.members_->data;
                        alignment = calculateMaxAlign(sub_members);
                    }
                    break;
            }
        }

        if(alignment > max_align)
            max_align = alignment;
    }

    return max_align;
}

static size_t size_of(const rosidl_typesupport_introspection_cpp::MessageMembers *members)
{
    size_t size = 0;

    const rosidl_typesupport_introspection_cpp::MessageMember &last_member = members->members_[members->member_count_ - 1];

    if(last_member.is_array_ && (!last_member.array_size_ || last_member.is_upper_bound_))
    {
        size = sizeof(std::vector<unsigned char>);
    }
    else
    {
        switch(last_member.type_id_)
        {
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                size = sizeof(bool);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                size = sizeof(uint8_t);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                size = sizeof(char);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                size = sizeof(float);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                size = sizeof(double);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                size = sizeof(int16_t);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                size = sizeof(uint16_t);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                size = sizeof(int32_t);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                size = sizeof(uint32_t);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                size = sizeof(int64_t);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                size = sizeof(uint64_t);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                size = sizeof(std::string);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                {
                    const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)last_member.members_->data;
                    size = size_of(sub_members);
                }
                break;
        }

        if(last_member.is_array_)
            size *= last_member.array_size_;
    }

    return last_member.offset_ + size;
}

#define SER_ARRAY_SIZE_AND_VALUES(TYPE) \
{ \
    if (member->array_size_ && !member->is_upper_bound_) { \
        ser.serializeArray((TYPE*)field, member->array_size_); \
    } else { \
        std::vector<TYPE> &vector = *reinterpret_cast<std::vector<TYPE> *>(field); \
        if(vector.size() > (member->is_upper_bound_ ? member->array_size_ : (typeByDefaultLarge() ? 30 : 101))) { \
            printf("vector overcomes the maximum length\n"); \
            throw std::runtime_error("vector overcomes the maximum length"); \
        } \
        ser << vector; \
    } \
}

bool TypeSupport::serializeROSmessage(eprosima::fastcdr::Cdr &ser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
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
                    {
                        bool sb = false, &b = *(bool*)field;
                        if(b) sb = true; 
                        ser << sb;
                    }
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
                    {
                        std::string &str = *(std::string*)field;

                        // Control maximum length.
                        if((member->string_upper_bound_ && str.length() > member->string_upper_bound_ + 1) || str.length() > 256)
                        {
                            printf("string overcomes the maximum length with length %zu\n", str.length());
                            throw std::runtime_error("string overcomes the maximum length");
                        }
                        ser << str;
                    }
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
                    SER_ARRAY_SIZE_AND_VALUES(bool)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                    SER_ARRAY_SIZE_AND_VALUES(uint8_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    SER_ARRAY_SIZE_AND_VALUES(char)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                    SER_ARRAY_SIZE_AND_VALUES(float)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                    SER_ARRAY_SIZE_AND_VALUES(double)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                    SER_ARRAY_SIZE_AND_VALUES(int16_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    SER_ARRAY_SIZE_AND_VALUES(uint16_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                    SER_ARRAY_SIZE_AND_VALUES(int32_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    SER_ARRAY_SIZE_AND_VALUES(uint32_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                    SER_ARRAY_SIZE_AND_VALUES(int64_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    SER_ARRAY_SIZE_AND_VALUES(uint64_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    SER_ARRAY_SIZE_AND_VALUES(std::string)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member->members_->data;
                        void *subros_message = nullptr;
                        size_t array_size = 0;
                        size_t sub_members_size = size_of(sub_members);
                        size_t max_align = calculateMaxAlign(sub_members);
                        size_t space = 100;

                        if (member->array_size_ && !member->is_upper_bound_)
                        {
                            subros_message = field;
                            array_size = member->array_size_;
                        }
                        else
                        {
                            std::vector<unsigned char> *vector = reinterpret_cast<std::vector<unsigned char> *>(field);
                            void *ptr = (void*)sub_members_size;
                            size_t vsize = vector->size() / (size_t)align_(max_align, 0, ptr, space);
                            if(vsize > (member->is_upper_bound_ ? member->array_size_ : (typeByDefaultLarge() ? 30 : 101))) {
                                printf("vector overcomes the maximum length\n");
                                throw std::runtime_error("vector overcomes the maximum length");
                            }
                            subros_message = reinterpret_cast<void*>(vector->data());
                            array_size = vsize;

                            // Serialize length
                            ser << (uint32_t)vsize;
                        }

                        for(size_t index = 0; index < array_size; ++index)
                        {
                            serializeROSmessage(ser, sub_members, subros_message);
                            subros_message = (char*)subros_message + sub_members_size;
                            // TODO Change 100 values.
                            subros_message = align_(max_align, 0, subros_message, space);
                        }
                    }
                    break;
                default:
                    printf("unknown type id %u\n", member->type_id_);
                    throw std::runtime_error("unknown type");
            }
        }
    }

    return true;
}

#define DESER_ARRAY_SIZE_AND_VALUES(TYPE) \
{ \
    if (member->array_size_ && !member->is_upper_bound_) { \
        deser.deserializeArray((TYPE*)field, member->array_size_); \
    } else { \
        std::vector<TYPE> &vector = *reinterpret_cast<std::vector<TYPE> *>(field); \
        if(call_new) \
        new(&vector) std::vector<TYPE>; \
        deser >> vector; \
    } \
}

bool TypeSupport::deserializeROSmessage(eprosima::fastcdr::Cdr &deser, const rosidl_typesupport_introspection_cpp::MessageMembers *members,
        void *ros_message, bool call_new)
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
                    {
                        deser >> *(bool*)field;
                    }
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
                    {
                        std::string &str = *(std::string*)field;
                        if(call_new)
                            new(&str) std::string;
                        deser >> str;
                    }
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member->members_->data;
                        deserializeROSmessage(deser, sub_members, field, call_new);
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
                    DESER_ARRAY_SIZE_AND_VALUES(bool)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                    DESER_ARRAY_SIZE_AND_VALUES(uint8_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    DESER_ARRAY_SIZE_AND_VALUES(char)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                    DESER_ARRAY_SIZE_AND_VALUES(float)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                    DESER_ARRAY_SIZE_AND_VALUES(double)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                    DESER_ARRAY_SIZE_AND_VALUES(int16_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    DESER_ARRAY_SIZE_AND_VALUES(uint16_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                    DESER_ARRAY_SIZE_AND_VALUES(int32_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    DESER_ARRAY_SIZE_AND_VALUES(uint32_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                    DESER_ARRAY_SIZE_AND_VALUES(int64_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    DESER_ARRAY_SIZE_AND_VALUES(uint64_t)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    DESER_ARRAY_SIZE_AND_VALUES(std::string)
                        break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member->members_->data;
                        void *subros_message = nullptr;
                        size_t array_size = 0;
                        size_t sub_members_size = size_of(sub_members);
                        size_t max_align = calculateMaxAlign(sub_members);
                        size_t space = 100;
                        bool recall_new = call_new;

                        if (member->array_size_ && !member->is_upper_bound_)
                        {
                            subros_message = field;
                            array_size = member->array_size_;
                        }
                        else
                        {
                            uint32_t vsize = 0;
                            // Deserialize length
                            deser >> vsize;
                            std::vector<unsigned char> *vector = reinterpret_cast<std::vector<unsigned char> *>(field);
                            if(call_new)
                                new(vector) std::vector<unsigned char>;
                            void *ptr = (void*)sub_members_size;
                            vector->resize(vsize * (size_t)align_(max_align, 0, ptr, space));
                            subros_message = reinterpret_cast<void*>(vector->data());
                            array_size = vsize;
                            recall_new = true;
                        }

                        for(size_t index = 0; index < array_size; ++index)
                        {
                            deserializeROSmessage(deser, sub_members, subros_message, recall_new);
                            subros_message = (char*)subros_message + sub_members_size;
                            // TODO Change 100 values.
                            subros_message = align_(max_align, 0, subros_message, space);
                        }
                    }
                    break;
                default:
                    printf("unknown type id %u\n", member->type_id_);
                    throw std::runtime_error("unknown type");
            }
        }
    }

    return true;
}

size_t TypeSupport::calculateMaxSerializedSize(const rosidl_typesupport_introspection_cpp::MessageMembers *members, size_t current_alignment)
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
                    current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + (member->string_upper_bound_ ? member->string_upper_bound_ + 1 : 257);
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
            size_t array_size = member->array_size_;

            // Whether it is a sequence.
            if(array_size == 0 || member->is_upper_bound_)
            {
                current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);

                if(array_size == 0) typeByDefaultLarge() ? array_size = 30 : array_size = 101;
            }

            switch(member->type_id_) 
            {
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                    current_alignment += array_size;
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                    current_alignment += array_size * 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                    current_alignment += array_size * 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                    current_alignment += array_size * 8 + eprosima::fastcdr::Cdr::alignment(current_alignment, 8);
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                    {
                        for(size_t index = 0; index < array_size; ++index)
                            current_alignment += 4 + eprosima::fastcdr::Cdr::alignment(current_alignment, 4) + (member->string_upper_bound_ ? member->string_upper_bound_ + 1 : 257);
                    }
                    break;
                case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE:
                    {
                        const ::rosidl_typesupport_introspection_cpp::MessageMembers *sub_members = (const ::rosidl_typesupport_introspection_cpp::MessageMembers*)member->members_->data;
                        for(size_t index = 0; index < array_size; ++index)
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


bool TypeSupport::serializeROSmessage(const void *ros_message, Buffer *buffer)
{
    assert(buffer);
    assert(ros_message);

    eprosima::fastcdr::FastBuffer fastbuffer(buffer->pointer, m_typeSize);
    eprosima::fastcdr::Cdr ser(fastbuffer);

    if(members_->member_count_ != 0)
        TypeSupport::serializeROSmessage(ser, members_, ros_message);
    else
        ser << (uint8_t)0;

    buffer->length = (uint32_t)ser.getSerializedDataLength();
    return true;
}

bool TypeSupport::deserializeROSmessage(const Buffer *buffer, void *ros_message)
{
    assert(buffer);
    assert(ros_message);

    eprosima::fastcdr::FastBuffer fastbuffer(buffer->pointer, buffer->length);
    eprosima::fastcdr::Cdr deser(fastbuffer);

    if(members_->member_count_ != 0)
        TypeSupport::deserializeROSmessage(deser, members_, ros_message, false);
    else
    {
        uint8_t dump;
        deser >> dump;
    }

    return true;
}

void* TypeSupport::createData()
{
    Buffer *buffer = static_cast<Buffer*>(malloc(sizeof(Buffer) + m_typeSize));

    if(buffer)
    {
        buffer->length = 0;
        buffer->pointer = (char*)(buffer + 1);
    }

    return buffer;
}

bool TypeSupport::serialize(void *data, SerializedPayload_t *payload)
{
    assert(data);
    assert(payload);

    Buffer *buffer = static_cast<Buffer*>(data);
    payload->length = buffer->length;
    payload->encapsulation = CDR_LE;
    memcpy(payload->data, buffer->pointer, buffer->length);
    return true;
}

bool TypeSupport::deserialize(SerializedPayload_t *payload, void *data)
{
    assert(data);
    assert(payload);

    Buffer *buffer = static_cast<Buffer*>(data);
    buffer->length = payload->length;
    memcpy(buffer->pointer, payload->data, payload->length);
    return true;
}
