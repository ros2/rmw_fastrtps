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
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/get_topic_endpoint_info.h"
#include "rmw/rmw.h"

#include "rcpputils/scope_exit.hpp"
#include "rcutils/logging_macros.h"

#include "rmw/impl/cpp/macros.hpp"

#include "rmw_dds_common/qos.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/custom_publisher_info.hpp"
#include "rmw_fastrtps_shared_cpp/publisher.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/publisher.hpp"

#include "rmw_dds_common/context.hpp"
#include "rmw_dds_common/msg/participant_entities_info.hpp"

#include "buffer_backend_context.hpp"
#include "buffer_endpoint_registry.hpp"
#include "rosidl_buffer_backend_registry/backend_utils.hpp"

extern "C"
{
rmw_ret_t
rmw_init_publisher_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_publisher_allocation_t * allocation)
{
  // Unused in current implementation.
  (void) type_support;
  (void) message_bounds;
  (void) allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_fini_publisher_allocation(rmw_publisher_allocation_t * allocation)
{
  // Unused in current implementation.
  (void) allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_UNSUPPORTED;
}

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options)
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
  rmw_ret_t ret = rmw_dds_common::qos_profile_get_best_available_for_topic_publisher(
    node, topic_name, &adapted_qos_policies, rmw_get_subscriptions_info_by_topic);
  if (RMW_RET_OK != ret) {
    return nullptr;
  }


  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);

  rmw_publisher_t * publisher = rmw_fastrtps_cpp::create_publisher(
    participant_info,
    type_supports,
    topic_name,
    &adapted_qos_policies,
    publisher_options);

  if (!publisher) {
    return nullptr;
  }
  auto cleanup_publisher = rcpputils::make_scope_exit(
    [participant_info, publisher]() {
      rmw_error_state_t error_state = *rmw_get_error_state();
      rmw_reset_error();
      if (RMW_RET_OK != rmw_fastrtps_shared_cpp::destroy_publisher(
        eprosima_fastrtps_identifier, participant_info, publisher))
      {
        RMW_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
        RMW_SAFE_FWRITE_TO_STDERR(" during '" RCUTILS_STRINGIFY(__function__) "' cleanup\n");
        rmw_reset_error();
      }
      rmw_set_error_state(error_state.message, error_state.file, error_state.line_number);
    });

  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<CustomPublisherInfo *>(publisher->data);

  // Update graph
  if (RMW_RET_OK != common_context->add_publisher_graph(
      info->publisher_gid,
      node->name, node->namespace_))
  {
    return nullptr;
  }

  // Register buffer-aware subscriber discovery callback
  if (info->is_buffer_aware_) {
    auto state = info->buffer_state_;
    auto pub_gid = info->publisher_gid;
    std::string base_topic = info->topic_->get_name();
    auto * backend_context =
      static_cast<const rmw_fastrtps_cpp::BufferBackendContext *>(info->serialization_context_);
    auto * buf_registry = static_cast<rmw_fastrtps_cpp::BufferEndpointRegistry *>(
      node->context->impl->buffer_endpoint_registry);
    if (buf_registry) {
      buf_registry->register_subscriber_discovery_callback(
        publisher->topic_name,
        info->publisher_gid,
        [state, pub_gid, base_topic, backend_context](
          const rmw_fastrtps_cpp::BufferEndpointInfo & sub_info) {
          if (!state->alive.load()) {
            return;
          }

          // Detect whether the discovered subscriber is CPU-only.
          // CPU-only subscribers advertise only {"cpu": ""} in their backend_metadata.
          bool sub_is_cpu_only = (sub_info.backend_metadata.size() == 1 &&
          sub_info.backend_metadata.count("cpu") == 1) ||
          sub_info.backend_metadata.empty();

          {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (!state->alive.load()) {
              return;
            }

            // Check for duplicate in existing p2p endpoints or CPU-only list.
            for (const auto & ep : state->endpoints) {
              bool equal = false;
              rmw_ret_t ret = rmw_compare_gids_equal(
                &ep->target_subscriber_gid, &sub_info.gid, &equal);
              if (RMW_RET_OK != ret) {
                RCUTILS_LOG_ERROR_NAMED(
                  "rmw_fastrtps_cpp",
                  "Buffer publisher: rmw_compare_gids_equal failed during duplicate check");
                continue;
              }
              if (equal) {
                return;
              }
            }
            for (const auto & g : state->cpu_only_subscribers) {
              bool equal = false;
              rmw_ret_t ret = rmw_compare_gids_equal(&g, &sub_info.gid, &equal);
              if (RMW_RET_OK != ret) {
                RCUTILS_LOG_ERROR_NAMED(
                  "rmw_fastrtps_cpp",
                  "Buffer publisher: rmw_compare_gids_equal failed during duplicate check");
                continue;
              }
              if (equal) {
                return;
              }
            }

            if (sub_is_cpu_only) {
              // CPU-only subscriber: track its GID; the shared CPU channel
              // DataWriter will serve it -- no per-subscriber endpoint needed.
              state->cpu_only_subscribers.push_back(sub_info.gid);
              RCUTILS_LOG_INFO_NAMED(
                "rmw_fastrtps_cpp",
                "Buffer publisher: CPU-only subscriber discovered on '%s', "
                "served by shared CPU channel",
                base_topic.c_str());
              return;
            }

            // Non-CPU subscriber: create a peer-to-peer endpoint.
            for (const auto & p : state->pending) {
              bool equal = false;
              rmw_ret_t ret = rmw_compare_gids_equal(
                &p.target_subscriber_gid, &sub_info.gid, &equal);
              if (RMW_RET_OK != ret) {
                RCUTILS_LOG_ERROR_NAMED(
                  "rmw_fastrtps_cpp",
                  "Buffer publisher: rmw_compare_gids_equal failed during pending check");
                continue;
              }
              if (equal) {
                return;
              }
            }

            std::string sub_hex = rmw_fastrtps_shared_cpp::gid_to_hex(sub_info.gid);
            std::string unique_topic = base_topic + "/_buf/" + sub_hex;

            rmw_topic_endpoint_info_t discovered_endpoint_info =
            rmw_get_zero_initialized_topic_endpoint_info();
            discovered_endpoint_info.endpoint_type = RMW_ENDPOINT_SUBSCRIPTION;
            std::memcpy(
              discovered_endpoint_info.endpoint_gid,
              sub_info.gid.data, RMW_GID_STORAGE_SIZE);

            if (backend_context) {
              std::vector<rmw_topic_endpoint_info_t> existing_endpoints;
              existing_endpoints.reserve(state->endpoints.size() + state->pending.size());
              for (const auto & ep : state->endpoints) {
                existing_endpoints.push_back(ep->subscriber_endpoint_info);
              }
              for (const auto & p : state->pending) {
                existing_endpoints.push_back(p.subscriber_endpoint_info);
              }

              std::unordered_map<std::string,
              std::vector<std::set<uint32_t>>> backend_endpoint_groups;
              (void)rosidl_buffer_backend_registry::notify_endpoint_discovered(
                backend_context->backend_instances,
                discovered_endpoint_info,
                existing_endpoints,
                backend_endpoint_groups,
                sub_info.backend_metadata);
            }

            PendingBufferPublisher pending;
            pending.unique_topic = unique_topic;
            pending.target_subscriber_gid = sub_info.gid;
            pending.subscriber_endpoint_info = discovered_endpoint_info;
            pending.backend_metadata = sub_info.backend_metadata;
            state->pending.push_back(std::move(pending));

            RCUTILS_LOG_INFO_NAMED(
              "rmw_fastrtps_cpp",
              "Buffer publisher: non-CPU subscriber discovered, queued '%s'",
              unique_topic.c_str());
          }
        });
    }
  }

  cleanup_publisher.cancel();
  return publisher;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription_count, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_publisher_count_matched_subscriptions(
    publisher, subscription_count);
}

rmw_ret_t
rmw_publisher_assert_liveliness(const rmw_publisher_t * publisher)
{
  return rmw_fastrtps_shared_cpp::__rmw_publisher_assert_liveliness(
    eprosima_fastrtps_identifier, publisher);
}

rmw_ret_t
rmw_publisher_wait_for_all_acked(const rmw_publisher_t * publisher, rmw_time_t wait_timeout)
{
  return rmw_fastrtps_shared_cpp::__rmw_publisher_wait_for_all_acked(
    eprosima_fastrtps_identifier, publisher, wait_timeout);
}

rmw_ret_t
rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  return rmw_fastrtps_shared_cpp::__rmw_publisher_get_actual_qos(
    publisher, qos);
}

rmw_ret_t
rmw_borrow_loaned_message(
  const rmw_publisher_t * publisher,
  const rosidl_message_type_support_t * type_support,
  void ** ros_message)
{
  return rmw_fastrtps_shared_cpp::__rmw_borrow_loaned_message(
    eprosima_fastrtps_identifier, publisher, type_support, ros_message);
}

rmw_ret_t
rmw_return_loaned_message_from_publisher(
  const rmw_publisher_t * publisher,
  void * loaned_message)
{
  return rmw_fastrtps_shared_cpp::__rmw_return_loaned_message_from_publisher(
    eprosima_fastrtps_identifier, publisher, loaned_message);
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node,
    node->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher,
    publisher->implementation_identifier,
    eprosima_fastrtps_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<CustomPublisherInfo *>(publisher->data);
  if (info && info->is_buffer_aware_) {
    auto & state = *info->buffer_state_;
    std::vector<std::shared_ptr<BufferPublisherEndpoint>> endpoints_to_destroy;
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      state.alive.store(false);
      endpoints_to_destroy = std::move(state.endpoints);
      state.endpoints.clear();
      state.pending.clear();
      state.cpu_only_subscribers.clear();
    }

    auto * buf_registry = static_cast<rmw_fastrtps_cpp::BufferEndpointRegistry *>(
      node->context->impl->buffer_endpoint_registry);
    if (buf_registry) {
      buf_registry->unregister_callbacks(info->publisher_gid);
    }

    for (auto & endpoint : endpoints_to_destroy) {
      if (endpoint->data_writer) {
        info->dds_publisher_->delete_datawriter(endpoint->data_writer);
      }
      if (endpoint->topic && endpoint->owns_topic) {
        info->participant_->delete_topic(endpoint->topic);
      }
    }

    // Clean up the shared CPU channel DataWriter and Topic.
    if (info->cpu_data_writer_) {
      info->dds_publisher_->delete_datawriter(info->cpu_data_writer_);
      info->cpu_data_writer_ = nullptr;
    }
    if (info->cpu_topic_) {
      auto * participant_info =
        static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
      participant_info->delete_topic(info->cpu_topic_, nullptr);
      info->cpu_topic_ = nullptr;
    }
  }

  return rmw_fastrtps_shared_cpp::__rmw_destroy_publisher(
    eprosima_fastrtps_identifier, node, publisher);
}
}  // extern "C"
