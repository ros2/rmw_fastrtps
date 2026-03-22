// Copyright 2019 Open Source Robotics Foundation, Inc.
// Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/get_topic_endpoint_info.h"
#include "rmw/rmw.h"

#include "rcpputils/scope_exit.hpp"
#include "rcutils/logging_macros.h"

#include "rmw_dds_common/qos.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/subscription.hpp"

#include "buffer_endpoint_registry.hpp"

namespace
{
/// Lightweight listener for per-publisher DataReaders in buffer-aware subscriptions.
/// Triggers the subscription's buffer guard condition so rmw_wait detects the data,
/// and also notifies via the callback path for callback-based executors.
class BufferDataReaderListener final : public eprosima::fastdds::dds::DataReaderListener
{
public:
  BufferDataReaderListener(
    eprosima::fastdds::dds::GuardCondition * guard,
    RMWSubscriptionEvent * event)
  : guard_(guard), event_(event) {}

  void on_data_available(eprosima::fastdds::dds::DataReader * reader) override
  {
    if (guard_) {
      guard_->set_trigger_value(true);
    }
    auto unread = reader->get_unread_count();
    if (event_) {
      event_->notify_buffer_data_available(unread > 0 ? static_cast<size_t>(unread) : 1);
    }
  }

private:
  eprosima::fastdds::dds::GuardCondition * guard_;
  RMWSubscriptionEvent * event_;
};
}  // namespace

extern "C"
{
rmw_ret_t
rmw_init_subscription_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_subscription_allocation_t * allocation)
{
  // Unused in current implementation.
  (void) type_support;
  (void) message_bounds;
  (void) allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_fini_subscription_allocation(rmw_subscription_allocation_t * allocation)
{
  // Unused in current implementation.
  (void) allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_UNSUPPORTED;
}

rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_subscription_options_t * subscription_options)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    eprosima_fastrtps_identifier,
    return nullptr);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos_policies, nullptr);

  // Adapt any 'best available' QoS options
  rmw_qos_profile_t adapted_qos_policies = *qos_policies;
  rmw_ret_t ret = rmw_dds_common::qos_profile_get_best_available_for_topic_subscription(
    node, topic_name, &adapted_qos_policies, rmw_get_publishers_info_by_topic);
  if (RMW_RET_OK != ret) {
    return nullptr;
  }

  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  rmw_subscription_t * subscription = rmw_fastrtps_cpp::create_subscription(
    participant_info,
    type_supports,
    topic_name,
    &adapted_qos_policies,
    subscription_options,
    false);  // use no keyed topic
  if (!subscription) {
    return nullptr;
  }
  auto cleanup_subscription = rcpputils::make_scope_exit(
    [participant_info, subscription]() {
      rmw_error_state_t error_state = *rmw_get_error_state();
      rmw_reset_error();
      if (RMW_RET_OK != rmw_fastrtps_shared_cpp::destroy_subscription(
        eprosima_fastrtps_identifier, participant_info, subscription))
      {
        RMW_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
        RMW_SAFE_FWRITE_TO_STDERR(" during '" RCUTILS_STRINGIFY(__function__) "' cleanup\n");
        rmw_reset_error();
      }
      rmw_set_error_state(error_state.message, error_state.file, error_state.line_number);
    });

  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);

  // Update graph
  if (RMW_RET_OK != common_context->add_subscriber_graph(
      info->subscription_gid_,
      node->name, node->namespace_))
  {
    return nullptr;
  }

  info->node_ = node;
  info->common_context_ = common_context;

  // Register buffer-aware publisher discovery callback
  if (info->is_buffer_aware_) {
    auto alive = info->buffer_alive_flag_;
    auto & buf_registry = rmw_fastrtps_cpp::BufferEndpointRegistry::get_instance();
    buf_registry.register_publisher_discovery_callback(
      subscription->topic_name,
      info->subscription_gid_,
      [info, alive](const rmw_fastrtps_cpp::BufferEndpointInfo & pub_info) {
        if (!alive->load()) {
          return;
        }

        auto gid_to_hex = [](const rmw_gid_t & gid, size_t bytes = 8) -> std::string {
          static const char hex_chars[] = "0123456789abcdef";
          std::string result;
          result.reserve(bytes * 2);
          for (size_t i = 0; i < bytes && i < RMW_GID_STORAGE_SIZE; ++i) {
            result += hex_chars[(gid.data[i] >> 4) & 0xF];
            result += hex_chars[gid.data[i] & 0xF];
          }
          return result;
        };

        std::string pub_hex = gid_to_hex(pub_info.gid);
        std::string sub_hex = gid_to_hex(info->subscription_gid_);
        std::string unique_topic = info->topic_name_mangled_ +
        "/_buf/" + pub_hex + "_" + sub_hex;

        {
          std::lock_guard<std::mutex> lock(info->buffer_mutex_);
          for (const auto & ep : info->buffer_endpoints_) {
            if (std::memcmp(ep->publisher_gid.data, pub_info.gid.data,
              RMW_GID_STORAGE_SIZE) == 0)
            {
              RCUTILS_LOG_DEBUG_NAMED(
                "rmw_fastrtps_cpp",
                "Buffer subscription: publisher already known, skipping");
              return;
            }
          }
          if (!info->pending_buffer_endpoints_.insert(unique_topic).second) {
            return;
          }
        }

        RCUTILS_LOG_INFO_NAMED(
          "rmw_fastrtps_cpp",
          "Buffer subscription: publisher discovered, computing compatibility for '%s'",
          unique_topic.c_str());

        auto endpoint = std::make_shared<BufferSubscriptionEndpoint>();
        endpoint->key = unique_topic;
        endpoint->publisher_gid = pub_info.gid;
        endpoint->backend_metadata = pub_info.backend_metadata;

        endpoint->publisher_endpoint_info = rmw_get_zero_initialized_topic_endpoint_info();
        endpoint->publisher_endpoint_info.endpoint_type = RMW_ENDPOINT_PUBLISHER;
        std::memcpy(
          endpoint->publisher_endpoint_info.endpoint_gid,
          pub_info.gid.data, RMW_GID_STORAGE_SIZE);

        // In single-process scenarios, the publisher may have already created
        // this topic on the same DDS participant. Reuse it if it exists.
        eprosima::fastdds::dds::Topic * topic = nullptr;
        auto * existing_desc =
        info->dds_participant_->lookup_topicdescription(unique_topic);
        if (existing_desc) {
          topic = dynamic_cast<eprosima::fastdds::dds::Topic *>(existing_desc);
          if (topic) {
            endpoint->owns_topic = false;
          }
        }
        if (!topic) {
          eprosima::fastdds::dds::TopicQos topic_qos =
          info->dds_participant_->get_default_topic_qos();
          topic = info->dds_participant_->create_topic(
            unique_topic,
            info->type_support_.get_type_name(),
            topic_qos);
        }
        if (!topic) {
          RCUTILS_LOG_ERROR_NAMED(
            "rmw_fastrtps_cpp",
            "Failed to create per-publisher topic '%s'", unique_topic.c_str());
          std::lock_guard<std::mutex> lock(info->buffer_mutex_);
          info->pending_buffer_endpoints_.erase(unique_topic);
          return;
        }
        endpoint->topic = topic;

        RCUTILS_LOG_INFO_NAMED(
          "rmw_fastrtps_cpp",
          "Buffer subscription: creating DataReader for '%s'", unique_topic.c_str());

        eprosima::fastdds::dds::DataReaderQos reader_qos =
        info->subscriber_->get_default_datareader_qos();
        reader_qos.endpoint().history_memory_policy =
        eprosima::fastdds::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
        reader_qos.data_sharing().off();
        reader_qos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
        reader_qos.history() = info->datareader_qos_.history();
        constexpr auto rep = eprosima::fastdds::dds::XCDR_DATA_REPRESENTATION;
        reader_qos.representation().clear();
        reader_qos.representation().m_value.push_back(rep);

        auto buffer_listener = std::make_shared<BufferDataReaderListener>(
          info->buffer_data_guard_.get(), info->subscription_event_);
        auto * data_reader = info->subscriber_->create_datareader(
          topic, reader_qos, buffer_listener.get(),
          eprosima::fastdds::dds::StatusMask::data_available());
        if (!data_reader) {
          if (endpoint->owns_topic) {
            info->dds_participant_->delete_topic(topic);
          }
          RCUTILS_LOG_ERROR_NAMED(
            "rmw_fastrtps_cpp",
            "Failed to create per-publisher DataReader for '%s'", unique_topic.c_str());
          std::lock_guard<std::mutex> lock(info->buffer_mutex_);
          info->pending_buffer_endpoints_.erase(unique_topic);
          return;
        }
        endpoint->data_reader = data_reader;
        endpoint->listener = buffer_listener;

        {
          std::lock_guard<std::mutex> lock(info->buffer_mutex_);
          info->buffer_endpoints_.push_back(endpoint);
          info->pending_buffer_endpoints_.erase(unique_topic);
        }
        RCUTILS_LOG_INFO_NAMED(
          "rmw_fastrtps_cpp",
          "Buffer subscription: created per-pub endpoint '%s'", unique_topic.c_str());
      });
  }

  cleanup_subscription.cancel();
  return subscription;
}

rmw_ret_t
rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription,
    subscription->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher_count, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_subscription_count_matched_publishers(
    subscription, publisher_count);
}

rmw_ret_t
rmw_subscription_get_actual_qos(
  const rmw_subscription_t * subscription,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription,
    subscription->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_subscription_get_actual_qos(subscription, qos);
}

rmw_ret_t
rmw_subscription_set_content_filter(
  rmw_subscription_t * subscription,
  const rmw_subscription_content_filter_options_t * options)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(options, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription,
    subscription->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  rmw_ret_t ret = rmw_fastrtps_shared_cpp::__rmw_subscription_set_content_filter(
    subscription, options);
  auto info = static_cast<const CustomSubscriberInfo *>(subscription->data);
  subscription->is_cft_enabled = (info && info->filtered_topic_);
  return ret;
}

rmw_ret_t
rmw_subscription_get_content_filter(
  const rmw_subscription_t * subscription,
  rcutils_allocator_t * allocator,
  rmw_subscription_content_filter_options_t * options)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(allocator, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(options, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription,
    subscription->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  return rmw_fastrtps_shared_cpp::__rmw_subscription_get_content_filter(
    subscription, allocator, options);
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription,
    subscription->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  if (info && info->is_buffer_aware_) {
    info->buffer_alive_flag_->store(false);

    rmw_fastrtps_cpp::BufferEndpointRegistry::get_instance().unregister_callbacks(
      info->subscription_gid_);

    std::vector<std::shared_ptr<BufferSubscriptionEndpoint>> endpoints_to_destroy;
    {
      std::lock_guard<std::mutex> lock(info->buffer_mutex_);
      endpoints_to_destroy = std::move(info->buffer_endpoints_);
      info->buffer_endpoints_.clear();
    }
    for (auto & endpoint : endpoints_to_destroy) {
      if (endpoint->data_reader) {
        info->subscriber_->delete_datareader(endpoint->data_reader);
      }
      if (endpoint->topic && endpoint->owns_topic) {
        info->dds_participant_->delete_topic(endpoint->topic);
      }
    }
  }

  return rmw_fastrtps_shared_cpp::__rmw_destroy_subscription(
    eprosima_fastrtps_identifier, node, subscription);
}

rmw_ret_t
rmw_subscription_set_on_new_message_callback(
  rmw_subscription_t * rmw_subscription,
  rmw_event_callback_t callback,
  const void * user_data)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(rmw_subscription, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_subscription_set_on_new_message_callback(
    rmw_subscription,
    callback,
    user_data);
}
}  // extern "C"
