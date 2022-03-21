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

#include <memory>
#include <utility>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/serialized_message.h"
#include "rmw/rmw.h"

#include "fastdds/dds/subscriber/SampleInfo.hpp"

#include "fastrtps/utils/collections/ResourceLimitedVector.hpp"

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"
#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/subscription.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"
#include "rmw_fastrtps_shared_cpp/utils.hpp"

#include "tracetools/tracetools.h"

namespace rmw_fastrtps_shared_cpp
{

using DataSharingKind = eprosima::fastdds::dds::DataSharingKind;

void
_assign_message_info(
  const char * identifier,
  rmw_message_info_t * message_info,
  const eprosima::fastdds::dds::SampleInfo * sinfo)
{
  message_info->source_timestamp = sinfo->source_timestamp.to_ns();
  message_info->received_timestamp = sinfo->reception_timestamp.to_ns();
  auto fastdds_sn = sinfo->sample_identity.sequence_number();
  message_info->publication_sequence_number =
    (static_cast<uint64_t>(fastdds_sn.high) << 32) |
    static_cast<uint64_t>(fastdds_sn.low);
  message_info->reception_sequence_number = RMW_MESSAGE_INFO_SEQUENCE_NUMBER_UNSUPPORTED;
  rmw_gid_t * sender_gid = &message_info->publisher_gid;
  sender_gid->implementation_identifier = identifier;
  memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);

  rmw_fastrtps_shared_cpp::copy_from_fastrtps_guid_to_byte_array(
    sinfo->sample_identity.writer_guid(),
    sender_gid->data);
}

rmw_ret_t
_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void) allocation;
  *taken = false;

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle,
    subscription->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  eprosima::fastdds::dds::SampleInfo sinfo;

  rmw_fastrtps_shared_cpp::SerializedData data;

  data.is_cdr_buffer = false;
  data.data = ros_message;
  data.impl = info->type_support_impl_;

  while (0 < info->data_reader_->get_unread_count()) {
    if (info->data_reader_->take_next_sample(&data, &sinfo) == ReturnCode_t::RETCODE_OK) {
      // Update hasData from listener
      info->listener_->update_has_data(info->data_reader_);

      if (subscription->options.ignore_local_publications) {
        auto sample_writer_guid =
          eprosima::fastrtps::rtps::iHandle2GUID(sinfo.publication_handle);

        if (sample_writer_guid.guidPrefix == info->data_reader_->guid().guidPrefix) {
          // This is a local publication. Ignore it
          continue;
        }
      }

      if (sinfo.valid_data) {
        if (message_info) {
          _assign_message_info(identifier, message_info, &sinfo);
        }
        *taken = true;
        break;
      }
    }
  }

  TRACEPOINT(
    rmw_take,
    static_cast<const void *>(subscription),
    static_cast<const void *>(ros_message),
    (message_info ? message_info->source_timestamp : 0LL),
    *taken);
  return RMW_RET_OK;
}

rmw_ret_t
_take_sequence(
  const char * identifier,
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  *taken = 0;
  bool taken_flag = false;
  rmw_ret_t ret = RMW_RET_OK;

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle,
    subscription->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  // Limit the upper bound of reads to the number unread at the beginning.
  // This prevents any samples that are added after the beginning of the
  // _take_sequence call from being read.
  auto unread_count = info->data_reader_->get_unread_count();
  if (unread_count < count) {
    count = unread_count;
  }

  for (size_t ii = 0; ii < count; ++ii) {
    taken_flag = false;
    ret = _take(
      identifier, subscription, message_sequence->data[*taken],
      &taken_flag, &message_info_sequence->data[*taken], allocation);

    if (ret != RMW_RET_OK) {
      break;
    }

    if (taken_flag) {
      (*taken)++;
    }
  }

  message_sequence->size = *taken;
  message_info_sequence->size = *taken;

  return ret;
}

rmw_ret_t
__rmw_take_event(
  const char * identifier,
  const rmw_event_t * event_handle,
  void * event_info,
  bool * taken)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(event_handle, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(event_info, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  *taken = false;

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    event handle,
    event_handle->implementation_identifier,
    identifier,
    return RMW_RET_ERROR);

  auto event = static_cast<CustomEventInfo *>(event_handle->data);
  if (event->getListener()->takeNextEvent(event_handle->event_type, event_info)) {
    *taken = true;
    return RMW_RET_OK;
  }

  return RMW_RET_ERROR;
}

rmw_ret_t
__rmw_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(
    subscription, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    ros_message, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    taken, RMW_RET_INVALID_ARGUMENT);

  return _take(identifier, subscription, ros_message, taken, nullptr, allocation);
}

rmw_ret_t
__rmw_take_sequence(
  const char * identifier,
  const rmw_subscription_t * subscription,
  size_t count,
  rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence,
  size_t * taken,
  rmw_subscription_allocation_t * allocation)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(
    subscription, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    message_sequence, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    message_info_sequence, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    taken, RMW_RET_INVALID_ARGUMENT);

  if (0u == count) {
    RMW_SET_ERROR_MSG("count cannot be 0");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (count > message_sequence->capacity) {
    RMW_SET_ERROR_MSG("Insufficient capacity in message_sequence");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (count > message_info_sequence->capacity) {
    RMW_SET_ERROR_MSG("Insufficient capacity in message_info_sequence");
    return RMW_RET_INVALID_ARGUMENT;
  }

  return _take_sequence(
    identifier, subscription, count, message_sequence, message_info_sequence,
    taken, allocation);
}

rmw_ret_t
__rmw_take_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(
    message_info, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    taken, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    ros_message, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    subscription, RMW_RET_INVALID_ARGUMENT);

  return _take(identifier, subscription, ros_message, taken, message_info, allocation);
}

rmw_ret_t
_take_serialized_message(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void) allocation;
  *taken = false;

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription handle,
    subscription->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(info, "custom subscriber info is null", return RMW_RET_ERROR);

  eprosima::fastcdr::FastBuffer buffer;
  eprosima::fastdds::dds::SampleInfo sinfo;

  rmw_fastrtps_shared_cpp::SerializedData data;
  data.is_cdr_buffer = true;
  data.data = &buffer;
  data.impl = nullptr;    // not used when is_cdr_buffer is true

  if (info->data_reader_->take_next_sample(&data, &sinfo) == ReturnCode_t::RETCODE_OK) {
    // Update hasData from listener
    info->listener_->update_has_data(info->data_reader_);

    if (sinfo.valid_data) {
      auto buffer_size = static_cast<size_t>(buffer.getBufferSize());
      if (serialized_message->buffer_capacity < buffer_size) {
        auto ret = rmw_serialized_message_resize(serialized_message, buffer_size);
        if (ret != RMW_RET_OK) {
          return ret;  // Error message already set
        }
      }
      serialized_message->buffer_length = buffer_size;
      memcpy(serialized_message->buffer, buffer.getBuffer(), serialized_message->buffer_length);

      if (message_info) {
        _assign_message_info(identifier, message_info, &sinfo);
      }
      *taken = true;
    }
  }

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_take_serialized_message(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(
    subscription, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    serialized_message, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    taken, RMW_RET_INVALID_ARGUMENT);

  return _take_serialized_message(
    identifier, subscription, serialized_message, taken, nullptr, allocation);
}

rmw_ret_t
__rmw_take_serialized_message_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(
    subscription, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    serialized_message, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    taken, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_ARGUMENT_FOR_NULL(
    message_info, RMW_RET_INVALID_ARGUMENT);

  return _take_serialized_message(
    identifier, subscription, serialized_message, taken, message_info, allocation);
}

// ----------------- Loans related code ------------------------- //

struct GenericSequence : public eprosima::fastdds::dds::LoanableCollection
{
  GenericSequence() = default;

  void resize(size_type /*new_length*/) override
  {
    // This kind of collection should only be used with loans
    throw std::bad_alloc();
  }
};

struct LoanManager
{
  struct Item
  {
    GenericSequence data_seq{};
    eprosima::fastdds::dds::SampleInfoSeq info_seq{};
  };

  explicit LoanManager(const eprosima::fastrtps::ResourceLimitedContainerConfig & items_cfg)
  : items(items_cfg)
  {
  }

  void add_item(std::unique_ptr<Item> item)
  {
    std::lock_guard<std::mutex> guard(mtx);
    items.push_back(std::move(item));
  }

  std::unique_ptr<Item> erase_item(void * loaned_message)
  {
    std::unique_ptr<Item> ret{nullptr};

    std::lock_guard<std::mutex> guard(mtx);
    for (auto it = items.begin(); it != items.end(); ++it) {
      if (loaned_message == (*it)->data_seq.buffer()[0]) {
        ret = std::move(*it);
        items.erase(it);
        break;
      }
    }

    return ret;
  }

private:
  std::mutex mtx;
  using ItemVector = eprosima::fastrtps::ResourceLimitedVector<std::unique_ptr<Item>>;
  ItemVector items RCPPUTILS_TSA_GUARDED_BY(mtx);
};

void
__init_subscription_for_loans(
  rmw_subscription_t * subscription)
{
  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  const auto & qos = info->data_reader_->get_qos();
  bool has_data_sharing = DataSharingKind::OFF != qos.data_sharing().kind();
  subscription->can_loan_messages = has_data_sharing && info->type_support_->is_plain();
  if (subscription->can_loan_messages) {
    const auto & allocation_qos = qos.reader_resource_limits().outstanding_reads_allocation;
    info->loan_manager_ = std::make_shared<LoanManager>(allocation_qos);
  }
}

rmw_ret_t
__rmw_take_loaned_message_internal(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription, subscription->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (!subscription->can_loan_messages) {
    RMW_SET_ERROR_MSG("Loaning is not supported");
    return RMW_RET_UNSUPPORTED;
  }

  RMW_CHECK_ARGUMENT_FOR_NULL(loaned_message, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);

  auto item = std::make_unique<rmw_fastrtps_shared_cpp::LoanManager::Item>();

  while (ReturnCode_t::RETCODE_OK == info->data_reader_->take(item->data_seq, item->info_seq, 1)) {
    if (item->info_seq[0].valid_data) {
      if (nullptr != message_info) {
        _assign_message_info(identifier, message_info, &item->info_seq[0]);
      }
      *loaned_message = item->data_seq.buffer()[0];
      *taken = true;
      info->listener_->update_has_data(info->data_reader_);

      info->loan_manager_->add_item(std::move(item));

      return RMW_RET_OK;
    }

    // Should return loan before taking again
    info->data_reader_->return_loan(item->data_seq, item->info_seq);
  }

  // No data available, return loan information.
  *taken = false;
  info->listener_->update_has_data(info->data_reader_);
  return RMW_RET_OK;
}

rmw_ret_t
__rmw_return_loaned_message_from_subscription(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * loaned_message)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    subscription, subscription->implementation_identifier, identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (!subscription->can_loan_messages) {
    RMW_SET_ERROR_MSG("Loaning is not supported");
    return RMW_RET_UNSUPPORTED;
  }
  RMW_CHECK_ARGUMENT_FOR_NULL(loaned_message, RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<CustomSubscriberInfo *>(subscription->data);
  std::unique_ptr<rmw_fastrtps_shared_cpp::LoanManager::Item> item;
  item = info->loan_manager_->erase_item(loaned_message);
  if (item != nullptr) {
    if (!info->data_reader_->return_loan(item->data_seq, item->info_seq)) {
      RMW_SET_ERROR_MSG("Error returning loan");
      return RMW_RET_ERROR;
    }

    return RMW_RET_OK;
  }

  RMW_SET_ERROR_MSG("Trying to return message not loaned by this subscription");
  return RMW_RET_ERROR;
}
}  // namespace rmw_fastrtps_shared_cpp
