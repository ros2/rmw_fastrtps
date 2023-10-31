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

#include <utility>
#include <string>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "rcpputils/scope_exit.hpp"

#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/namespace_prefix.hpp"
#include "rmw_fastrtps_shared_cpp/qos.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

namespace rmw_fastrtps_shared_cpp
{
rmw_ret_t
__rmw_destroy_subscription(
  const char * identifier,
  const rmw_node_t * node,
  rmw_subscription_t * subscription,
  bool reset_cft)
{
  assert(node->implementation_identifier == identifier);
  assert(subscription->implementation_identifier == identifier);

  rmw_ret_t ret = RMW_RET_OK;
  rmw_error_state_t error_state;
  rmw_error_string_t error_string;
  auto common_context = static_cast<rmw_dds_common::Context *>(node->context->impl->common);
  auto info = static_cast<const CustomSubscriberInfo *>(subscription->data);
  // Update graph
  ret = common_context->remove_subscriber_graph(
    info->subscription_gid_,
    node->name, node->namespace_
  );
  if (RMW_RET_OK != ret) {
    error_state = *rmw_get_error_state();
    error_string = rmw_get_error_string();
    rmw_reset_error();
  }

  auto participant_info =
    static_cast<CustomParticipantInfo *>(node->context->impl->participant_info);
  rmw_ret_t local_ret = destroy_subscription(identifier, participant_info, subscription, reset_cft);
  if (RMW_RET_OK != local_ret) {
    if (RMW_RET_OK != ret) {
      RMW_SAFE_FWRITE_TO_STDERR(error_string.str);
      RMW_SAFE_FWRITE_TO_STDERR(" during '" RCUTILS_STRINGIFY(__function__) "'\n");
    }
    ret = local_ret;
  } else if (RMW_RET_OK != ret) {
    rmw_set_error_state(error_state.message, error_state.file, error_state.line_number);
  }
  return ret;
}

rmw_ret_t
__rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count)
{
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);

  *publisher_count = info->subscription_event_->publisher_count();

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_subscription_get_actual_qos(
  const rmw_subscription_t * subscription,
  rmw_qos_profile_t * qos)
{
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  eprosima::fastdds::dds::DataReader * fastdds_dr = info->data_reader_;
  const eprosima::fastdds::dds::DataReaderQos & dds_qos = fastdds_dr->get_qos();

  dds_qos_to_rmw_qos(dds_qos, qos);

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_subscription_set_content_filter(
  rmw_subscription_t * subscription,
  const rmw_subscription_content_filter_options_t * options
)
{
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  eprosima::fastdds::dds::ContentFilteredTopic * filtered_topic = info->filtered_topic_;
  const bool filter_expression_empty = (*options->filter_expression == '\0');

  if (!filtered_topic && filter_expression_empty) {
    // can't reset current subscriber
    RMW_SET_ERROR_MSG(
      "current subscriber has no content filter topic");
    return RMW_RET_ERROR;
  } else if (filtered_topic && !filter_expression_empty) {
    std::vector<std::string> expression_parameters;
    for (size_t i = 0; i < options->expression_parameters.size; ++i) {
      expression_parameters.push_back(options->expression_parameters.data[i]);
    }

    ReturnCode_t ret =
      filtered_topic->set_filter_expression(options->filter_expression, expression_parameters);
    if (ret != ReturnCode_t::RETCODE_OK) {
      RMW_SET_ERROR_MSG(
        "failed to set_filter_expression");
      return RMW_RET_ERROR;
    }
    return RMW_RET_OK;
  }

  eprosima::fastdds::dds::DomainParticipant * dds_participant =
    info->dds_participant_;
  eprosima::fastdds::dds::TopicDescription * des_topic = nullptr;
  const char * identifier = subscription->implementation_identifier;

  rmw_ret_t ret = rmw_fastrtps_shared_cpp::__rmw_destroy_subscription(
    identifier,
    info->node_,
    subscription,
    true /* reset_cft */);
  if (ret != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("delete subscription with reset cft");
    return RMW_RET_ERROR;
  }

  if (!filtered_topic) {
    // create content filtered topic
    eprosima::fastdds::dds::ContentFilteredTopic * filtered_topic = nullptr;
    if (!rmw_fastrtps_shared_cpp::create_content_filtered_topic(
        dds_participant, info->topic_,
        info->topic_name_mangled_, options, &filtered_topic))
    {
      RMW_SET_ERROR_MSG("create_contentfilteredtopic() failed to create contentfilteredtopic");
      return RMW_RET_ERROR;
    }
    info->filtered_topic_ = filtered_topic;
    des_topic = filtered_topic;
  } else {
    // use the existing parent topic
    des_topic = info->topic_;
  }

  // create data reader
  eprosima::fastdds::dds::Subscriber * subscriber = info->subscriber_;
  const rmw_subscription_options_t * subscription_options =
    &subscription->options;
  if (!rmw_fastrtps_shared_cpp::create_datareader(
      info->datareader_qos_,
      subscription_options,
      subscriber,
      des_topic,
      info->data_reader_listener_,
      &info->data_reader_))
  {
    RMW_SET_ERROR_MSG("create_datareader() could not create data reader");
    return RMW_RET_ERROR;
  }

  // Initialize DataReader's StatusCondition to be notified when new data is available
  info->data_reader_->get_statuscondition().set_enabled_statuses(
    eprosima::fastdds::dds::StatusMask::data_available());

  // lambda to delete datareader
  auto cleanup_datareader = rcpputils::make_scope_exit(
    [subscriber, info]()
    {
      subscriber->delete_datareader(info->data_reader_);
    });

  /////
  // Update RMW GID
  info->subscription_gid_ = rmw_fastrtps_shared_cpp::create_rmw_gid(
    identifier, info->data_reader_->guid());

  rmw_dds_common::Context * common_context = info->common_context_;
  const rmw_node_t * node = info->node_;

  // Update graph
  ret = common_context->add_subscriber_graph(
    info->subscription_gid_,
    node->name, node->namespace_
  );
  if (RMW_RET_OK != ret) {
    return RMW_RET_ERROR;
  }

  cleanup_datareader.cancel();
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_subscription_get_content_filter(
  const rmw_subscription_t * subscription,
  rcutils_allocator_t * allocator,
  rmw_subscription_content_filter_options_t * options
)
{
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  eprosima::fastdds::dds::ContentFilteredTopic * filtered_topic = info->filtered_topic_;

  if (nullptr == filtered_topic) {
    RMW_SET_ERROR_MSG("this subscriber has not created a ContentFilteredTopic");
    return RMW_RET_ERROR;
  }
  std::vector<std::string> expression_parameters;
  ReturnCode_t ret = filtered_topic->get_expression_parameters(expression_parameters);
  if (ret != ReturnCode_t::RETCODE_OK) {
    RMW_SET_ERROR_MSG(
      "failed to get_expression_parameters");
    return RMW_RET_ERROR;
  }

  std::vector<const char *> string_array;
  for (size_t i = 0; i < expression_parameters.size(); ++i) {
    string_array.push_back(expression_parameters[i].c_str());
  }

  return rmw_subscription_content_filter_options_init(
    filtered_topic->get_filter_expression().c_str(),
    string_array.size(),
    string_array.data(),
    allocator,
    options
  );
}

rmw_ret_t
__rmw_subscription_set_on_new_message_callback(
  rmw_subscription_t * rmw_subscription,
  rmw_event_callback_t callback,
  const void * user_data)
{
  auto custom_subscriber_info = static_cast<CustomSubscriberInfo *>(rmw_subscription->data);
  custom_subscriber_info->subscription_event_->set_on_new_message_callback(
    user_data,
    callback);
  return RMW_RET_OK;
}

}  // namespace rmw_fastrtps_shared_cpp
