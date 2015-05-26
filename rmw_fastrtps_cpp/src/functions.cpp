#include "rmw/rmw.h"
#include "rmw/error_handling.h"
#include "rosidl_typesupport_introspection_cpp/Identifier.h"
#include "rmw_fastrtps_cpp/MessageTypeSupport.h"

#include <fastrtps/Domain.h>
#include <fastrtps/participant/Participant.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/publisher/Publisher.h>
#include <fastrtps/attributes/PublisherAttributes.h>
#include <fastrtps/subscriber/Subscriber.h>
#include <fastrtps/subscriber/SubscriberListener.h>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastrtps/attributes/SubscriberAttributes.h>

#include <cassert>
#include <mutex>
#include <condition_variable>

using namespace eprosima::fastrtps;

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

    rmw_node_t* rmw_create_node(const char *name)
    {
        assert(name);

        ParticipantAttributes participantParam;
        participantParam.rtps.builtin.domainId = 0;
        participantParam.rtps.builtin.leaseDuration = c_TimeInfinite;
        participantParam.rtps.setName(name);

        Participant *participant = Domain::createParticipant(participantParam);

        if(!participant)
        {
            rmw_set_error_string("create_node() could not create participant");
            return NULL;
        }

        rmw_node_t *node_handle = new rmw_node_t;
        node_handle->implementation_identifier = eprosima_fastrtps_identifier;
        node_handle->data = participant;

        return node_handle;
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
            rmw_set_error_string("node handle not from this implementation");
            return NULL;
        }

        Participant *participant = static_cast<Participant*>(node->data);

        if(strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0)
        {
            rmw_set_error_string("type support not from this implementation");
            return NULL;
        }

        CustomPublisherInfo *info = new CustomPublisherInfo();

        const rosidl_typesupport_introspection_cpp::MessageMembers *members = (rosidl_typesupport_introspection_cpp::MessageMembers*)type_support->data;
        info->type_support_ = new rmw_fastrtps_cpp::MessageTypeSupport(members);

        Domain::registerType(participant, info->type_support_);

        PublisherAttributes publisherParam;
        publisherParam.topic.topicKind = NO_KEY;
        publisherParam.topic.topicDataType = std::string(members->package_name_) + "::dds_::" + members->message_name_ + "_";
        publisherParam.topic.topicName = topic_name;

        info->publisher_ = Domain::createPublisher(participant, publisherParam, NULL);

        if(!info->publisher_)
        {
            rmw_set_error_string("create_publisher() could not create publisher");
            return NULL;
        }

        rmw_publisher_t *rmw_publisher = new rmw_publisher_t;
        rmw_publisher->implementation_identifier = eprosima_fastrtps_identifier;
        rmw_publisher->data = info;

        return rmw_publisher;
    }


    rmw_ret_t rmw_publish(const rmw_publisher_t *publisher, const void *ros_message)
    {
        assert(publisher);
        assert(ros_message);
        rmw_ret_t returnedValue = RMW_RET_ERROR;

        if(publisher->implementation_identifier != eprosima_fastrtps_identifier)
        {
            rmw_set_error_string("publisher handle not from this implementation");
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
                rmw_set_error_string("cannot publish data");
        }
        else
            rmw_set_error_string("cannot serialize data");

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
            const char *topic_name, size_t queue_size)
    {
        assert(node);
        assert(type_support);
        assert(topic_name);

        if(node->implementation_identifier != eprosima_fastrtps_identifier)
        {
            rmw_set_error_string("node handle not from this implementation");
            return NULL;
        }

        Participant *participant = static_cast<Participant*>(node->data);

        if(strcmp(type_support->typesupport_identifier, rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier) != 0)
        {
            rmw_set_error_string("type support not from this implementation");
            return NULL;
        }

        CustomSubscriberInfo *info = new CustomSubscriberInfo();

        const rosidl_typesupport_introspection_cpp::MessageMembers *members = (rosidl_typesupport_introspection_cpp::MessageMembers*)type_support->data;
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
            rmw_set_error_string("create_subscriber() could not create subscriber");
            return NULL;
        }

        rmw_subscription_t *subscription = new rmw_subscription_t; 
        subscription->implementation_identifier = eprosima_fastrtps_identifier;
        subscription->data = info;

        return subscription;
    }

    rmw_ret_t rmw_take(const rmw_subscription_t *subscription, void *ros_message, bool *taken)
    {
        assert(subscription);
        assert(ros_message);
        assert(taken);

        *taken = false;

        if(subscription->implementation_identifier != eprosima_fastrtps_identifier)
        {
            rmw_set_error_string("publisher handle not from this implementation");
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
            rmw_set_error_string("guard condition handle not from this implementation");
            return RMW_RET_ERROR;
        }

        GuardCondition *guard_condition = (GuardCondition*)guard_condition_handle->data;
        guard_condition->trigger();
        return RMW_RET_OK;
    }

    rmw_ret_t rmw_wait(rmw_subscriptions_t *subscriptions,
            rmw_guard_conditions_t *guard_conditions,
            rmw_services_t *services,
            rmw_clients_t *clients,
            bool non_blocking)
    {
        std::mutex conditionMutex;
        std::condition_variable conditionVariable;

        for(unsigned long i = 0; i < subscriptions->subscriber_count; ++i)
        {
            void *data = subscriptions->subscribers[i];
            CustomSubscriberInfo *custom_subscriber_info = (CustomSubscriberInfo*)data;
            custom_subscriber_info->listener_->attachCondition(&conditionMutex, &conditionVariable);
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

    rmw_client_t* rmw_create_client(const rmw_node_t *node,
            const rosidl_service_type_support_t *type_support,
            const char *service_name)
    {
        assert(node);
        assert(type_support);
        assert(service_name);

        return NULL;
    }

    rmw_ret_t rmw_send_request(const rmw_client_t *client,
            const void *ros_request,
            int64_t *sequence_id)
    {
        return RMW_RET_ERROR;
    }

    rmw_ret_t rmw_take_request(const rmw_service_t *service,
            void *ros_request_header,
            void *ros_request,
            bool *taken)
    {
        *taken = false;
        return RMW_RET_ERROR;
    }

    rmw_ret_t rmw_take_response(const rmw_client_t *client,
            void *ros_request_header,
            void *ros_response,
            bool *taken)
    {
        *taken = false;
        return RMW_RET_ERROR;
    }

    rmw_ret_t rmw_send_response(const rmw_service_t *service,
            void *ros_request,
            void *ros_response)
    {
        return RMW_RET_ERROR;
    }

    rmw_service_t *rmw_create_service(const rmw_node_t *node,
            const rosidl_service_type_support_t *type_support,
            const char *service_name)
    {
        return NULL;
    }
    rmw_ret_t rmw_destroy_service(rmw_service_t *service)
    {
        return RMW_RET_ERROR;
    }

    rmw_ret_t rmw_destroy_client(rmw_client_t *client)
    {
        return RMW_RET_ERROR;
    }
}
