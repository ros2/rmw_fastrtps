// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rmw/allocators.h"
#include <rmw/rmw.h>
#include <rmw/error_handling.h>
#include <rmw/impl/cpp/macros.hpp>
#include <rmw_fastrtps_cpp/MessageTypeSupport.h>
#include <rmw_fastrtps_cpp/ServiceTypeSupport.h>

#include <fastrtps/Domain.h>
#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/publisher/PublisherListener.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/subscriber/SubscriberListener.h>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/attributes/SubscriberAttributes.h>



#include <fastrtps/rtps/RTPSDomain.h>
#include <fastrtps/rtps/builtin/data/WriterProxyData.h>
#include <fastrtps/rtps/common/CDRMessage_t.h>

#include <fastrtps/rtps/reader/RTPSReader.h> 
#include <fastrtps/rtps/reader/StatefulReader.h>
#include <fastrtps/rtps/reader/ReaderListener.h>
#include <fastrtps/rtps/builtin/discovery/endpoint/EDPSimple.h>

#include <cassert>
#include <mutex>
#include <condition_variable>
#include <list>

#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/service_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "rosidl_typesupport_introspection_c/service_introspection.h"
#include "rosidl_typesupport_introspection_c/visibility_control.h"

using namespace eprosima::fastrtps;

using MessageTypeSupport_c = rmw_fastrtps_cpp::MessageTypeSupport<rosidl_typesupport_introspection_c__MessageMembers>;
using MessageTypeSupport_cpp = rmw_fastrtps_cpp::MessageTypeSupport<rosidl_typesupport_introspection_cpp::MessageMembers>;
using TypeSupport_c = rmw_fastrtps_cpp::TypeSupport<rosidl_typesupport_introspection_c__MessageMembers>;
using TypeSupport_cpp = rmw_fastrtps_cpp::TypeSupport<rosidl_typesupport_introspection_cpp::MessageMembers>;

using RequestTypeSupport_c = rmw_fastrtps_cpp::RequestTypeSupport<
    rosidl_typesupport_introspection_c__ServiceMembers,
    rosidl_typesupport_introspection_c__MessageMembers
>;
using RequestTypeSupport_cpp = rmw_fastrtps_cpp::RequestTypeSupport<
    rosidl_typesupport_introspection_cpp::ServiceMembers,
    rosidl_typesupport_introspection_cpp::MessageMembers
>;

using ResponseTypeSupport_c = rmw_fastrtps_cpp::ResponseTypeSupport<
    rosidl_typesupport_introspection_c__ServiceMembers,
    rosidl_typesupport_introspection_c__MessageMembers
>;
using ResponseTypeSupport_cpp = rmw_fastrtps_cpp::ResponseTypeSupport<
    rosidl_typesupport_introspection_cpp::ServiceMembers,
    rosidl_typesupport_introspection_cpp::MessageMembers
>;

bool using_introspection_c_typesupport(const char * typesupport_identifier)
{
    return typesupport_identifier == rosidl_typesupport_introspection_c__identifier;
}

bool using_introspection_cpp_typesupport(const char * typesupport_identifier)
{
    return typesupport_identifier == rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier;
}

template<typename MembersType>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_LOCAL
inline std::string
_create_type_name(
    const void * untyped_members,
    const std::string & sep)
{
    auto members = static_cast<const MembersType *>(untyped_members);
    if (!members) {
        RMW_SET_ERROR_MSG("members handle is null");
        return "";
    }
    return
        std::string(members->package_name_) + "::" + sep + "::dds_::" + members->message_name_ + "_";
}

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_LOCAL
inline std::string
_create_type_name(
  const void * untyped_members,
  const std::string & sep,
  const char * typesupport)
{
    if (using_introspection_c_typesupport(typesupport)) {
        return _create_type_name<rosidl_typesupport_introspection_c__MessageMembers>(
            untyped_members, sep);
    } else if (using_introspection_cpp_typesupport(typesupport)) {
        return _create_type_name<rosidl_typesupport_introspection_cpp::MessageMembers>(
            untyped_members, sep);
    }
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    return "";
}

template<typename ServiceType>
const void * get_request_ptr(const void * untyped_service_members)
{
    auto service_members = static_cast<const ServiceType *>(untyped_service_members);
    if (!service_members) {
        RMW_SET_ERROR_MSG("service members handle is null");
        return NULL;
    }
    return service_members->request_members_;
}

const void * get_request_ptr(const void * untyped_service_members, const char * typesupport)
{
    if (using_introspection_c_typesupport(typesupport)) {
        return get_request_ptr<rosidl_typesupport_introspection_c__ServiceMembers>(
            untyped_service_members);
    } else if (using_introspection_cpp_typesupport(typesupport)) {
        return get_request_ptr<rosidl_typesupport_introspection_cpp::ServiceMembers>(
            untyped_service_members);
    }
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    return NULL;
}

template<typename ServiceType>
const void * get_response_ptr(const void * untyped_service_members)
{
    auto service_members = static_cast<const ServiceType *>(untyped_service_members);
    if (!service_members) {
        RMW_SET_ERROR_MSG("service members handle is null");
        return NULL;
    }
    return service_members->response_members_;
}

const void * get_response_ptr(const void * untyped_service_members, const char * typesupport)
{
    if (using_introspection_c_typesupport(typesupport)) {
        return get_response_ptr<rosidl_typesupport_introspection_c__ServiceMembers>(
            untyped_service_members);
    } else if (using_introspection_cpp_typesupport(typesupport)) {
        return get_response_ptr<rosidl_typesupport_introspection_cpp::ServiceMembers>(
            untyped_service_members);
    }
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    return NULL;
}

void *
_create_message_type_support(const void * untyped_members, const char * typesupport_identifier)
{
    if (using_introspection_c_typesupport(typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_c__MessageMembers*>(
            untyped_members);
        return new MessageTypeSupport_c(members);
    } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers*>(
            untyped_members);
        return new MessageTypeSupport_cpp(members);
    }
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    return nullptr;
}

void *
_create_request_type_support(const void * untyped_members, const char * typesupport_identifier)
{
    if (using_introspection_c_typesupport(typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers*>(
            untyped_members);
        return new RequestTypeSupport_c(members);
    } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
        auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers*>(
            untyped_members);
        return new RequestTypeSupport_cpp(members);
    }
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    return nullptr;
}

void *
_create_response_type_support(const void * untyped_members, const char * typesupport_identifier)
{
    if (using_introspection_c_typesupport(typesupport_identifier)) {
      auto members = static_cast<const rosidl_typesupport_introspection_c__ServiceMembers*>(
          untyped_members);
      return new ResponseTypeSupport_c(members);
    } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
      auto members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers*>(
          untyped_members);
      return new ResponseTypeSupport_cpp(members);
    }
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    return nullptr;
}

void
_register_type(
    eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
    const char* typesupport_identifier)
{
    if (using_introspection_c_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
        Domain::registerType(participant, typed_typesupport);
    } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
        Domain::registerType(participant, typed_typesupport);
    } else {
        RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    }
}

void
_unregister_type(
    eprosima::fastrtps::Participant * participant, void * untyped_typesupport,
    const char* typesupport_identifier)
{
    if (using_introspection_c_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
        if(Domain::unregisterType(participant, typed_typesupport->getName()))
            delete typed_typesupport;
    } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
        if(Domain::unregisterType(participant, typed_typesupport->getName()))
            delete typed_typesupport;
    } else {
        RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    }
}

void
_delete_typesupport(void * untyped_typesupport, const char* typesupport_identifier)
{
    if (using_introspection_c_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
        if (typed_typesupport != nullptr)
            delete typed_typesupport;
    } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<MessageTypeSupport_cpp *>(untyped_typesupport);
        if (typed_typesupport != nullptr)
            delete typed_typesupport;
    } else {
        RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    }
}

bool
_serialize_ros_message(
    const void *ros_message, eprosima::fastcdr::Cdr& ser, void * untyped_typesupport,
    const char* typesupport_identifier)
{
    if (using_introspection_c_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<MessageTypeSupport_c *>(untyped_typesupport);
        return typed_typesupport->serializeROSmessage(ros_message, ser);
    } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<MessageTypeSupport_cpp *>(untyped_typesupport);
        return typed_typesupport->serializeROSmessage(ros_message, ser);
    }
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    return false;
}

bool
_deserialize_ros_message(
    eprosima::fastcdr::FastBuffer * buffer, void *ros_message, void * untyped_typesupport,
    const char* typesupport_identifier)
{
    if (using_introspection_c_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<TypeSupport_c *>(untyped_typesupport);
        return typed_typesupport->deserializeROSmessage(buffer, ros_message);
    } else if (using_introspection_cpp_typesupport(typesupport_identifier)) {
        auto typed_typesupport = static_cast<TypeSupport_cpp *>(untyped_typesupport);
        return typed_typesupport->deserializeROSmessage(buffer, ros_message);
    }
    RMW_SET_ERROR_MSG("Unknown typesupport identifier");
    return false;
}

class ClientListener;

typedef struct CustomWaitsetInfo
{
    std::condition_variable condition;
    std::mutex condition_mutex;
} CustomWaitsetInfo;

typedef struct CustomClientInfo
{
    void *request_type_support_;
    void *response_type_support_;
    Subscriber *response_subscriber_;
    Publisher *request_publisher_;
    ClientListener *listener_;
    eprosima::fastrtps::rtps::GUID_t writer_guid_;
    Participant *participant_;
    const char *typesupport_identifier_;
} CustomClientInfo;

typedef struct CustomClientResponse
{
    eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
    eprosima::fastcdr::FastBuffer *buffer_;

    CustomClientResponse() : buffer_(nullptr) {}
} CustomClientResponse;

class topicnamesandtypesReaderListener : public ReaderListener {
	public:
	topicnamesandtypesReaderListener(){};
	void onNewCacheChangeAdded(RTPSReader* reader, const CacheChange_t* const change_in){
		CacheChange_t* change = (CacheChange_t*) change_in;
		if(change->kind == ALIVE){
			WriterProxyData proxyData;
			CDRMessage_t tempMsg;
			tempMsg.msg_endian = change->serializedPayload.encapsulation == PL_CDR_BE ? BIGEND:LITTLEEND;
			tempMsg.length = change->serializedPayload.length;
			memcpy(tempMsg.buffer,change->serializedPayload.data,tempMsg.length);
			if(proxyData.readFromCDRMessage(&tempMsg)){
				mapmutex.lock();
				topicNtypes[proxyData.topicName()].insert(proxyData.typeName());		
				mapmutex.unlock();
			}
		}
	}
	std::map<std::string,std::set<std::string>> topicNtypes;
	std::mutex mapmutex;
};

class ClientListener : public SubscriberListener
{
    public:

        ClientListener(CustomClientInfo *info) : info_(info),
        conditionMutex_(NULL), conditionVariable_(NULL) {}


        void onNewDataMessage(Subscriber *sub)
        {
            assert(sub);

            CustomClientResponse response;
            response.buffer_ = new eprosima::fastcdr::FastBuffer();
            SampleInfo_t sinfo;

            if(sub->takeNextData(response.buffer_, &sinfo))
            {
                if(sinfo.sampleKind == ALIVE)
                {
                    response.sample_identity_ = sinfo.related_sample_identity;

                    if(info_->writer_guid_ == response.sample_identity_.writer_guid())
                    {
                        std::lock_guard<std::mutex> lock(internalMutex_);

                        if(conditionMutex_ != NULL)
                        {
                            std::unique_lock<std::mutex> clock(*conditionMutex_);
                            list.push_back(response);
                            clock.unlock();
                            conditionVariable_->notify_one();
                        }
                        else
                            list.push_back(response);
                    }
                }
            }
        }

        CustomClientResponse getResponse()
        {
            std::lock_guard<std::mutex> lock(internalMutex_);
            CustomClientResponse response;

            if(conditionMutex_ != NULL)
            {
                std::unique_lock<std::mutex> clock(*conditionMutex_);
                if(!list.empty())
                {
                    response = list.front();
                    list.pop_front();
                }
            }
            else
            {
                if(!list.empty())
                {
                    response = list.front();
                    list.pop_front();
                }
            }

            return response;
        }

        void attachCondition(std::mutex *conditionMutex, std::condition_variable *conditionVariable)
        {
            std::lock_guard<std::mutex> lock(internalMutex_);
            conditionMutex_ = conditionMutex;
            conditionVariable_ = conditionVariable;
        }

        void dettachCondition()
        {
            std::lock_guard<std::mutex> lock(internalMutex_);
            conditionMutex_ = NULL;
            conditionVariable_ = NULL;
        }

        bool hasData()
        {
            return !list.empty();
        }

    private:

        CustomClientInfo *info_;
        std::mutex internalMutex_;
        std::list<CustomClientResponse> list;
        std::mutex *conditionMutex_;
        std::condition_variable *conditionVariable_;
};

typedef struct CustomParticipantInfo{
    Participant * participant;
    topicnamesandtypesReaderListener *secondarySubListener;
    topicnamesandtypesReaderListener *secondaryPubListener;
    rmw_guard_condition_t * graph_guard_condition;
} CustomParticipantInfo;

extern "C"
{
    const char* const eprosima_fastrtps_identifier = "rmw_fastrtps_cpp";

    const char* rmw_get_implementation_identifier()
    {
        return eprosima_fastrtps_identifier;
    }

    bool get_datareader_qos(const rmw_qos_profile_t& qos_policies, SubscriberAttributes& sattr)
    {
        switch (qos_policies.history) {
            case RMW_QOS_POLICY_KEEP_LAST_HISTORY:
                sattr.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
                break;
            case RMW_QOS_POLICY_KEEP_ALL_HISTORY:
                sattr.topic.historyQos.kind = KEEP_ALL_HISTORY_QOS;
                break;
            case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
                break;
            default:
                RMW_SET_ERROR_MSG("Unknown QoS history policy");
                return false;
        }

        switch (qos_policies.reliability)
        {
            case RMW_QOS_POLICY_BEST_EFFORT:
                sattr.qos.m_reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
                break;
            case RMW_QOS_POLICY_RELIABLE:
                sattr.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
                break;
            case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
                break;
            default:
                RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
                return false;
        }

        switch (qos_policies.durability)
        {
            case RMW_QOS_POLICY_TRANSIENT_LOCAL_DURABILITY:
                sattr.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
                break;
            case RMW_QOS_POLICY_VOLATILE_DURABILITY:
                sattr.qos.m_durability.kind = VOLATILE_DURABILITY_QOS;
                break;
            case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
                break;
            default:
                RMW_SET_ERROR_MSG("Unknown QoS durability policy");
                return false;
        }

        if (qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
            sattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
        }

        // ensure the history depth is at least the requested queue size
        assert(sattr.topic.historyQos.depth >= 0);
        if (
                sattr.topic.historyQos.kind == KEEP_LAST_HISTORY_QOS &&
                static_cast<size_t>(sattr.topic.historyQos.depth) < qos_policies.depth
           )
        {
            if (qos_policies.depth > (std::numeric_limits<int32_t>::max)()) {
                RMW_SET_ERROR_MSG(
                        "failed to set history depth since the requested queue size exceeds the DDS type");
                return false;
            }
            sattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
        }

        return true;
    }

    bool get_datawriter_qos(const rmw_qos_profile_t& qos_policies, PublisherAttributes& pattr)
    {
        switch (qos_policies.history)
        {
            case RMW_QOS_POLICY_KEEP_LAST_HISTORY:
                pattr.topic.historyQos.kind = KEEP_LAST_HISTORY_QOS;
                break;
            case RMW_QOS_POLICY_KEEP_ALL_HISTORY:
                pattr.topic.historyQos.kind = KEEP_ALL_HISTORY_QOS;
                break;
            case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
                break;
            default:
                RMW_SET_ERROR_MSG("Unknown QoS history policy");
                return false;
        }

        switch (qos_policies.durability)
        {
            case RMW_QOS_POLICY_TRANSIENT_LOCAL_DURABILITY:
                pattr.qos.m_durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
                break;
            case RMW_QOS_POLICY_VOLATILE_DURABILITY:
                pattr.qos.m_durability.kind = VOLATILE_DURABILITY_QOS;
                break;
            case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
                break;
            default:
                RMW_SET_ERROR_MSG("Unknown QoS durability policy");
                return false;
        }

        switch (qos_policies.reliability)
        {
            case RMW_QOS_POLICY_BEST_EFFORT:
                pattr.qos.m_reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
                break;
            case RMW_QOS_POLICY_RELIABLE:
                pattr.qos.m_reliability.kind = RELIABLE_RELIABILITY_QOS;
                break;
            case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
                break;
            default:
                RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
                return false;
        }

        if (qos_policies.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
            pattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
        }

        // ensure the history depth is at least the requested queue size
        assert(pattr.topic.historyQos.depth >= 0);
        if (
                pattr.topic.historyQos.kind == KEEP_LAST_HISTORY_QOS &&
                static_cast<size_t>(pattr.topic.historyQos.depth) < qos_policies.depth
           )
        {
            if (qos_policies.depth > (std::numeric_limits<int32_t>::max)()) {
                RMW_SET_ERROR_MSG(
                        "failed to set history depth since the requested queue size exceeds the DDS type");
                return false;
            }
            pattr.topic.historyQos.depth = static_cast<int32_t>(qos_policies.depth);
        }

        return true;
    }

    rmw_ret_t rmw_init()
    {
        return RMW_RET_OK;
    }
    
    rmw_node_t* rmw_create_node(const char *name, size_t domain_id)
    {
        if (!name) {
            RMW_SET_ERROR_MSG("name is null");
            return NULL;
        }

        eprosima::fastrtps::Log::SetVerbosity(eprosima::fastrtps::Log::Error);

        ParticipantAttributes participantParam;
        participantParam.rtps.builtin.domainId = static_cast<uint32_t>(domain_id);
        participantParam.rtps.setName(name);

        Participant *participant = Domain::createParticipant(participantParam);
        if(!participant)
        {
            RMW_SET_ERROR_MSG("create_node() could not create participant");
            return NULL;
        }

        rmw_guard_condition_t * graph_guard_condition = rmw_create_guard_condition();
        if (!graph_guard_condition) {
            // error already set
            return NULL;
        }

        CustomParticipantInfo* node_impl = nullptr;
        try {
            node_impl = new CustomParticipantInfo();
        } catch(std::bad_alloc) {
            RMW_SET_ERROR_MSG("failed to allocate node impl struct");
            return NULL;
        }

        rmw_node_t * node_handle =
            static_cast<rmw_node_t *>(malloc(sizeof(rmw_node_t)));
        if (!node_handle) {
            RMW_SET_ERROR_MSG("failed to allocate rmw_node_t");
            return NULL;
        }
        node_handle->implementation_identifier = eprosima_fastrtps_identifier;
        node_impl->participant = participant;
        node_impl->graph_guard_condition = graph_guard_condition;
        node_handle->data = node_impl;


        node_handle->name =
            static_cast<const char *>(malloc(sizeof(char) * strlen(name) + 1));
        if (!node_handle->name) {
            RMW_SET_ERROR_MSG("failed to allocate memory");
            free(static_cast<void*>(node_handle));
            return NULL;
        }
        memcpy(const_cast<char *>(node_handle->name), name, strlen(name) + 1);
        
	topicnamesandtypesReaderListener* tnat_1 = new topicnamesandtypesReaderListener();
	topicnamesandtypesReaderListener* tnat_2 = new topicnamesandtypesReaderListener();

	node_impl->secondarySubListener = tnat_1;
	node_impl->secondaryPubListener = tnat_2;


 	std::pair<StatefulReader*, StatefulReader*> EDPReaders = participant->getEDPReaders();
	
	if( !( EDPReaders.first->setListener(tnat_1) & EDPReaders.second->setListener(tnat_2) ) ){
		RMW_SET_ERROR_MSG("Failed to attach ROS related logic to the Participant");
		goto fail;	
	}

        return node_handle;
    	fail:
	delete(tnat_1);
	delete(tnat_2);
	delete(node_impl);
	return NULL;

    }

    rmw_ret_t rmw_destroy_node(rmw_node_t * node)
    {
        if (!node) {
            RMW_SET_ERROR_MSG("node handle is null");
            return RMW_RET_ERROR;
        }

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomParticipantInfo* impl = static_cast<CustomParticipantInfo *>(node->data);
        if (!impl) {
            RMW_SET_ERROR_MSG("node impl is null");
            return RMW_RET_ERROR;
        }

        Participant *participant = impl->participant;

	std::pair<StatefulReader*,StatefulReader*> EDPReaders = participant->getEDPReaders();
	EDPReaders.first->setListener(nullptr);
	delete(impl->secondarySubListener);
	EDPReaders.second->setListener(nullptr);
	delete(impl->secondaryPubListener);


        if (RMW_RET_OK != rmw_destroy_guard_condition(impl->graph_guard_condition)) {
            RMW_SET_ERROR_MSG("failed to destroy graph guard condition");
            return RMW_RET_ERROR;
        }

	Domain::removeParticipant(participant);

	delete(impl);
        if (node->name) {
            free(const_cast<char *>(node->name));
            node->name = nullptr;
        }

        free(static_cast<void*>(node));

        return RMW_RET_OK;
    }

    typedef struct CustomPublisherInfo
    {
        Publisher *publisher_;
        void *type_support_;
        rmw_gid_t publisher_gid;
        const char * typesupport_identifier_;
    } CustomPublisherInfo;

    rmw_publisher_t* rmw_create_publisher(const rmw_node_t *node, const rosidl_message_type_support_t *type_support,
            const char* topic_name, const rmw_qos_profile_t * qos_policies)
    {
        rmw_publisher_t *rmw_publisher  = nullptr;
        const GUID_t *guid = nullptr;

        assert(node);
        assert(type_support);
        assert(topic_name);
        assert(qos_policies);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return NULL;
        }

        CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
        if (!impl) {
            RMW_SET_ERROR_MSG("node impl is null");
            return NULL;
        }

        Participant *participant = impl->participant;

        if(
            strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_c__identifier) != 0 &&
            strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0
        )
        {
            RMW_SET_ERROR_MSG("type support not from this implementation");
            return NULL;
        }

        CustomPublisherInfo *info = new CustomPublisherInfo();
        info->typesupport_identifier_ = type_support->typesupport_identifier;

        std::string type_name = _create_type_name(type_support->data, "msg",
            info->typesupport_identifier_);
        if(!Domain::getRegisteredType(participant, type_name.c_str(), (TopicDataType**)&info->type_support_))
        {

            info->type_support_ = _create_message_type_support(type_support->data, info->typesupport_identifier_);
            _register_type(participant, info->type_support_, info->typesupport_identifier_);
        }

        PublisherAttributes publisherParam;
        publisherParam.topic.topicKind = NO_KEY;
        publisherParam.topic.topicDataType = type_name;
        publisherParam.topic.topicName = topic_name;
        publisherParam.qos.m_publishMode.kind = ASYNCHRONOUS_PUBLISH_MODE;
        publisherParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

        // 1 Heartbeat every 10ms
        //publisherParam.times.heartbeatPeriod.seconds = 0;
        //publisherParam.times.heartbeatPeriod.fraction = 42949673;

        // 300000 bytes each 10ms
        //ThroughputControllerDescriptor throughputController{3000000, 10};
        //publisherParam.throughputController = throughputController;

        if(!get_datawriter_qos(*qos_policies, publisherParam))
            goto fail;

        info->publisher_ = Domain::createPublisher(participant, publisherParam, NULL);

        if(!info->publisher_)
        {
            RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
            goto fail;
        }

        info->publisher_gid.implementation_identifier = eprosima_fastrtps_identifier;
        static_assert(
                sizeof(eprosima::fastrtps::rtps::GUID_t) <= RMW_GID_STORAGE_SIZE,
                "RMW_GID_STORAGE_SIZE insufficient to store the rmw_fastrtps_cpp GID implementation."
                );

        memset(info->publisher_gid.data, 0, RMW_GID_STORAGE_SIZE);
        guid = &info->publisher_->getGuid();
        memcpy(info->publisher_gid.data, guid, sizeof(eprosima::fastrtps::rtps::GUID_t));

        rmw_publisher = new rmw_publisher_t;
        rmw_publisher->implementation_identifier = eprosima_fastrtps_identifier;
        rmw_publisher->data = info;
        rmw_publisher->topic_name = reinterpret_cast<const char *>(new char [strlen(topic_name) + 1]);
        memcpy(const_cast<char *>(rmw_publisher->topic_name), topic_name, strlen(topic_name)+1);
        return rmw_publisher;
fail:

        if(info != nullptr)
        {
            _delete_typesupport(info->type_support_, info->typesupport_identifier_);
            delete info;
        }

        return NULL;

    }

    rmw_ret_t rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
    {
        if (!node)
        {
            RMW_SET_ERROR_MSG("node handle is null");
            return RMW_RET_ERROR;
        }

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("publisher handle not from this implementation");
            return RMW_RET_ERROR;
        }

        if (!publisher) {
            RMW_SET_ERROR_MSG("publisher handle is null");
            return RMW_RET_ERROR;
        }

        if(publisher->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("publisher handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomPublisherInfo *info = (CustomPublisherInfo *)publisher->data;
        if(info != nullptr)
        {
            if(info->publisher_ != nullptr)
                Domain::removePublisher(info->publisher_);
            if(info->type_support_ != nullptr)
            {
                CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
                if (!impl) {
                    RMW_SET_ERROR_MSG("node impl is null");
                    return RMW_RET_ERROR;
                }

                Participant *participant = impl->participant;
                _unregister_type(participant, info->type_support_, info->typesupport_identifier_);
            }
            delete info;
        }
        delete(publisher);

        return RMW_RET_OK;
    }


    rmw_ret_t rmw_publish(const rmw_publisher_t *publisher, const void *ros_message)
    {
        assert(publisher);
        assert(ros_message);
        rmw_ret_t returnedValue = RMW_RET_ERROR;

        if(publisher->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("publisher handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomPublisherInfo *info = (CustomPublisherInfo*)publisher->data;
        assert(info);

        eprosima::fastcdr::FastBuffer buffer;
        eprosima::fastcdr::Cdr ser(buffer);

        if(_serialize_ros_message(ros_message, ser, info->type_support_, info->typesupport_identifier_))
        {
            if(info->publisher_->write(&ser))
                returnedValue = RMW_RET_OK;
            else
                RMW_SET_ERROR_MSG("cannot publish data");
        }
        else
            RMW_SET_ERROR_MSG("cannot serialize data");

        return returnedValue;
    }

    class SubListener;

    typedef struct CustomSubscriberInfo
    {
        Subscriber *subscriber_;
        SubListener *listener_;
        void *type_support_;
        const char *typesupport_identifier_;
    } CustomSubscriberInfo;

    class SubListener : public SubscriberListener
    {
        public:

            SubListener(CustomSubscriberInfo *info) : data_(0),
            conditionMutex_(NULL), conditionVariable_(NULL) {
              // Field is not used right now
              (void)info;
            }

            void onSubscriptionMatched(Subscriber *sub, MatchingInfo &info) {
              (void)sub;
              (void)info;
            }

            void onNewDataMessage(Subscriber *sub)
            {
                (void)sub;
                std::lock_guard<std::mutex> lock(internalMutex_);

                if(conditionMutex_ != NULL)
                {
                    std::unique_lock<std::mutex> clock(*conditionMutex_);
                    ++data_;
                    clock.unlock();
                    conditionVariable_->notify_one();
                }
                else
                    ++data_;

            }

            void attachCondition(std::mutex *conditionMutex, std::condition_variable *conditionVariable)
            {
                std::lock_guard<std::mutex> lock(internalMutex_);
                conditionMutex_ = conditionMutex;
                conditionVariable_ = conditionVariable;
            }

            void dettachCondition()
            {
                std::lock_guard<std::mutex> lock(internalMutex_);
                conditionMutex_ = NULL;
                conditionVariable_ = NULL;
            }

            bool hasData()
            {
                return data_ > 0;
            }

            void data_taken()
            {
                std::lock_guard<std::mutex> lock(internalMutex_);

                if(conditionMutex_ != NULL)
                {
                    std::unique_lock<std::mutex> clock(*conditionMutex_);
                    --data_;
                }
                else
                    --data_;

                return;
            }

        private:

            std::mutex internalMutex_;
            uint32_t data_;
            std::mutex *conditionMutex_;
            std::condition_variable *conditionVariable_;
    };

    rmw_subscription_t* rmw_create_subscription(const rmw_node_t *node, const rosidl_message_type_support_t *type_support,
            const char *topic_name, const rmw_qos_profile_t * qos_policies, bool ignore_local_publications)
    {
        (void)ignore_local_publications;
        rmw_subscription_t *subscription = nullptr;

        assert(node);
        assert(type_support);
        assert(topic_name);
        assert(qos_policies);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return NULL;
        }

        CustomParticipantInfo* impl = static_cast<CustomParticipantInfo *>(node->data);
        if (!impl) {
            RMW_SET_ERROR_MSG("node impl is null");
            return NULL;
        }

        Participant *participant = impl->participant;
        if(
            strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_c__identifier) != 0 &&
            strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0
        )
        {
            RMW_SET_ERROR_MSG("type support not from this implementation");
            return NULL;
        }

        CustomSubscriberInfo *info = new CustomSubscriberInfo();
        info->typesupport_identifier_ = type_support->typesupport_identifier;

        std::string type_name = _create_type_name(
            type_support->data, "msg", info->typesupport_identifier_);

        if(!Domain::getRegisteredType(participant, type_name.c_str(), (TopicDataType**)&info->type_support_))
        {

            info->type_support_ = _create_message_type_support(type_support->data, info->typesupport_identifier_);
            _register_type(participant, info->type_support_, info->typesupport_identifier_);
        }

        SubscriberAttributes subscriberParam;
        subscriberParam.topic.topicKind = NO_KEY;
        subscriberParam.topic.topicDataType = type_name;
        subscriberParam.topic.topicName = topic_name;
        subscriberParam.historyMemoryPolicy = PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

        if(!get_datareader_qos(*qos_policies, subscriberParam))
            goto fail;

        info->listener_ = new SubListener(info);
        info->subscriber_ = Domain::createSubscriber(participant, subscriberParam, info->listener_);

        if(!info->subscriber_)
        {
            RMW_SET_ERROR_MSG("create_subscriber() could not create subscriber");
            goto fail;
        }

        subscription = new rmw_subscription_t;
        subscription->implementation_identifier = eprosima_fastrtps_identifier;
        subscription->data = info;
        subscription->topic_name = reinterpret_cast<const char *>(new char [strlen(topic_name) + 1]);
        memcpy(const_cast<char *>(subscription->topic_name), topic_name, strlen(topic_name)+1);

        return subscription;
fail:

        if(info != nullptr)
        {
            if(info->type_support_ != nullptr)
            _delete_typesupport(info->type_support_, info->typesupport_identifier_);
            delete info;
        }

        return NULL;
    }

    rmw_ret_t rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
    {
        if (!node) {
            RMW_SET_ERROR_MSG("node handle is null");
            return RMW_RET_ERROR;
        }

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return RMW_RET_ERROR;
        }

        if (!subscription) {
            RMW_SET_ERROR_MSG("subscription handle is null");
            return RMW_RET_ERROR;
        }

        if(subscription->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomSubscriberInfo *info = static_cast<CustomSubscriberInfo*>(subscription->data);

        if(info != nullptr)
        {
            if(info->subscriber_ != nullptr)
                Domain::removeSubscriber(info->subscriber_);
            if(info->listener_ != nullptr)
                delete info->listener_;
            if(info->type_support_ != nullptr)
            {
                CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
                if (!impl) {
                    RMW_SET_ERROR_MSG("node impl is null");
                    return RMW_RET_ERROR;
                }

                Participant *participant = impl->participant;
                _unregister_type(participant, info->type_support_, info->typesupport_identifier_);
            }
            delete info;
        }

        delete(subscription);

        return RMW_RET_OK;
    }

    rmw_ret_t rmw_take(const rmw_subscription_t *subscription, void *ros_message, bool *taken)
    {
        assert(subscription);
        assert(ros_message);
        assert(taken);

        *taken = false;

        if(subscription->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("publisher handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomSubscriberInfo *info = (CustomSubscriberInfo*)subscription->data;
        assert(info);

        eprosima::fastcdr::FastBuffer buffer;
        SampleInfo_t sinfo;

        if(info->subscriber_->takeNextData(&buffer, &sinfo))
        {
            info->listener_->data_taken();

            if(sinfo.sampleKind == ALIVE)
            {
                _deserialize_ros_message(&buffer, ros_message, info->type_support_, info->typesupport_identifier_);
                *taken = true;
            }
        }

        return RMW_RET_OK;
    }

    rmw_ret_t rmw_take_with_info(
            const rmw_subscription_t * subscription,
            void * ros_message,
            bool * taken,
            rmw_message_info_t * message_info)
    {
        assert(subscription);
        assert(ros_message);
        assert(taken);

        if (!message_info) {
            RMW_SET_ERROR_MSG("message info is null");
            return RMW_RET_ERROR;
        }

        *taken = false;

        if(subscription->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("publisher handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomSubscriberInfo *info = (CustomSubscriberInfo*)subscription->data;
        assert(info);

        eprosima::fastcdr::FastBuffer buffer;
        SampleInfo_t sinfo;

        if(info->subscriber_->takeNextData(&buffer, &sinfo))
        {
            info->listener_->data_taken();

            if(sinfo.sampleKind == ALIVE)
            {
                _deserialize_ros_message(&buffer, ros_message, info->type_support_, info->typesupport_identifier_);
                rmw_gid_t * sender_gid = &message_info->publisher_gid;
                sender_gid->implementation_identifier = eprosima_fastrtps_identifier;
                memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
                memcpy(sender_gid->data, &sinfo.sample_identity.writer_guid(), sizeof(eprosima::fastrtps::rtps::GUID_t));
                *taken = true;
            }
        }

        return RMW_RET_OK;
    }

    class GuardCondition
    {
        public:

            GuardCondition() : hasTriggered_(false),
            conditionMutex_(NULL), conditionVariable_(NULL) {}

            void trigger()
            {
                std::lock_guard<std::mutex> lock(internalMutex_);

                if(conditionMutex_ != NULL)
                {
                    std::unique_lock<std::mutex> clock(*conditionMutex_);
                    hasTriggered_ = true;
                    clock.unlock();
                    conditionVariable_->notify_one();
                }
                else
                    hasTriggered_ = true;

            }

            void attachCondition(std::mutex *conditionMutex, std::condition_variable *conditionVariable)
            {
                std::lock_guard<std::mutex> lock(internalMutex_);
                conditionMutex_ = conditionMutex;
                conditionVariable_ = conditionVariable;
            }

            void dettachCondition()
            {
                std::lock_guard<std::mutex> lock(internalMutex_);
                conditionMutex_ = NULL;
                conditionVariable_ = NULL;
            }

            bool hasTriggered()
            {
                return hasTriggered_;
            }

            bool getHasTriggered()
            {
                bool ret = hasTriggered_;
                hasTriggered_ = false;
                return ret;
            }

        private:

            std::mutex internalMutex_;
            bool hasTriggered_;
            std::mutex *conditionMutex_;
            std::condition_variable *conditionVariable_;
    };

    rmw_guard_condition_t* rmw_create_guard_condition()
    {
        rmw_guard_condition_t *guard_condition_handle = new rmw_guard_condition_t;
        guard_condition_handle->implementation_identifier = eprosima_fastrtps_identifier;
        guard_condition_handle->data = new GuardCondition();
        return guard_condition_handle;
    }


    rmw_ret_t rmw_destroy_guard_condition(rmw_guard_condition_t *guard_condition)
    {
        if(guard_condition)
        {
            delete (GuardCondition*)guard_condition->data;
            delete guard_condition;
            return RMW_RET_OK;
        }

        return RMW_RET_ERROR;
    }

    rmw_ret_t rmw_trigger_guard_condition(const rmw_guard_condition_t *guard_condition_handle)
    {
        assert(guard_condition_handle);

        if(guard_condition_handle->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("guard condition handle not from this implementation");
            return RMW_RET_ERROR;
        }

        GuardCondition *guard_condition = (GuardCondition*)guard_condition_handle->data;
        guard_condition->trigger();
        return RMW_RET_OK;
    }

    rmw_waitset_t *
    rmw_create_waitset(size_t max_conditions)
    {
        (void)max_conditions;
        rmw_waitset_t * waitset = rmw_waitset_allocate();
        CustomWaitsetInfo * waitset_info = nullptr;

        // From here onward, error results in unrolling in the goto fail block.
        if (!waitset) {
            RMW_SET_ERROR_MSG("failed to allocate waitset");
            goto fail;
        }
        waitset->implementation_identifier = eprosima_fastrtps_identifier;
        waitset->data = rmw_allocate(sizeof(CustomWaitsetInfo));
        // This should default-construct the fields of CustomWaitsetInfo
        waitset_info = static_cast<CustomWaitsetInfo *>(waitset->data);
        RMW_TRY_PLACEMENT_NEW(waitset_info, waitset_info, goto fail, CustomWaitsetInfo);
        if (!waitset_info) {
            RMW_SET_ERROR_MSG("failed to construct waitset info struct");
            goto fail;
        }

        return waitset;

fail:
        if (waitset) {
            if (waitset->data) {
                RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
                    waitset_info->~CustomWaitsetInfo(), waitset_info)
                rmw_free(waitset->data);
            }
            rmw_waitset_free(waitset);
        }
        return nullptr;
    }

    rmw_ret_t
    rmw_destroy_waitset(rmw_waitset_t * waitset)
    {
        if (!waitset) {
            RMW_SET_ERROR_MSG("waitset handle is null");
            return RMW_RET_ERROR;
        }
        RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
                waitset handle,
                waitset->implementation_identifier, eprosima_fastrtps_identifier,
                return RMW_RET_ERROR)

        auto result = RMW_RET_OK;
        CustomWaitsetInfo * waitset_info = static_cast<CustomWaitsetInfo *>(waitset->data);
        if (!waitset_info) {
            RMW_SET_ERROR_MSG("waitset info is null");
            return RMW_RET_ERROR;
        }
        std::mutex * conditionMutex = &waitset_info->condition_mutex;
        if (!conditionMutex) {
            RMW_SET_ERROR_MSG("waitset mutex is null");
            return RMW_RET_ERROR;
        }

        if (waitset) {
            if (waitset->data) {
                if (waitset_info) {
                    RMW_TRY_DESTRUCTOR(
                        waitset_info->~CustomWaitsetInfo(), waitset_info, result = RMW_RET_ERROR)
                }
                rmw_free(waitset->data);
            }
            rmw_waitset_free(waitset);
        }
        return result;
    }

    class ServiceListener;

    typedef struct CustomServiceInfo
    {
        void *request_type_support_;
        void *response_type_support_;
        Subscriber *request_subscriber_;
        Publisher *response_publisher_;
        ServiceListener *listener_;
        Participant *participant_;
        const char *typesupport_identifier_;
    } CustomServiceInfo;

    typedef struct CustomServiceRequest
    {
	    eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
	    eprosima::fastcdr::FastBuffer *buffer_;

	    CustomServiceRequest() : buffer_(nullptr) {}
    } CustomServiceRequest;

    class ServiceListener : public SubscriberListener
    {
        public:

            explicit ServiceListener(CustomServiceInfo *info)
            : info_(info), conditionMutex_(NULL), conditionVariable_(NULL)
            {
                (void)info_;
            }


            void onNewDataMessage(Subscriber *sub)
            {
                assert(sub);

                CustomServiceRequest request;
                request.buffer_ = new eprosima::fastcdr::FastBuffer();
                SampleInfo_t sinfo;

                if(sub->takeNextData(request.buffer_, &sinfo))
                {
                    if(sinfo.sampleKind == ALIVE)
                    {
                        request.sample_identity_ = sinfo.sample_identity;

                        std::lock_guard<std::mutex> lock(internalMutex_);

                        if(conditionMutex_ != NULL)
                        {
                            std::unique_lock<std::mutex> clock(*conditionMutex_);
                            list.push_back(request);
                            clock.unlock();
                            conditionVariable_->notify_one();
                        }
                        else
                            list.push_back(request);
                    }
                }
            }

            CustomServiceRequest getRequest()
            {
                std::lock_guard<std::mutex> lock(internalMutex_);
                CustomServiceRequest request;

                if(conditionMutex_ != NULL)
                {
                    std::unique_lock<std::mutex> clock(*conditionMutex_);
                    if(!list.empty())
                    {
                        request = list.front();
                        list.pop_front();
                    }
                }
                else
                {
                    if(!list.empty())
                    {
                        request = list.front();
                        list.pop_front();
                    }
                }

                return request;
            }

            void attachCondition(std::mutex *conditionMutex, std::condition_variable *conditionVariable)
            {
                std::lock_guard<std::mutex> lock(internalMutex_);
                conditionMutex_ = conditionMutex;
                conditionVariable_ = conditionVariable;
            }

            void dettachCondition()
            {
                std::lock_guard<std::mutex> lock(internalMutex_);
                conditionMutex_ = NULL;
                conditionVariable_ = NULL;
            }

            bool hasData()
            {
                return !list.empty();
            }

        private:

            CustomServiceInfo *info_;
            std::mutex internalMutex_;
            std::list<CustomServiceRequest> list;
            std::mutex *conditionMutex_;
            std::condition_variable *conditionVariable_;
    };

    rmw_client_t* rmw_create_client(const rmw_node_t *node,
            const rosidl_service_type_support_t *type_support,
            const char *service_name, const rmw_qos_profile_t * qos_policies)
    {
        CustomClientInfo *info = nullptr;
        rmw_client_t *client = nullptr;

        assert(node);
        assert(type_support);
        assert(service_name);
        assert(qos_policies);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return NULL;
        }

        CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
        if (!impl) {
            RMW_SET_ERROR_MSG("node impl is null");
            return NULL;
        }

        Participant *participant = impl->participant;

        if(
            strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_c__identifier) != 0 &&
            strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0
        )
        {
            RMW_SET_ERROR_MSG("type support not from this implementation");
            return NULL;
        }

        info = new CustomClientInfo();
        info->participant_ = participant;
        info->typesupport_identifier_ = type_support->typesupport_identifier;

        const void * untyped_request_members;
        const void * untyped_response_members;

        untyped_request_members =
            get_request_ptr(type_support->data, info->typesupport_identifier_);
        untyped_response_members = get_response_ptr(type_support->data,
            info->typesupport_identifier_);

        std::string request_type_name = _create_type_name(untyped_request_members, "srv",
            info->typesupport_identifier_);
        std::string response_type_name = _create_type_name(untyped_response_members, "srv",
            info->typesupport_identifier_);

        if(!Domain::getRegisteredType(participant, request_type_name.c_str(), (TopicDataType**)&info->request_type_support_))
        {

            info->request_type_support_ = _create_request_type_support(type_support->data, info->typesupport_identifier_);
            _register_type(participant, info->request_type_support_, info->typesupport_identifier_);
        }

        if(!Domain::getRegisteredType(participant, response_type_name.c_str(), (TopicDataType**)&info->response_type_support_))
        {
            info->response_type_support_ = _create_response_type_support(type_support->data, info->typesupport_identifier_);
            _register_type(participant, info->response_type_support_, info->typesupport_identifier_);
        }

        SubscriberAttributes subscriberParam;
        PublisherAttributes publisherParam;

        subscriberParam.topic.topicKind = NO_KEY;
        subscriberParam.topic.topicDataType = response_type_name;
        subscriberParam.topic.topicName = std::string(service_name) + "Reply";

        if(!get_datareader_qos(*qos_policies, subscriberParam))
            goto fail;

        info->listener_ = new ClientListener(info);
        info->response_subscriber_ = Domain::createSubscriber(participant, subscriberParam, info->listener_);

        if(!info->response_subscriber_)
        {
            RMW_SET_ERROR_MSG("create_client() could not create subscriber");
            goto fail;
        }

        publisherParam.topic.topicKind = NO_KEY;
        publisherParam.topic.topicDataType = request_type_name;
        publisherParam.topic.topicName = std::string(service_name) + "Request";

        if(!get_datawriter_qos(*qos_policies, publisherParam))
            goto fail;

        info->request_publisher_ = Domain::createPublisher(participant, publisherParam, NULL);

        if(!info->request_publisher_)
        {
            RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
            goto fail;
        }

        info->writer_guid_ = info->request_publisher_->getGuid();

        client = new rmw_client_t;
        client->implementation_identifier = eprosima_fastrtps_identifier;
        client->data = info;
        client->service_name = reinterpret_cast<const char*>(rmw_allocate(strlen(service_name) + 1));
        if (!client->service_name) {
          RMW_SET_ERROR_MSG("failed to allocate memory for node name");
          goto fail;
        }
        memcpy(const_cast<char *>(client->service_name), service_name, strlen(service_name) + 1);

        return client;

fail:

        if(info != nullptr)
        {
            if(info->request_publisher_ != nullptr)
            {
                Domain::removePublisher(info->request_publisher_);
            }

            if(info->response_subscriber_ != nullptr)
            {
                Domain::removeSubscriber(info->response_subscriber_);
            }

            if(info->listener_ != nullptr)
            {
                delete info->listener_;
            }

            CustomParticipantInfo* impl = static_cast<CustomParticipantInfo *>(node->data);
            if (impl) {
                Participant *participant = impl->participant;
                if(info->request_type_support_ != nullptr)
                {
                    _unregister_type(participant, info->request_type_support_, info->typesupport_identifier_);
                }

                if(info->response_type_support_ != nullptr)
                {
                    _unregister_type(participant, info->response_type_support_, info->typesupport_identifier_);
                }
            } else {
                fprintf(stderr,
                    "[rmw_fastrtps] leaking type support objects because node impl is null\n");
            }

            delete info;
        }

        return NULL;
    }

    rmw_ret_t rmw_send_request(const rmw_client_t *client,
            const void *ros_request,
            int64_t *sequence_id)
    {
        assert(client);
        assert(ros_request);
        assert(sequence_id);

        rmw_ret_t returnedValue = RMW_RET_ERROR;

        if(client->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomClientInfo *info = (CustomClientInfo*)client->data;
        assert(info);

        eprosima::fastcdr::FastBuffer buffer;
        eprosima::fastcdr::Cdr ser(buffer);

        if(_serialize_ros_message(ros_request, ser, info->request_type_support_, info->typesupport_identifier_))
        {
            eprosima::fastrtps::rtps::WriteParams wparams;

            if(info->request_publisher_->write(&ser, wparams))
            {
                returnedValue = RMW_RET_OK;
                *sequence_id = ((int64_t)wparams.sample_identity().sequence_number().high) << 32 | wparams.sample_identity().sequence_number().low;
            }
            else
                RMW_SET_ERROR_MSG("cannot publish data");
        }
        else
            RMW_SET_ERROR_MSG("cannot serialize data");

        return returnedValue;
    }

    rmw_ret_t rmw_take_request(const rmw_service_t *service,
            rmw_request_id_t *request_header,
            void *ros_request,
            bool *taken)
    {
        assert(service);
        assert(request_header);
        assert(ros_request);
        assert(taken);

        *taken = false;

        if(service->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("service handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomServiceInfo *info = (CustomServiceInfo*)service->data;
        assert(info);

        CustomServiceRequest request = info->listener_->getRequest();

        if(request.buffer_ != nullptr)
        {
            _deserialize_ros_message(request.buffer_, ros_request, info->request_type_support_, info->typesupport_identifier_);

            // Get header
            memcpy(request_header->writer_guid, &request.sample_identity_.writer_guid(), sizeof(eprosima::fastrtps::rtps::GUID_t));
            request_header->sequence_number = ((int64_t)request.sample_identity_.sequence_number().high) << 32 | request.sample_identity_.sequence_number().low;

            delete request.buffer_;

            *taken = true;
        }

        return RMW_RET_OK;
    }

    rmw_ret_t rmw_take_response(const rmw_client_t *client,
            rmw_request_id_t *request_header,
            void *ros_response,
            bool *taken)
    {
        assert(client);
        assert(request_header);
        assert(ros_response);
        assert(taken);

        *taken = false;

        if(client->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("service handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomClientInfo *info = (CustomClientInfo*)client->data;
        assert(info);

        CustomClientResponse response = info->listener_->getResponse();

        if(response.buffer_ != nullptr)
        {
            _deserialize_ros_message(response.buffer_, ros_response, info->response_type_support_, info->typesupport_identifier_);

            request_header->sequence_number = ((int64_t)response.sample_identity_.sequence_number().high) << 32 | response.sample_identity_.sequence_number().low;

            delete response.buffer_;

            *taken = true;
        }

        return RMW_RET_OK;
    }

    rmw_ret_t rmw_send_response(const rmw_service_t *service,
            rmw_request_id_t *request_header,
            void *ros_response)
    {
        assert(service);
        assert(request_header);
        assert(ros_response);

        rmw_ret_t returnedValue = RMW_RET_ERROR;

        if(service->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("service handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomServiceInfo *info = (CustomServiceInfo*)service->data;
        assert(info);

        eprosima::fastcdr::FastBuffer buffer;
        eprosima::fastcdr::Cdr ser(buffer);

        _serialize_ros_message(ros_response, ser, info->response_type_support_, info->typesupport_identifier_);
        eprosima::fastrtps::rtps::WriteParams wparams;
        memcpy(&wparams.related_sample_identity().writer_guid(), request_header->writer_guid, sizeof(eprosima::fastrtps::rtps::GUID_t));
        wparams.related_sample_identity().sequence_number().high = (int32_t)((request_header->sequence_number & 0xFFFFFFFF00000000) >> 32);
        wparams.related_sample_identity().sequence_number().low = (int32_t)(request_header->sequence_number & 0xFFFFFFFF);

        if(info->response_publisher_->write(&ser, wparams))
        {
            returnedValue = RMW_RET_OK;
        }
        else
            RMW_SET_ERROR_MSG("cannot publish data");

        return returnedValue;
    }

    rmw_service_t *rmw_create_service(const rmw_node_t *node,
            const rosidl_service_type_support_t *type_support,
            const char *service_name, const rmw_qos_profile_t * qos_policies)
    {
        CustomServiceInfo *info = nullptr;
        rmw_service_t *service  = nullptr;

        assert(node);
        assert(type_support);
        assert(service_name);
        assert(qos_policies);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return NULL;
        }

        CustomParticipantInfo * impl = static_cast<CustomParticipantInfo *>(node->data);
        if (!impl) {
            RMW_SET_ERROR_MSG("node impl is null");
            return NULL;
        }

        Participant *participant = impl->participant;
        if(
            strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_c__identifier) != 0 &&
            strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0
        )
        {
            RMW_SET_ERROR_MSG("type support not from this implementation");
            return NULL;
        }

        info = new CustomServiceInfo();
        info->participant_ = participant;
        info->typesupport_identifier_ = type_support->typesupport_identifier;

        const void * untyped_request_members;
        const void * untyped_response_members;

        untyped_request_members =
            get_request_ptr(type_support->data, info->typesupport_identifier_);
        untyped_response_members = get_response_ptr(type_support->data,
            info->typesupport_identifier_);

        std::string request_type_name = _create_type_name(untyped_request_members, "srv",
            info->typesupport_identifier_);
        std::string response_type_name = _create_type_name(untyped_response_members, "srv",
            info->typesupport_identifier_);

        if(!Domain::getRegisteredType(participant, request_type_name.c_str(), (TopicDataType**)&info->request_type_support_))
        {
            info->request_type_support_ = _create_request_type_support(type_support->data, info->typesupport_identifier_);
            _register_type(participant, info->request_type_support_, info->typesupport_identifier_);
        }

        if(!Domain::getRegisteredType(participant, response_type_name.c_str(), (TopicDataType**)&info->response_type_support_))
        {
            info->response_type_support_ = _create_response_type_support(type_support->data, info->typesupport_identifier_);
            _register_type(participant, info->response_type_support_, info->typesupport_identifier_);
        }

        SubscriberAttributes subscriberParam;
        PublisherAttributes publisherParam;

        subscriberParam.topic.topicKind = NO_KEY;
        subscriberParam.topic.topicDataType = request_type_name;
        subscriberParam.topic.topicName = std::string(service_name) + "Request";

        if(!get_datareader_qos(*qos_policies, subscriberParam))
            goto fail;

        info->listener_ = new ServiceListener(info);
        info->request_subscriber_ = Domain::createSubscriber(participant, subscriberParam, info->listener_);

        if(!info->request_subscriber_)
        {
            RMW_SET_ERROR_MSG("create_client() could not create subscriber");
            goto fail;
        }

        publisherParam.topic.topicKind = NO_KEY;
        publisherParam.topic.topicDataType = response_type_name;
        publisherParam.topic.topicName = std::string(service_name) + "Reply";

        if(!get_datawriter_qos(*qos_policies, publisherParam))
            goto fail;

        info->response_publisher_ = Domain::createPublisher(participant, publisherParam, NULL);

        if(!info->response_publisher_)
        {
            RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
            goto fail;
        }

        service = new rmw_service_t;
        service->implementation_identifier = eprosima_fastrtps_identifier;
        service->data = info;
        service->service_name = reinterpret_cast<const char *>(
          rmw_allocate(strlen(service_name) +1));
        if (!service->service_name) {
          RMW_SET_ERROR_MSG("failed to allocate memory for node name");
          goto fail;
        }
        memcpy(const_cast<char *>(service->service_name), service_name, strlen(service_name) + 1);

        return service;

fail:

        if(info != nullptr)
        {
            if(info->response_publisher_ != nullptr)
            {
                Domain::removePublisher(info->response_publisher_);
            }

            if(info->request_subscriber_ != nullptr)
            {
                Domain::removeSubscriber(info->request_subscriber_);
            }

            if(info->listener_ != nullptr)
            {
                delete info->listener_;
            }

            if(info->request_type_support_ != nullptr)
            {
                _unregister_type(participant, info->request_type_support_, info->typesupport_identifier_);
            }

            if(info->response_type_support_ != nullptr)
            {
                _unregister_type(participant, info->response_type_support_, info->typesupport_identifier_);
            }

            delete info;
        }

        return NULL;
    }

    rmw_ret_t rmw_destroy_service(rmw_service_t *service)
    {
        if (!service) {
            RMW_SET_ERROR_MSG("service handle is null");
            return RMW_RET_ERROR;
        }
        if(service->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("publisher handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomServiceInfo *info = (CustomServiceInfo *)service->data;
        if(info != nullptr)
        {
            if(info->request_subscriber_ != nullptr)
            {
                Domain::removeSubscriber(info->request_subscriber_);
            }
            if(info->response_publisher_ != nullptr)
            {
                Domain::removePublisher(info->response_publisher_);
            }
            if(info->listener_ != nullptr)
                delete info->listener_;

            if(info->request_type_support_ != nullptr)
            {
                _unregister_type(info->participant_, info->request_type_support_, info->typesupport_identifier_);
            }
            if(info->response_type_support_ != nullptr)
            {
                _unregister_type(info->participant_, info->response_type_support_, info->typesupport_identifier_);
            }
            delete info;
        }
        delete(service);

        return RMW_RET_OK;
    }

    rmw_ret_t rmw_destroy_client(rmw_client_t *client)
    {
        if (!client) {
            RMW_SET_ERROR_MSG("client handle is null");
            return RMW_RET_ERROR;
        }
        if(client->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("publisher handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomClientInfo *info = (CustomClientInfo*)client->data;
        if(info != nullptr)
        {
            if(info->response_subscriber_ != nullptr)
                Domain::removeSubscriber(info->response_subscriber_);
            if(info->request_publisher_ != nullptr)
                Domain::removePublisher(info->request_publisher_);
            if(info->listener_ != nullptr)
                delete info->listener_;
            if(info->request_type_support_ != nullptr)
            {
                _unregister_type(info->participant_, info->request_type_support_, info->typesupport_identifier_);
            }
            if(info->response_type_support_ != nullptr)
            {
                _unregister_type(info->participant_, info->response_type_support_, info->typesupport_identifier_);
            }
            delete info;
        }
        delete(client);

        return RMW_RET_OK;
    }

    // helper function for wait
    bool check_waitset_for_data(const rmw_subscriptions_t *subscriptions,
            const rmw_guard_conditions_t *guard_conditions,
            const rmw_services_t *services,
            const rmw_clients_t *clients)
    {
        for(unsigned long i = 0; i < subscriptions->subscriber_count; ++i)
        {
            void *data = subscriptions->subscribers[i];
            CustomSubscriberInfo *custom_subscriber_info = (CustomSubscriberInfo*)data;
            // Short circuiting out of this function is possible
            if (custom_subscriber_info && custom_subscriber_info->listener_->hasData()) {
              return true;
            }
        }

        for(unsigned long i = 0; i < clients->client_count; ++i)
        {
            void *data = clients->clients[i];
            CustomClientInfo *custom_client_info = (CustomClientInfo*)data;
            if (custom_client_info && custom_client_info->listener_->hasData()) {
              return true;
            }
        }

        for(unsigned long i = 0; i < services->service_count; ++i)
        {
            void *data = services->services[i];
            CustomServiceInfo *custom_service_info = (CustomServiceInfo*)data;
            if (custom_service_info && custom_service_info->listener_->hasData()) {
              return true;
            }
        }

        if (guard_conditions)
        {
            for(unsigned long i = 0; i < guard_conditions->guard_condition_count; ++i)
            {
                void *data = guard_conditions->guard_conditions[i];
                GuardCondition *guard_condition = (GuardCondition*)data;
                if (guard_condition && guard_condition->hasTriggered()) {
                  return true;
                }
            }
        }
        return false;
    }

    rmw_ret_t rmw_wait(rmw_subscriptions_t *subscriptions,
            rmw_guard_conditions_t *guard_conditions,
            rmw_services_t *services,
            rmw_clients_t *clients,
            rmw_waitset_t * waitset,
            const rmw_time_t *wait_timeout)
    {
        if (!waitset) {
            RMW_SET_ERROR_MSG("Waitset handle is null");
            return RMW_RET_ERROR;
        }
        CustomWaitsetInfo * waitset_info = static_cast<CustomWaitsetInfo*>(waitset->data);
        if (!waitset_info) {
            RMW_SET_ERROR_MSG("Waitset info struct is null");
            return RMW_RET_ERROR;
        }
        std::mutex * conditionMutex = &waitset_info->condition_mutex;
        std::condition_variable * conditionVariable = &waitset_info->condition;
        if (!conditionMutex) {
            RMW_SET_ERROR_MSG("Mutex for waitset was null");
            return RMW_RET_ERROR;
        }
        if (!conditionVariable) {
            RMW_SET_ERROR_MSG("Condition variable for waitset was null");
            return RMW_RET_ERROR;
        }

        for(unsigned long i = 0; i < subscriptions->subscriber_count; ++i)
        {
            void *data = subscriptions->subscribers[i];
            CustomSubscriberInfo *custom_subscriber_info = (CustomSubscriberInfo*)data;
            custom_subscriber_info->listener_->attachCondition(conditionMutex, conditionVariable);
        }

        for(unsigned long i = 0; i < clients->client_count; ++i)
        {
            void *data = clients->clients[i];
            CustomClientInfo *custom_client_info = (CustomClientInfo*)data;
            custom_client_info->listener_->attachCondition(conditionMutex, conditionVariable);
        }

        for(unsigned long i = 0; i < services->service_count; ++i)
        {
            void *data = services->services[i];
            CustomServiceInfo *custom_service_info = (CustomServiceInfo*)data;
            custom_service_info->listener_->attachCondition(conditionMutex, conditionVariable);
        }

        if (guard_conditions) {
            for(unsigned long i = 0; i < guard_conditions->guard_condition_count; ++i)
            {
                void *data = guard_conditions->guard_conditions[i];
                GuardCondition *guard_condition = (GuardCondition*)data;
                guard_condition->attachCondition(conditionMutex, conditionVariable);
            }
        }

        std::unique_lock<std::mutex> lock(*conditionMutex);

        // First check variables.
        // If wait_timeout is null, wait indefinitely (so we have to wait)
        // If wait_timeout is not null and either of its fields are nonzero, we have to wait
        bool hasToWait = (wait_timeout && (wait_timeout->sec > 0 || wait_timeout->nsec > 0)) ||
            !wait_timeout;

        for(unsigned long i = 0; hasToWait && i < subscriptions->subscriber_count; ++i)
        {
            void *data = subscriptions->subscribers[i];
            CustomSubscriberInfo *custom_subscriber_info = (CustomSubscriberInfo*)data;
            if(custom_subscriber_info->listener_->hasData())
                hasToWait = false;
        }

        for(unsigned long i = 0; hasToWait && i < clients->client_count; ++i)
        {
            void *data = clients->clients[i];
            CustomClientInfo *custom_client_info = (CustomClientInfo*)data;
            if(custom_client_info->listener_->hasData())
                hasToWait = false;
        }

        for(unsigned long i = 0; hasToWait && i < services->service_count; ++i)
        {
            void *data = services->services[i];
            CustomServiceInfo *custom_service_info = (CustomServiceInfo*)data;
            if(custom_service_info->listener_->hasData())
                hasToWait = false;
        }

        if (guard_conditions) {
            for (unsigned long i = 0; hasToWait && i < guard_conditions->guard_condition_count; ++i)
            {
                void *data = guard_conditions->guard_conditions[i];
                GuardCondition *guard_condition = (GuardCondition*)data;
                if(guard_condition->hasTriggered())
                    hasToWait = false;
            }
        }

        bool timeout = false;

        if(hasToWait)
        {
            if(!wait_timeout)
                conditionVariable->wait(lock);
            else
            {
                auto predicate = [subscriptions, guard_conditions, services, clients]() {
                  return check_waitset_for_data(subscriptions, guard_conditions, services, clients);
                };
                auto n = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::seconds(wait_timeout->sec));
                n += std::chrono::nanoseconds(wait_timeout->nsec);
                timeout = !conditionVariable->wait_for(lock, n, predicate);
            }
        }

        for(unsigned long i = 0; i < subscriptions->subscriber_count; ++i)
        {
            void *data = subscriptions->subscribers[i];
            CustomSubscriberInfo *custom_subscriber_info = (CustomSubscriberInfo*)data;
            if(!custom_subscriber_info->listener_->hasData())
            {
                subscriptions->subscribers[i] = 0;
            }
            lock.unlock();
            custom_subscriber_info->listener_->dettachCondition();
            lock.lock();
        }

        for(unsigned long i = 0; i < clients->client_count; ++i)
        {
            void *data = clients->clients[i];
            CustomClientInfo *custom_client_info = (CustomClientInfo*)data;
            if(!custom_client_info->listener_->hasData())
            {
                clients->clients[i] = 0;
            }
            lock.unlock();
            custom_client_info->listener_->dettachCondition();
            lock.lock();
        }

        for(unsigned long i = 0; i < services->service_count; ++i)
        {
            void *data = services->services[i];
            CustomServiceInfo *custom_service_info = (CustomServiceInfo*)data;
            if(!custom_service_info->listener_->hasData())
            {
                services->services[i] = 0;
            }
            lock.unlock();
            custom_service_info->listener_->dettachCondition();
            lock.lock();
        }

        if (guard_conditions)
        {
            for(unsigned long i = 0; i < guard_conditions->guard_condition_count; ++i)
            {
                void *data = guard_conditions->guard_conditions[i];
                GuardCondition *guard_condition = (GuardCondition*)data;
                if (!guard_condition->getHasTriggered()) {
                    guard_conditions->guard_conditions[i] = 0;
                }
                lock.unlock();
                guard_condition->dettachCondition();
                lock.lock();
            }
        }
        // Make timeout behavior consistent with rcl expectations for zero timeout value
        bool hasData = check_waitset_for_data(subscriptions, guard_conditions, services, clients);
        if (!hasData && wait_timeout && wait_timeout->sec == 0 && wait_timeout->nsec == 0) {
         return RMW_RET_TIMEOUT;
        }

        return timeout ? RMW_RET_TIMEOUT : RMW_RET_OK;
    }


    rmw_ret_t
    rmw_get_topic_names_and_types(
      const rmw_node_t * node,
      rmw_topic_names_and_types_t * topic_names_and_types)
    {
	
	if(!node){
		RMW_SET_ERROR_MSG("null node handle");
		return RMW_RET_ERROR;
	}	
	if(!topic_names_and_types){
		RMW_SET_ERROR_MSG("null topics_names_and_types");
		return RMW_RET_ERROR;
	}
	//Get participant pointer from node
        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return RMW_RET_ERROR;
        }

	CustomParticipantInfo* impl = static_cast<CustomParticipantInfo*>(node->data);
        Participant *participant = impl->participant;

        //if(strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0)
        //{
        //    RMW_SET_ERROR_MSG("type support not from this implementation");
        //    return NULL;
        //}

	//Get and combine info from both Pub and Sub
	std::pair<StatefulReader*,StatefulReader*> EDPReaders = participant->getEDPReaders();
	//Access the slave Listeners, which are the ones that have the topicnamesandtypes member
  	//Get info from publisher and subscriber
	std::map<std::string,std::set<std::string>> unfiltered_topics; //Combined results from the two lists
	topicnamesandtypesReaderListener* slave_target = impl->secondarySubListener; 
	slave_target->mapmutex.lock();
	for(auto it : slave_target->topicNtypes){
		for(auto & itt: it.second){
			unfiltered_topics[it.first].insert(itt);
		}
	}
	slave_target->mapmutex.unlock();
 	slave_target = impl->secondaryPubListener; 
	slave_target->mapmutex.lock();
	for(auto it : slave_target->topicNtypes){
		for(auto & itt: it.second){
			unfiltered_topics[it.first].insert(itt);
		}
	}
	slave_target->mapmutex.unlock();
	//Filter duplicates
	std::map<std::string,std::string> topics;
	for(auto & it : unfiltered_topics){
		if(it.second.size() == 1)	topics[it.first] = *it.second.begin();
	}	
	std::string substring = "::msg::dds_::";
	for (auto & it : topics) {
	       size_t substring_position = it.second.find(substring);
	       if(it.second[it.second.size()-1] == '_' && substring_position != std::string::npos){
		       it.second = it.second.substr(0,substring_position) + "/" + it.second.substr(substring_position + substring.size(), it.second.size() - substring_position -substring.size() -1);
		}
	}


	//Copy data to results handle
	if(topics.size() > 0){
		//Alloc memory for pointers to instances
		topic_names_and_types->topic_names = static_cast<char **>(rmw_allocate(sizeof(char*) * topics.size()));
		if(!topic_names_and_types->topic_names){
			RMW_SET_ERROR_MSG("Failed to allocate memory");
			return RMW_RET_ERROR;
		}
		topic_names_and_types->type_names = static_cast<char **>(rmw_allocate(sizeof(char *) * topics.size()));
		if(!topic_names_and_types->type_names){
			rmw_free(topic_names_and_types->topic_names);
			RMW_SET_ERROR_MSG("Failed to allocate memory");
			return RMW_RET_ERROR;
		}
		//Iterate topics for instances
		int index;
		topic_names_and_types->topic_count = 0;
		for(auto it : topics){
			index = topic_names_and_types->topic_count;
			//Alloc
			char *topic_name = strdup(it.first.c_str());
			if(!topic_name){
				RMW_SET_ERROR_MSG("Failed to allocate memory");
				return RMW_RET_ERROR;
			}
			char *topic_type = strdup(it.second.c_str());
			if(!topic_type){
				rmw_free(topic_name);
				RMW_SET_ERROR_MSG("Failed to allocate memory");
				return RMW_RET_ERROR;
			}
			//Insert
			topic_names_and_types->topic_names[index] = topic_name;
			topic_names_and_types->type_names[index] = topic_type;
			++topic_names_and_types->topic_count;
		}

	}
        return RMW_RET_OK;

    }

    rmw_ret_t
    rmw_destroy_topic_names_and_types(
      rmw_topic_names_and_types_t * topic_names_and_types)
    {
        int cap = topic_names_and_types->topic_count;
	for(int i=0;i < cap; i++){
		rmw_free(topic_names_and_types->topic_names[i]);
		rmw_free(topic_names_and_types->type_names[i]);
	}	
	rmw_free(topic_names_and_types->topic_names);
	rmw_free(topic_names_and_types->type_names);
        return RMW_RET_OK; 
    }

    rmw_ret_t
    rmw_count_publishers(
      const rmw_node_t * node,
      const char * topic_name,
      size_t * count)
    {
        char *target_topic = const_cast<char *>(topic_name);
	//safechecks
	
	if(!node){
		RMW_SET_ERROR_MSG("null node handle");
		return RMW_RET_ERROR;
	}	
	//Get participant pointer from node
        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomParticipantInfo* impl = static_cast<CustomParticipantInfo*>(node->data);
	Participant *participant = impl->participant;

	std::map<std::string,std::set<std::string>>unfiltered_topics;
	topicnamesandtypesReaderListener* slave_target = impl->secondaryPubListener;
	slave_target->mapmutex.lock();
	for(auto it : slave_target->topicNtypes){
		for(auto & itt: it.second){
			unfiltered_topics[it.first].insert(itt);
		}
	}
	slave_target->mapmutex.unlock();

	//get count
	auto it = unfiltered_topics.find(topic_name);
	if(it == unfiltered_topics.end()){
		*count = 0;
	}else{
		*count = it->second.size();
	}
	return RMW_RET_OK;

    }

    rmw_ret_t
    rmw_count_subscribers(
      const rmw_node_t * node,
      const char * topic_name,
      size_t * count)
{
	char *target_topic = const_cast<char *>(topic_name);
	//safechecks
	
	if(!node){
		RMW_SET_ERROR_MSG("null node handle");
		return RMW_RET_ERROR;
	}	
	//Get participant pointer from node
        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomParticipantInfo* impl = static_cast<CustomParticipantInfo*>(node->data);
	Participant *participant = impl->participant;

	std::map<std::string,std::set<std::string>>unfiltered_topics;
	topicnamesandtypesReaderListener* slave_target = impl->secondarySubListener;
	slave_target->mapmutex.lock();
	for(auto it : slave_target->topicNtypes){
		for(auto & itt: it.second){
			unfiltered_topics[it.first].insert(itt);
		}
	}
	slave_target->mapmutex.unlock();

	//get_count
	auto it = unfiltered_topics.find(topic_name);
	if(it == unfiltered_topics.end()){
		*count = 0;
	}else{
		*count = it->second.size();
	}

	return RMW_RET_OK;
}

rmw_ret_t
rmw_service_server_is_available(
	const rmw_node_t *node,
	const rmw_client_t *client,
	bool *is_available)
{
	(void)node;
	(void)client;
	(void)is_available;
	RMW_SET_ERROR_MSG("not implemented");
	return RMW_RET_ERROR;
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t* node)
{
	//TODO(wjwwood): actually use the graph guard condition and notify it when changes happen.
	CustomParticipantInfo* impl = static_cast<CustomParticipantInfo*>(node->data);
	if(!impl){
		RMW_SET_ERROR_MSG("node impl is null");
		return NULL;
	}
	return impl->graph_guard_condition;
}


rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  if (!publisher) {
    RMW_SET_ERROR_MSG("publisher is null");
    return RMW_RET_ERROR;
  }

  if(publisher->implementation_identifier != eprosima_fastrtps_identifier)
  {
	  RMW_SET_ERROR_MSG("publisher handle not from this implementation");
	  return RMW_RET_ERROR;
  }

  if (!gid)
  {
    RMW_SET_ERROR_MSG("gid is null");
    return RMW_RET_ERROR;
  }

  const CustomPublisherInfo *info = static_cast<const CustomPublisherInfo *>(publisher->data);

  if (!info)
  {
    RMW_SET_ERROR_MSG("publisher info handle is null");
    return RMW_RET_ERROR;
  }

  *gid = info->publisher_gid;
  return RMW_RET_OK;
}

rmw_ret_t
rmw_compare_gids_equal(const rmw_gid_t * gid1, const rmw_gid_t * gid2, bool * result)
{
  if (!gid1)
  {
    RMW_SET_ERROR_MSG("gid1 is null");
    return RMW_RET_ERROR;
  }

  if(gid1->implementation_identifier != eprosima_fastrtps_identifier)
  {
	  RMW_SET_ERROR_MSG("guid1 handle not from this implementation");
	  return RMW_RET_ERROR;
  }

  if (!gid2)
  {
    RMW_SET_ERROR_MSG("gid2 is null");
    return RMW_RET_ERROR;
  }

  if(gid2->implementation_identifier != eprosima_fastrtps_identifier)
  {
	  RMW_SET_ERROR_MSG("gid1 handle not from this implementation");
	  return RMW_RET_ERROR;
  }

  if (!result)
  {
    RMW_SET_ERROR_MSG("result is null");
    return RMW_RET_ERROR;
  }

  *result =
    memcmp(gid1->data, gid2->data, sizeof(eprosima::fastrtps::rtps::GUID_t)) == 0;

  return RMW_RET_OK;
}

}
