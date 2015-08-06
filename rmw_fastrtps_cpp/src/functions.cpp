#include <rmw/rmw.h>
#include <rmw/error_handling.h>
#include <rosidl_typesupport_introspection_cpp/identifier.hpp>
#include <rmw_fastrtps_cpp/MessageTypeSupport.h>
#include <rmw_fastrtps_cpp/ServiceTypeSupport.h>

#include <fastrtps/Domain.h>
#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/subscriber/SubscriberListener.h>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/attributes/SubscriberAttributes.h>

#include <rpcdds/transports/dds/RTPSProxyTransport.h>
#include <rpcdds/transports/dds/RTPSServerTransport.h>
#include <rpcdds/transports/dds/components/RTPSProxyProcedureEndpoint.h>
#include <rpcdds/transports/dds/components/RTPSServerProcedureEndpoint.h>
#include <rpcdds/strategies/SingleThreadStrategy.h>
#include <rpcdds/transports/dds/RTPSAsyncTask.h>

#include <cassert>
#include <mutex>
#include <condition_variable>
#include <list>

using namespace eprosima::fastrtps;

class ClientListener;

typedef struct CustomClientInfo
{
    eprosima::rpc::transport::dds::RTPSProxyTransport *transport_;
    rmw_fastrtps_cpp::RequestTypeSupport *request_type_support_;
    rmw_fastrtps_cpp::ResponseTypeSupport *response_type_support_;
    eprosima::rpc::transport::dds::RTPSProxyProcedureEndpoint *topic_endpoint_;
    ClientListener *listener_;
} CustomClientInfo;

class ClientListener
{
    public:

        ClientListener(CustomClientInfo *info) : info_(info),
        conditionMutex_(NULL), conditionVariable_(NULL) {}


        void onNewResponse(rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer *buffer)
        {
            assert(buffer);
            std::lock_guard<std::mutex> lock(internalMutex_);

            if(conditionMutex_ != NULL)
            {
                std::unique_lock<std::mutex> clock(*conditionMutex_);
                list.push_back(buffer);
                clock.unlock();
                conditionVariable_->notify_one();
            }
            else
                list.push_back(buffer);

        }

        rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer* getResponse()
        {
            std::lock_guard<std::mutex> lock(internalMutex_);
            rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer *buffer = nullptr;

            if(conditionMutex_ != NULL)
            {
                std::unique_lock<std::mutex> clock(*conditionMutex_);
                buffer = list.front();
                list.pop_front();
            }
            else
            {
                buffer = list.front();
                list.pop_front();
            }

            return buffer;
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
        std::list<rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer*> list;
        std::mutex *conditionMutex_;
        std::condition_variable *conditionVariable_;
};


class AsyncTask : public eprosima::rpc::transport::dds::RTPSAsyncTask
{
    public:

        AsyncTask(CustomClientInfo *info) :
            info_(info),
            buffer_(static_cast<rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer*>(info->response_type_support_->createData()))
    {
    }

        virtual ~AsyncTask(){}

        virtual void execute()
        {
            info_->listener_->onNewResponse(buffer_);
        }

        virtual void on_exception(const eprosima::rpc::exception::SystemException &ex){}

        virtual void* getReplyInstance() { return buffer_; }

    private:

        CustomClientInfo *info_;

        rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer *buffer_;
};

extern "C"
{
    const char* const eprosima_fastrtps_identifier = "fastrtps";

    const char* rmw_get_implementation_identifier()
    {
        return eprosima_fastrtps_identifier;
    }

    rmw_ret_t rmw_init()
    {
        return RMW_RET_OK;
    }

    rmw_node_t* rmw_create_node(const char *name, size_t domain_id)
    {
        assert(name);

        ParticipantAttributes participantParam;
        participantParam.rtps.builtin.domainId = domain_id;
        participantParam.rtps.builtin.leaseDuration = c_TimeInfinite;
        participantParam.rtps.setName(name);

        Participant *participant = Domain::createParticipant(participantParam);

        if(!participant)
        {
            RMW_SET_ERROR_MSG("create_node() could not create participant");
            return NULL;
        }

        rmw_node_t *node_handle = new rmw_node_t;
        node_handle->implementation_identifier = eprosima_fastrtps_identifier;
        node_handle->data = participant;

        return node_handle;
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

        Participant *participant = static_cast<Participant*>(node->data);
        if (!participant) {
            RMW_SET_ERROR_MSG("participant handle is null");
        }

        Domain::removeParticipant(participant);

        node->data = nullptr;
        delete(node);

        return RMW_RET_OK;
    }

    typedef struct CustomPublisherInfo
    {
        Publisher *publisher_;
        rmw_fastrtps_cpp::MessageTypeSupport *type_support_;
    } CustomPublisherInfo;

    rmw_publisher_t* rmw_create_publisher(const rmw_node_t *node, const rosidl_message_type_support_t *type_support,
            const char* topic_name, size_t queue_size)
    {
        assert(node);
        assert(type_support);
        assert(topic_name);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return NULL;
        }

        Participant *participant = static_cast<Participant*>(node->data);

        if(strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0)
        {
            RMW_SET_ERROR_MSG("type support not from this implementation");
            return NULL;
        }

        CustomPublisherInfo *info = new CustomPublisherInfo();

        const rosidl_typesupport_introspection_cpp::MessageMembers *members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers*>(type_support->data);
        info->type_support_ = new rmw_fastrtps_cpp::MessageTypeSupport(members);

        Domain::registerType(participant, info->type_support_);

        PublisherAttributes publisherParam;
        publisherParam.topic.topicKind = NO_KEY;
        publisherParam.topic.topicDataType = std::string(members->package_name_) + "::dds_::" + members->message_name_ + "_";
        publisherParam.topic.topicName = topic_name;

        info->publisher_ = Domain::createPublisher(participant, publisherParam, NULL);

        if(!info->publisher_)
        {
            RMW_SET_ERROR_MSG("create_publisher() could not create publisher");
            return NULL;
        }

        rmw_publisher_t *rmw_publisher = new rmw_publisher_t;
        rmw_publisher->implementation_identifier = eprosima_fastrtps_identifier;
        rmw_publisher->data = info;

        return rmw_publisher;
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
                delete info->type_support_;
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

        rmw_fastrtps_cpp::MessageTypeSupport::Buffer *buffer = (rmw_fastrtps_cpp::MessageTypeSupport::Buffer*)info->type_support_->createData();

        if(info->type_support_->serializeROSmessage(ros_message, buffer))
        {
            if(info->publisher_->write((void*)buffer))
                returnedValue = RMW_RET_OK;
            else
                RMW_SET_ERROR_MSG("cannot publish data");
        }
        else
            RMW_SET_ERROR_MSG("cannot serialize data");

        info->type_support_->deleteData(buffer);

        return returnedValue;
    }

    class SubListener;

    typedef struct CustomSubscriberInfo
    {
        Subscriber *subscriber_;
        SubListener *listener_;
        rmw_fastrtps_cpp::MessageTypeSupport *type_support_;
    } CustomSubscriberInfo;

    class SubListener : public SubscriberListener
    {
        public:

            SubListener(CustomSubscriberInfo *info) : info_(info), hasData_(false),
            conditionMutex_(NULL), conditionVariable_(NULL) {}

            void onSubscriptionMatched(Subscriber *sub, MatchingInfo &info) {}

            void onNewDataMessage(Subscriber *sub)
            {
                std::lock_guard<std::mutex> lock(internalMutex_);

                if(conditionMutex_ != NULL)
                {
                    std::unique_lock<std::mutex> clock(*conditionMutex_);
                    hasData_ = true;
                    clock.unlock();
                    conditionVariable_->notify_one();
                }
                else
                    hasData_ = true;

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
                return hasData_;
            }

            bool getHasData()
            {
                bool ret = hasData_;
                hasData_ = false;
                return ret;
            }

        private:

            CustomSubscriberInfo *info_;
            std::mutex internalMutex_;
            bool hasData_;
            std::mutex *conditionMutex_;
            std::condition_variable *conditionVariable_;
    };

    rmw_subscription_t* rmw_create_subscription(const rmw_node_t *node, const rosidl_message_type_support_t *type_support,
            const char *topic_name, size_t queue_size, bool ignore_local_publications)
    {
        assert(node);
        assert(type_support);
        assert(topic_name);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return NULL;
        }

        Participant *participant = static_cast<Participant*>(node->data);

        if(strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0)
        {
            RMW_SET_ERROR_MSG("type support not from this implementation");
            return NULL;
        }

        CustomSubscriberInfo *info = new CustomSubscriberInfo();

        const rosidl_typesupport_introspection_cpp::MessageMembers *members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers*>(type_support->data);
        info->type_support_ = new rmw_fastrtps_cpp::MessageTypeSupport(members);

        Domain::registerType(participant, info->type_support_);

        SubscriberAttributes subscriberParam;
        subscriberParam.topic.topicKind = NO_KEY;
        subscriberParam.topic.topicDataType = std::string(members->package_name_) + "::dds_::" + members->message_name_ + "_";
        subscriberParam.topic.topicName = topic_name;

        info->listener_ = new SubListener(info);
        info->subscriber_ = Domain::createSubscriber(participant, subscriberParam, info->listener_);

        if(!info->subscriber_)
        {
            RMW_SET_ERROR_MSG("create_subscriber() could not create subscriber");
            return NULL;
        }

        rmw_subscription_t *subscription = new rmw_subscription_t; 
        subscription->implementation_identifier = eprosima_fastrtps_identifier;
        subscription->data = info;

        return subscription;
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
                delete info->type_support_;
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

        rmw_fastrtps_cpp::MessageTypeSupport::Buffer *buffer = (rmw_fastrtps_cpp::MessageTypeSupport::Buffer*)info->type_support_->createData();
        SampleInfo_t sinfo;

        if(info->subscriber_->takeNextData(buffer, &sinfo))
        {
            if(sinfo.sampleKind == ALIVE)
            {
                info->type_support_->deserializeROSmessage(buffer, ros_message);
                *taken = true;
            }
        }

        info->type_support_->deleteData(buffer);

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

    class ServiceListener;

    typedef struct CustomServiceInfo
    {
        eprosima::rpc::transport::dds::RTPSServerTransport *transport_;
        eprosima::rpc::strategy::SingleThreadStrategy *strategy_;
        rmw_fastrtps_cpp::Protocol *protocol_;
        rmw_fastrtps_cpp::RequestTypeSupport *request_type_support_;
        rmw_fastrtps_cpp::ResponseTypeSupport *response_type_support_;
        eprosima::rpc::transport::dds::RTPSServerProcedureEndpoint *topic_endpoint_;
        ServiceListener *listener_;
    } CustomServiceInfo;

    class ServiceListener
    {
        public:

            ServiceListener(CustomServiceInfo *info) : info_(info),
            conditionMutex_(NULL), conditionVariable_(NULL) {}


            void onNewRequest(rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer *buffer)
            {
                assert(buffer);
                std::lock_guard<std::mutex> lock(internalMutex_);

                if(conditionMutex_ != NULL)
                {
                    std::unique_lock<std::mutex> clock(*conditionMutex_);
                    list.push_back(buffer);
                    clock.unlock();
                    conditionVariable_->notify_one();
                }
                else
                    list.push_back(buffer);

            }

            rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer* getRequest()
            {
                std::lock_guard<std::mutex> lock(internalMutex_);
                rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer *buffer = nullptr;

                if(conditionMutex_ != NULL)
                {
                    std::unique_lock<std::mutex> clock(*conditionMutex_);
                    buffer = list.front();
                    list.pop_front();
                }
                else
                {
                    buffer = list.front();
                    list.pop_front();
                }

                return buffer;
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
            std::list<rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer*> list;
            std::mutex *conditionMutex_;
            std::condition_variable *conditionVariable_;
    };

    rmw_client_t* rmw_create_client(const rmw_node_t *node,
            const rosidl_service_type_support_t *type_support,
            const char *service_name)
    {
        assert(node);
        assert(type_support);
        assert(service_name);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return NULL;
        }

        Participant *participant = static_cast<Participant*>(node->data);

        if(strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0)
        {
            RMW_SET_ERROR_MSG("type support not from this implementation");
            return NULL;
        }

        CustomClientInfo *info = new CustomClientInfo();

        const rosidl_typesupport_introspection_cpp::ServiceMembers *members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers*>(type_support->data);
        info->request_type_support_ = new rmw_fastrtps_cpp::RequestTypeSupport(members);
        info->response_type_support_ = new rmw_fastrtps_cpp::ResponseTypeSupport(members);

        info->transport_  = new eprosima::rpc::transport::dds::RTPSProxyTransport(participant, service_name, service_name);
        info->transport_->initialize();

        Domain::registerType(participant, info->request_type_support_);
        Domain::registerType(participant, info->response_type_support_);

        //TODO Change "Prueba"
        info->topic_endpoint_ = dynamic_cast<eprosima::rpc::transport::dds::RTPSProxyProcedureEndpoint*>(info->transport_->createProcedureEndpoint("Prueba",
                    info->request_type_support_->getName(),
                    info->request_type_support_->getName(),
                    info->response_type_support_->getName(),
                    info->response_type_support_->getName(),
                    (eprosima::rpc::transport::dds::RTPSTransport::Create_data)rmw_fastrtps_cpp::ResponseTypeSupport::create_data,
                    (eprosima::rpc::transport::dds::RTPSTransport::Copy_data)rmw_fastrtps_cpp::ResponseTypeSupport::copy_data,
                    (eprosima::rpc::transport::dds::RTPSTransport::Destroy_data)rmw_fastrtps_cpp::ResponseTypeSupport::delete_data,
                    NULL,
                    info->response_type_support_->m_typeSize));

        info->listener_ = new ClientListener(info);

        rmw_client_t *client = new rmw_client_t; 
        client->implementation_identifier = eprosima_fastrtps_identifier;
        client->data = info;

        return client;
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

        rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer *buffer = static_cast<rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer*>(info->request_type_support_->createData());

        if(info->request_type_support_->serializeROSmessage(ros_request, buffer))
        {
            AsyncTask *task = new AsyncTask(info);
            eprosima::rpc::ReturnMessage retcode = info->topic_endpoint_->send_async(buffer, task);

            switch (retcode)
            {
                case eprosima::rpc::OK:
                    returnedValue = RMW_RET_OK;
                    *sequence_id = ((int64_t)buffer->header.requestId().sequence_number().high()) << 32 | buffer->header.requestId().sequence_number().low();
                    break;
                case eprosima::rpc::CLIENT_INTERNAL_ERROR:
                    RMW_SET_ERROR_MSG("cannot send the request");
                    break;
                case eprosima::rpc::SERVER_NOT_FOUND:
                    RMW_SET_ERROR_MSG("cannot connect to the server");
                    break;
                default:
                    RMW_SET_ERROR_MSG("error sending the request");
                    break;
            }
        }
        else
            RMW_SET_ERROR_MSG("cannot serialize data");

        info->request_type_support_->deleteData(buffer);

        return returnedValue;
    }

    rmw_ret_t rmw_take_request(const rmw_service_t *service,
            void *ros_request_header,
            void *ros_request,
            bool *taken)
    {
        assert(service);
        assert(ros_request_header);
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

        rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer *buffer = info->listener_->getRequest();

        if(buffer != nullptr)
        {
            info->request_type_support_->deserializeROSmessage(buffer, ros_request);

            // Get header
            rmw_request_id_t &req_id = *(static_cast<rmw_request_id_t*>(ros_request_header));
            memcpy(req_id.writer_guid, buffer->header.requestId().writer_guid().guidPrefix(), 12);
            req_id.writer_guid[12] = buffer->header.requestId().writer_guid().entityId().entityKey()[0];
            req_id.writer_guid[13] = buffer->header.requestId().writer_guid().entityId().entityKey()[1];
            req_id.writer_guid[14] = buffer->header.requestId().writer_guid().entityId().entityKey()[2];
            req_id.writer_guid[15] = buffer->header.requestId().writer_guid().entityId().entityKind();
            req_id.sequence_number = ((int64_t)buffer->header.requestId().sequence_number().high()) << 32 | buffer->header.requestId().sequence_number().low();

            info->request_type_support_->deleteData(buffer);

            *taken = true;
        }

        return RMW_RET_OK;
    }

    rmw_ret_t rmw_take_response(const rmw_client_t *client,
            void *ros_request_header,
            void *ros_response,
            bool *taken)
    {
        assert(client);
        assert(ros_request_header);
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

        rmw_request_id_t &req_id = *(static_cast<rmw_request_id_t*>(ros_request_header));

        rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer *buffer = info->listener_->getResponse();

        if(buffer != nullptr)
        {
            info->response_type_support_->deserializeROSmessage(buffer, ros_response);

            req_id.sequence_number = ((int64_t)buffer->header.relatedRequestId().sequence_number().high()) << 32 | buffer->header.relatedRequestId().sequence_number().low();

            *taken = true;

            info->request_type_support_->deleteData(buffer);
        }

        return RMW_RET_OK;
    }

    rmw_ret_t rmw_send_response(const rmw_service_t *service,
            void *ros_request_header,
            void *ros_response)
    {
        assert(service);
        assert(ros_request_header);
        assert(ros_response);

        rmw_ret_t returnedValue = RMW_RET_ERROR;

        if(service->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("service handle not from this implementation");
            return RMW_RET_ERROR;
        }

        CustomServiceInfo *info = (CustomServiceInfo*)service->data;
        assert(info);

        rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer *buffer = static_cast<rmw_fastrtps_cpp::ResponseTypeSupport::ResponseBuffer*>(info->response_type_support_->createData());

        if(buffer != nullptr)
        {
            info->response_type_support_->serializeROSmessage(ros_response, buffer);

            //Set header
            rmw_request_id_t &req_id = *(static_cast<rmw_request_id_t*>(ros_request_header));
            memcpy(buffer->header.relatedRequestId().writer_guid().guidPrefix(), req_id.writer_guid, 12);
            buffer->header.relatedRequestId().writer_guid().entityId().entityKey()[0] = req_id.writer_guid[12];
            buffer->header.relatedRequestId().writer_guid().entityId().entityKey()[1] = req_id.writer_guid[13];
            buffer->header.relatedRequestId().writer_guid().entityId().entityKey()[2] = req_id.writer_guid[14];
            buffer->header.relatedRequestId().writer_guid().entityId().entityKind() = req_id.writer_guid[15];
            buffer->header.relatedRequestId().sequence_number().high((int32_t)((req_id.sequence_number & 0xFFFFFFFF00000000) >> 32));
            buffer->header.relatedRequestId().sequence_number().low((int32_t)(req_id.sequence_number & 0xFFFFFFFF));

            info->topic_endpoint_->sendReply(buffer);


            returnedValue = RMW_RET_OK;

        }

        info->response_type_support_->deleteData(buffer);

        return returnedValue;
    }

    void serve(eprosima::rpc::protocol::Protocol &protocol, void *data,
            eprosima::rpc::transport::Endpoint *endpoint)
    {
        assert(data);
        rmw_fastrtps_cpp::Protocol &proto = dynamic_cast<rmw_fastrtps_cpp::Protocol&>(protocol);
        ServiceListener *listener = proto.getInfo()->listener_;
        rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer *src = static_cast<rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer*>(data);
        rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer *dst = static_cast<rmw_fastrtps_cpp::RequestTypeSupport::RequestBuffer*>(proto.getInfo()->request_type_support_->createData());
        rmw_fastrtps_cpp::RequestTypeSupport::copy_data(dst, src);
        listener->onNewRequest(dst);
    }

    rmw_service_t *rmw_create_service(const rmw_node_t *node,
            const rosidl_service_type_support_t *type_support,
            const char *service_name)
    {
        assert(node);
        assert(type_support);
        assert(service_name);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            RMW_SET_ERROR_MSG("node handle not from this implementation");
            return NULL;
        }

        Participant *participant = static_cast<Participant*>(node->data);

        if(strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0)
        {
            RMW_SET_ERROR_MSG("type support not from this implementation");
            return NULL;
        }

        CustomServiceInfo *info = new CustomServiceInfo();

        const rosidl_typesupport_introspection_cpp::ServiceMembers *members = static_cast<const rosidl_typesupport_introspection_cpp::ServiceMembers*>(type_support->data);
        info->request_type_support_ = new rmw_fastrtps_cpp::RequestTypeSupport(members);
        info->response_type_support_ = new rmw_fastrtps_cpp::ResponseTypeSupport(members);

        info->strategy_ = new eprosima::rpc::strategy::SingleThreadStrategy();
        info->protocol_ = new rmw_fastrtps_cpp::Protocol(info);
        info->transport_  = new eprosima::rpc::transport::dds::RTPSServerTransport(participant, service_name, service_name);
        info->transport_->setStrategy(*info->strategy_);
        info->transport_->linkProtocol(*info->protocol_);
        info->transport_->initialize();

        Domain::registerType(participant, info->request_type_support_);
        Domain::registerType(participant, info->response_type_support_);

        //TODO Change "Prueba"
        info->topic_endpoint_ = dynamic_cast<eprosima::rpc::transport::dds::RTPSServerProcedureEndpoint*>(info->transport_->createProcedureEndpoint("Prueba",
                    info->response_type_support_->getName(),
                    info->response_type_support_->getName(),
                    info->request_type_support_->getName(),
                    info->request_type_support_->getName(),
                    (eprosima::rpc::transport::dds::RTPSTransport::Create_data)rmw_fastrtps_cpp::RequestTypeSupport::create_data,
                    (eprosima::rpc::transport::dds::RTPSTransport::Copy_data)rmw_fastrtps_cpp::RequestTypeSupport::copy_data,
                    (eprosima::rpc::transport::dds::RTPSTransport::Destroy_data)rmw_fastrtps_cpp::RequestTypeSupport::delete_data,
                    serve,
                    info->request_type_support_->m_typeSize));

        info->listener_ = new ServiceListener(info);
        info->transport_->run();

        rmw_service_t *service = new rmw_service_t; 
        service->implementation_identifier = eprosima_fastrtps_identifier;
        service->data = info;

        return service;
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
            if(info->transport_ != nullptr)
            {
                info->transport_->stop();
                delete info->transport_;
            }
            if(info->listener_ != nullptr)
                delete info->listener_;
            if(info->strategy_ != nullptr)
                delete info->strategy_;
            if(info->protocol_ != nullptr)
                delete info->protocol_;
            if(info->topic_endpoint_ != nullptr)
                delete info->topic_endpoint_;
            if(info->request_type_support_ != nullptr)
                delete info->request_type_support_;
            if(info->response_type_support_ != nullptr)
                delete info->response_type_support_;
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

        CustomClientInfo *info = (CustomClientInfo *)client->data;
        if(info != nullptr)
        {
            if(info->transport_ != nullptr)
            {
                delete info->transport_;
            }
            if(info->listener_ != nullptr)
                delete info->listener_;
            if(info->topic_endpoint_ != nullptr)
                delete info->topic_endpoint_;
            if(info->request_type_support_ != nullptr)
                delete info->request_type_support_;
            if(info->response_type_support_ != nullptr)
                delete info->response_type_support_;
            delete info;
        }
        delete(client);

        return RMW_RET_OK;
    }

    rmw_ret_t rmw_wait(rmw_subscriptions_t *subscriptions,
            rmw_guard_conditions_t *guard_conditions,
            rmw_services_t *services,
            rmw_clients_t *clients,
            rmw_time_t*)
    {
        std::mutex conditionMutex;
        std::condition_variable conditionVariable;

        for(unsigned long i = 0; i < subscriptions->subscriber_count; ++i)
        {
            void *data = subscriptions->subscribers[i];
            CustomSubscriberInfo *custom_subscriber_info = (CustomSubscriberInfo*)data;
            custom_subscriber_info->listener_->attachCondition(&conditionMutex, &conditionVariable);
        }

        for(unsigned long i = 0; i < clients->client_count; ++i)
        {
            void *data = clients->clients[i];
            CustomClientInfo *custom_client_info = (CustomClientInfo*)data;
            custom_client_info->listener_->attachCondition(&conditionMutex, &conditionVariable);
        }

        for(unsigned long i = 0; i < services->service_count; ++i)
        {
            void *data = services->services[i];
            CustomServiceInfo *custom_service_info = (CustomServiceInfo*)data;
            custom_service_info->listener_->attachCondition(&conditionMutex, &conditionVariable);
        }

        for(unsigned long i = 0; i < guard_conditions->guard_condition_count; ++i)
        {
            void *data = guard_conditions->guard_conditions[i];
            GuardCondition *guard_condition = (GuardCondition*)data;
            guard_condition->attachCondition(&conditionMutex, &conditionVariable);
        }

        std::unique_lock<std::mutex> lock(conditionMutex);

        // First check variables.
        bool hasToWait = true;

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

        for(unsigned long i = 0; hasToWait && i < guard_conditions->guard_condition_count; ++i)
        {
            void *data = guard_conditions->guard_conditions[i];
            GuardCondition *guard_condition = (GuardCondition*)data;
            if(guard_condition->hasTriggered())
                hasToWait = false;
        }

        if(hasToWait)
            conditionVariable.wait(lock);
        
        for(unsigned long i = 0; i < subscriptions->subscriber_count; ++i)
        {
            void *data = subscriptions->subscribers[i];
            CustomSubscriberInfo *custom_subscriber_info = (CustomSubscriberInfo*)data;
            if(!custom_subscriber_info->listener_->getHasData())
            {
                subscriptions->subscribers[i] = 0;
            }
            custom_subscriber_info->listener_->dettachCondition();
        }

        for(unsigned long i = 0; i < clients->client_count; ++i)
        {
            void *data = clients->clients[i];
            CustomClientInfo *custom_client_info = (CustomClientInfo*)data;
            if(!custom_client_info->listener_->hasData())
            {
                clients->clients[i] = 0;
            }
            custom_client_info->listener_->dettachCondition();
        }

        for(unsigned long i = 0; i < services->service_count; ++i)
        {
            void *data = services->services[i];
            CustomServiceInfo *custom_service_info = (CustomServiceInfo*)data;
            if(!custom_service_info->listener_->hasData())
            {
                services->services[i] = 0;
            }
            custom_service_info->listener_->dettachCondition();
        }

        for(unsigned long i = 0; i < guard_conditions->guard_condition_count; ++i)
        {
            void *data = guard_conditions->guard_conditions[i];
            GuardCondition *guard_condition = (GuardCondition*)data;
            if(!guard_condition->getHasTriggered())
            {
                guard_conditions->guard_conditions[i] = 0;
            }
            guard_condition->dettachCondition();
        }

        return RMW_RET_OK;
    }
}

