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

#ifndef RMW_FASTRTPS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_
#define RMW_FASTRTPS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_

#include <condition_variable>
#include <mutex>
#include <unordered_set>
#include <unordered_map>

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

#include "rmw_fastrtps_shared_cpp/guid_utils.hpp"
#include "rmw_fastrtps_shared_cpp/TypeSupport.hpp"

class ServiceListener;
class ServicePubListener;

enum class client_present_t
{
  FAILURE,   // an error occurred when checking
  MAYBE,    // reader not matched, writer still present
  YES,      // reader matched
  GONE      // neither reader nor writer
};

typedef struct CustomServiceInfo
{
  eprosima::fastdds::dds::TypeSupport request_type_support_{nullptr};
  const void * request_type_support_impl_{nullptr};
  eprosima::fastdds::dds::TypeSupport response_type_support_{nullptr};
  const void * response_type_support_impl_{nullptr};
  eprosima::fastdds::dds::DataReader * request_reader_{nullptr};
  eprosima::fastdds::dds::DataWriter * response_writer_{nullptr};

  ServiceListener * listener_{nullptr};
  ServicePubListener * pub_listener_{nullptr};

  const char * typesupport_identifier_{nullptr};
} CustomServiceInfo;

typedef struct CustomServiceRequest
{
  eprosima::fastrtps::rtps::SampleIdentity sample_identity_;
  eprosima::fastcdr::FastBuffer * buffer_;

  CustomServiceRequest()
  : buffer_(nullptr)
  {
  }
} CustomServiceRequest;

class ServicePubListener : public eprosima::fastdds::dds::DataWriterListener
{
  using subscriptions_set_t =
    std::unordered_set<eprosima::fastrtps::rtps::GUID_t,
      rmw_fastrtps_shared_cpp::hash_fastrtps_guid>;
  using clients_endpoints_map_t =
    std::unordered_map<eprosima::fastrtps::rtps::GUID_t,
      eprosima::fastrtps::rtps::GUID_t,
      rmw_fastrtps_shared_cpp::hash_fastrtps_guid>;

public:
  explicit ServicePubListener(
    CustomServiceInfo * info)
  {
    (void) info;
  }

  void
  on_publication_matched(
    eprosima::fastdds::dds::DataWriter * /* writer */,
    const eprosima::fastdds::dds::PublicationMatchedStatus & info) final
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (info.current_count_change == 1) {
      subscriptions_.insert(eprosima::fastrtps::rtps::iHandle2GUID(info.last_subscription_handle));
    } else if (info.current_count_change == -1) {
      eprosima::fastrtps::rtps::GUID_t erase_endpoint_guid =
        eprosima::fastrtps::rtps::iHandle2GUID(info.last_subscription_handle);
      subscriptions_.erase(erase_endpoint_guid);
      auto endpoint = clients_endpoints_.find(erase_endpoint_guid);
      if (endpoint != clients_endpoints_.end()) {
        clients_endpoints_.erase(endpoint->second);
        clients_endpoints_.erase(erase_endpoint_guid);
      }
    } else {
      return;
    }
    cv_.notify_all();
  }

  template<class Rep, class Period>
  bool
  wait_for_subscription(
    const eprosima::fastrtps::rtps::GUID_t & guid,
    const std::chrono::duration<Rep, Period> & rel_time)
  {
    auto guid_is_present = [this, guid]() RCPPUTILS_TSA_REQUIRES(mutex_)->bool
    {
      return subscriptions_.find(guid) != subscriptions_.end();
    };

    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, rel_time, guid_is_present);
  }

  client_present_t
  check_for_subscription(
    const eprosima::fastrtps::rtps::GUID_t & guid)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      // Check if the guid is still in the map
      if (clients_endpoints_.find(guid) == clients_endpoints_.end()) {
        // Client is gone
        return client_present_t::GONE;
      }
    }
    // Wait for subscription
    if (!wait_for_subscription(guid, std::chrono::milliseconds(100))) {
      return client_present_t::MAYBE;
    }
    return client_present_t::YES;
  }

  void endpoint_erase_if_exists(
    const eprosima::fastrtps::rtps::GUID_t & endpointGuid)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto endpoint = clients_endpoints_.find(endpointGuid);
    if (endpoint != clients_endpoints_.end()) {
      clients_endpoints_.erase(endpoint->second);
      clients_endpoints_.erase(endpointGuid);
    }
  }

  void endpoint_add_reader_and_writer(
    const eprosima::fastrtps::rtps::GUID_t & readerGuid,
    const eprosima::fastrtps::rtps::GUID_t & writerGuid)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_endpoints_.emplace(readerGuid, writerGuid);
    clients_endpoints_.emplace(writerGuid, readerGuid);
  }

private:
  std::mutex mutex_;
  subscriptions_set_t subscriptions_ RCPPUTILS_TSA_GUARDED_BY(mutex_);
  clients_endpoints_map_t clients_endpoints_ RCPPUTILS_TSA_GUARDED_BY(mutex_);
  std::condition_variable cv_;
};

class ServiceListener : public eprosima::fastdds::dds::DataReaderListener
{
public:
  explicit ServiceListener(
    CustomServiceInfo * info)
  : info_(info)
  {
  }

  void
  on_subscription_matched(
    eprosima::fastdds::dds::DataReader *,
    const eprosima::fastdds::dds::SubscriptionMatchedStatus & info) final
  {
    if (info.current_count_change == -1) {
      info_->pub_listener_->endpoint_erase_if_exists(
        eprosima::fastrtps::rtps::iHandle2GUID(info.last_publication_handle));
    }
  }

  size_t get_unread_resquests()
  {
    return info_->request_reader_->get_unread_count(true);
  }

  void
  on_data_available(
    eprosima::fastdds::dds::DataReader *) final
  {
    std::unique_lock<std::mutex> lock_mutex(on_new_request_m_);

    auto unread_requests = get_unread_resquests();

    if (0u < unread_requests) {
      on_new_request_cb_(user_data_, unread_requests);
    }
  }

  // Provide handlers to perform an action when a
  // new event from this listener has ocurred
  void
  set_on_new_request_callback(
    const void * user_data,
    rmw_event_callback_t callback)
  {
    std::unique_lock<std::mutex> lock_mutex(on_new_request_m_);

    if (callback) {
      auto unread_requests = get_unread_resquests();

      if (0 < unread_requests) {
        callback(user_data, unread_requests);
      }

      user_data_ = user_data;
      on_new_request_cb_ = callback;

      eprosima::fastdds::dds::StatusMask status_mask = info_->request_reader_->get_status_mask();
      status_mask |= eprosima::fastdds::dds::StatusMask::data_available();
      info_->request_reader_->set_listener(this, status_mask);
    } else {
      eprosima::fastdds::dds::StatusMask status_mask = info_->request_reader_->get_status_mask();
      status_mask &= ~eprosima::fastdds::dds::StatusMask::data_available();
      info_->request_reader_->set_listener(this, status_mask);

      user_data_ = nullptr;
      on_new_request_cb_ = nullptr;
    }
  }

private:
  CustomServiceInfo * info_;

  rmw_event_callback_t on_new_request_cb_{nullptr};

  const void * user_data_{nullptr};

  std::mutex on_new_request_m_;
};

#endif  // RMW_FASTRTPS_SHARED_CPP__CUSTOM_SERVICE_INFO_HPP_
