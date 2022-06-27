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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <string>

#include "fastcdr/FastBuffer.h"

#include "fastdds/dds/core/status/PublicationMatchedStatus.hpp"
#include "fastdds/dds/core/status/SubscriptionMatchedStatus.hpp"
#include "fastdds/dds/publisher/DataWriter.hpp"
#include "fastdds/dds/publisher/DataWriterListener.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/DataReaderListener.hpp"
#include "fastdds/dds/subscriber/SampleInfo.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include "fastdds/rtps/common/Guid.h"
#include "fastdds/rtps/common/InstanceHandle.h"
#include "fastdds/rtps/common/SampleIdentity.h"

#include "rcpputils/thread_safety_annotations.hpp"

#include "rmw/event_callback_type.h"

#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

class ClientListener;
class ClientPubListener;

typedef struct CustomClientInfo
{
  eprosima::fastdds::dds::TypeSupport request_type_support_{nullptr};
  const void * request_type_support_impl_{nullptr};
  eprosima::fastdds::dds::TypeSupport response_type_support_{nullptr};
  const void * response_type_support_impl_{nullptr};
  eprosima::fastdds::dds::DataReader * response_reader_{nullptr};
  eprosima::fastdds::dds::DataWriter * request_writer_{nullptr};

  std::string request_topic_;
  std::string response_topic_;

  ClientListener * listener_{nullptr};
  eprosima::fastrtps::rtps::GUID_t writer_guid_;
  eprosima::fastrtps::rtps::GUID_t reader_guid_;

  const char * typesupport_identifier_{nullptr};
  ClientPubListener * pub_listener_{nullptr};
  std::atomic_size_t response_subscriber_matched_count_;
  std::atomic_size_t request_publisher_matched_count_;
} CustomClientInfo;

typedef struct CustomClientResponse
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  std::unique_ptr<eprosima::fastcdr::FastBuffer> buffer_;
  eprosima::fastdds::dds::SampleInfo sample_info_ {};
} CustomClientResponse;

class ClientListener : public eprosima::fastdds::dds::DataReaderListener
{
public:
  explicit ClientListener(CustomClientInfo * info)
  : info_(info), list_has_data_(false),
    conditionMutex_(nullptr), conditionVariable_(nullptr) {}


  void
  on_data_available(eprosima::fastdds::dds::DataReader * reader)
  {
    assert(reader);

    CustomClientResponse response;
    // Todo(sloretz) eliminate heap allocation pending eprosima/Fast-CDR#19
    response.buffer_.reset(new eprosima::fastcdr::FastBuffer());

    rmw_fastrtps_shared_cpp::SerializedData data;
    data.is_cdr_buffer = true;
    data.data = response.buffer_.get();
    data.impl = nullptr;    // not used when is_cdr_buffer is true
    while (reader->take_next_sample(&data, &response.sample_info_) == ReturnCode_t::RETCODE_OK) {
      if (response.sample_info_.valid_data) {
        response.sample_identity_ = response.sample_info_.related_sample_identity;

        if (response.sample_identity_.writer_guid() == info_->reader_guid_ ||
          response.sample_identity_.writer_guid() == info_->writer_guid_)
        {
          std::lock_guard<std::mutex> lock(internalMutex_);
          const eprosima::fastrtps::HistoryQosPolicy & history = reader->get_qos().history();
          if (eprosima::fastrtps::KEEP_LAST_HISTORY_QOS == history.kind) {
            while (list.size() >= static_cast<size_t>(history.depth)) {
              list.pop_front();
            }
          }

          if (conditionMutex_ != nullptr) {
            std::unique_lock<std::mutex> clock(*conditionMutex_);
            list.emplace_back(std::move(response));
            // the change to list_has_data_ needs to be mutually exclusive with
            // rmw_wait() which checks hasData() and decides if wait() needs to
            // be called
            list_has_data_.store(true);
            clock.unlock();
            conditionVariable_->notify_one();
          } else {
            list.emplace_back(std::move(response));
            list_has_data_.store(true);
          }

          std::unique_lock<std::mutex> lock_mutex(on_new_response_m_);

          if (on_new_response_cb_) {
            on_new_response_cb_(user_data_, 1);
          } else {
            unread_count_++;
          }
        }
      }
    }
  }

  bool
  getResponse(CustomClientResponse & response)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);

    if (conditionMutex_ != nullptr) {
      std::unique_lock<std::mutex> clock(*conditionMutex_);
      return popResponse(response);
    }
    return popResponse(response);
  }

  void
  attachCondition(std::mutex * conditionMutex, std::condition_variable * conditionVariable)
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = conditionMutex;
    conditionVariable_ = conditionVariable;
  }

  void
  detachCondition()
  {
    std::lock_guard<std::mutex> lock(internalMutex_);
    conditionMutex_ = nullptr;
    conditionVariable_ = nullptr;
  }

  bool
  hasData()
  {
    return list_has_data_.load();
  }

  void on_subscription_matched(
    eprosima::fastdds::dds::DataReader * ,
    const eprosima::fastdds::dds::SubscriptionMatchedStatus & info) final
  {
    if (info_ == nullptr) {
      return;
    }
    if (info.current_count_change == 1) {
      publishers_.insert(eprosima::fastrtps::rtps::iHandle2GUID(info.last_publication_handle));
    } else if (info.current_count_change == -1) {
      publishers_.erase(eprosima::fastrtps::rtps::iHandle2GUID(info.last_publication_handle));
    } else {
      return;
    }
    info_->response_subscriber_matched_count_.store(publishers_.size());
  }

  // Provide handlers to perform an action when a
  // new event from this listener has ocurred
  void
  set_on_new_response_callback(
    const void * user_data,
    rmw_event_callback_t callback)
  {
    std::unique_lock<std::mutex> lock_mutex(on_new_response_m_);

    if (callback) {
      // Push events arrived before setting the the executor callback
      if (unread_count_) {
        callback(user_data, unread_count_);
        unread_count_ = 0;
      }
      user_data_ = user_data;
      on_new_response_cb_ = callback;
    } else {
      user_data_ = nullptr;
      on_new_response_cb_ = nullptr;
    }
  }

private:
  bool popResponse(CustomClientResponse & response) RCPPUTILS_TSA_REQUIRES(internalMutex_)
  {
    if (!list.empty()) {
      response = std::move(list.front());
      list.pop_front();
      list_has_data_.store(!list.empty());
      return true;
    }
    return false;
  };

  CustomClientInfo * info_;
  std::mutex internalMutex_;
  std::list<CustomClientResponse> list RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::atomic_bool list_has_data_;
  std::mutex * conditionMutex_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::condition_variable * conditionVariable_ RCPPUTILS_TSA_GUARDED_BY(internalMutex_);
  std::set<eprosima::fastrtps::rtps::GUID_t> publishers_;

  rmw_event_callback_t on_new_response_cb_{nullptr};
  const void * user_data_{nullptr};
  std::mutex on_new_response_m_;
  uint64_t unread_count_ = 0;
};

class ClientPubListener : public eprosima::fastdds::dds::DataWriterListener
{
public:
  explicit ClientPubListener(CustomClientInfo * info)
  : info_(info)
  {
  }

  void on_publication_matched(
    eprosima::fastdds::dds::DataWriter * /* writer */,
    const eprosima::fastdds::dds::PublicationMatchedStatus & info) final
  {
    if (info_ == nullptr) {
      return;
    }
    if (info.current_count_change == 1) {
      subscriptions_.insert(eprosima::fastrtps::rtps::iHandle2GUID(info.last_subscription_handle));
    } else if (info.current_count_change == -1) {
      subscriptions_.erase(eprosima::fastrtps::rtps::iHandle2GUID(info.last_subscription_handle));
    } else {
      return;
    }
    info_->request_publisher_matched_count_.store(subscriptions_.size());
  }

private:
  CustomClientInfo * info_;
  std::set<eprosima::fastrtps::rtps::GUID_t> subscriptions_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_CLIENT_INFO_HPP_
