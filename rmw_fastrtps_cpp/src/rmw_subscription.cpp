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
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/subscription.hpp"

#include "buffer_backend_context.hpp"
#include "buffer_endpoint_registry.hpp"
#include "rosidl_buffer_backend_registry/backend_utils.hpp"

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

  // Register buffer-aware publisher discovery callback.
  // CPU-only subscriptions use the shared CPU channel and don't need
  // per-publisher metadata tracking, so skip the callback for them.
  if (info->is_buffer_aware_ && !info->is_cpu_only_) {
    auto state = info->buffer_state_;
    auto * guard = info->buffer_data_guard_.get();
    auto * backend_context =
      static_cast<const rmw_fastrtps_cpp::BufferBackendContext *>(
      info->serialization_context_);
    auto * buf_registry = static_cast<rmw_fastrtps_cpp::BufferEndpointRegistry *>(
      node->context->impl->buffer_endpoint_registry);
    if (buf_registry) {
      buf_registry->register_publisher_discovery_callback(
        subscription->topic_name,
        info->subscription_gid_,
        [state, guard, backend_context](
          const rmw_fastrtps_cpp::BufferEndpointInfo & pub_info)
        {
          if (!state->alive.load()) {
            return;
          }

          std::string pub_hex = rmw_fastrtps_shared_cpp::gid_to_hex(pub_info.gid);

          {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (!state->alive.load()) {
              return;
            }
            if (state->publisher_metadata.count(pub_hex) > 0) {
              return;
            }

            PublisherBufferMetadata meta;
            meta.publisher_gid = pub_info.gid;
            meta.publisher_endpoint_info = rmw_get_zero_initialized_topic_endpoint_info();
            meta.publisher_endpoint_info.endpoint_type = RMW_ENDPOINT_PUBLISHER;
            std::memcpy(
              meta.publisher_endpoint_info.endpoint_gid,
              pub_info.gid.data, RMW_GID_STORAGE_SIZE);
            meta.backend_metadata = pub_info.backend_metadata;

            if (backend_context) {
              std::vector<rmw_topic_endpoint_info_t> existing_endpoints;
              existing_endpoints.reserve(state->publisher_metadata.size());
              for (const auto & [hex, m] : state->publisher_metadata) {
                existing_endpoints.push_back(m.publisher_endpoint_info);
              }

              std::unordered_map<std::string,
              std::vector<std::set<uint32_t>>> backend_endpoint_groups;
              (void)rosidl_buffer_backend_registry::notify_endpoint_discovered(
                backend_context->backend_instances,
                meta.publisher_endpoint_info,
                existing_endpoints,
                backend_endpoint_groups,
                pub_info.backend_metadata);
            }

            state->publisher_metadata[pub_hex] = std::move(meta);

            if (guard) {
              guard->set_trigger_value(true);
            }
          }

          RCUTILS_LOG_INFO_NAMED(
            "rmw_fastrtps_cpp",
            "Buffer subscription: publisher discovered, recorded '%s'",
            pub_hex.c_str());
        });
    }
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
    {
      auto & state = *info->buffer_state_;
      std::lock_guard<std::mutex> lock(state.mutex);
      state.alive.store(false);
      state.publisher_metadata.clear();
    }

    auto * buf_registry = static_cast<rmw_fastrtps_cpp::BufferEndpointRegistry *>(
      node->context->impl->buffer_endpoint_registry);
    if (buf_registry) {
      buf_registry->unregister_callbacks(info->subscription_gid_);
    }

    auto * participant_info =
      static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);

    // Clean up the accelerated channel DataReader and Topic.
    if (info->accel_data_reader_) {
      info->subscriber_->delete_datareader(info->accel_data_reader_);
      info->accel_data_reader_ = nullptr;
    }
    info->accel_data_reader_listener_.reset();
    if (info->accel_topic_) {
      info->dds_participant_->delete_topic(info->accel_topic_);
      info->accel_topic_ = nullptr;
    }

    // Clean up the shared CPU channel DataReader and Topic.
    if (info->cpu_data_reader_) {
      info->subscriber_->delete_datareader(info->cpu_data_reader_);
      info->cpu_data_reader_ = nullptr;
    }
    info->cpu_data_reader_listener_.reset();
    if (info->cpu_topic_) {
      participant_info->delete_topic(info->cpu_topic_, nullptr);
      info->cpu_topic_ = nullptr;
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
