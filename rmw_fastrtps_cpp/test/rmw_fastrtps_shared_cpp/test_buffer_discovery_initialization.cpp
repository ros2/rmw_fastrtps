// Copyright 2026 Open Source Robotics Foundation, Inc.
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

#include <dlfcn.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

#include "fastdds/dds/subscriber/DataReader.hpp"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/topic_endpoint_info_array.h"
#include "sensor_msgs/msg/image.h"

#include "buffer_endpoint_registry.hpp"
#include "rmw_fastrtps_cpp/get_subscriber.hpp"
#include "rmw_fastrtps_shared_cpp/create_rmw_gid.hpp"
#include "rmw_fastrtps_shared_cpp/participant.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_context_impl.hpp"

namespace
{

using namespace std::chrono_literals;

struct InitializationGate;
thread_local InitializationGate * active_gate = nullptr;

// Forward real DDS callbacks unchanged, and signal only AFTER the production
// listener has finished processing the target reader.
class DiscoveryObserver : public eprosima::fastdds::dds::DomainParticipantListener
{
public:
  DiscoveryObserver(ParticipantListener * listener, InitializationGate & gate)
  : listener_(listener), gate_(gate) {}

  void on_participant_discovery(
    eprosima::fastdds::dds::DomainParticipant * participant,
    eprosima::fastdds::rtps::ParticipantDiscoveryStatus reason,
    const eprosima::fastdds::dds::ParticipantBuiltinTopicData & info,
    bool & ignored) override
  {
    listener_->on_participant_discovery(participant, reason, info, ignored);
  }

  void on_data_writer_discovery(
    eprosima::fastdds::dds::DomainParticipant * participant,
    eprosima::fastdds::rtps::WriterDiscoveryStatus reason,
    const eprosima::fastdds::rtps::PublicationBuiltinTopicData & info,
    bool & ignored) override
  {
    listener_->on_data_writer_discovery(participant, reason, info, ignored);
  }

  void on_data_reader_discovery(
    eprosima::fastdds::dds::DomainParticipant * participant,
    eprosima::fastdds::rtps::ReaderDiscoveryStatus reason,
    const eprosima::fastdds::dds::SubscriptionBuiltinTopicData & info,
    bool & ignored) override;

private:
  ParticipantListener * listener_;
  InitializationGate & gate_;
};

struct InitializationGate
{
  explicit InitializationGate(std::string topic)
  : dds_topic("rt" + topic) {}

  void after_participant_creation(CustomParticipantInfo * info)
  {
    observer = std::make_unique<DiscoveryObserver>(info->listener_, *this);
    const auto status = info->participant_->set_listener(
      observer.get(), eprosima::fastdds::dds::StatusMask::none());
    std::unique_lock<std::mutex> lock(mutex);
    if (status != eprosima::fastdds::dds::RETCODE_OK) {
      error = "Cannot attach discovery observer";
    }
    participant_created = true;
    cv.notify_all();
    if (!cv.wait_for(lock, 15s, [this] {return released;})) {
      error = "Timed out waiting to resume initialization";
    }
  }

  bool wait_for_participant()
  {
    std::unique_lock<std::mutex> lock(mutex);
    return cv.wait_for(lock, 15s, [this] {return participant_created;}) && error.empty();
  }

  bool wait_for_discovery()
  {
    std::unique_lock<std::mutex> lock(mutex);
    return cv.wait_for(lock, 15s, [this] {return reader_processed;});
  }

  void release()
  {
    std::lock_guard<std::mutex> lock(mutex);
    released = true;
    cv.notify_all();
  }

  const std::string dds_topic;
  std::unique_ptr<DiscoveryObserver> observer;
  std::mutex mutex;
  std::condition_variable cv;
  bool participant_created{false};
  bool released{false};
  bool reader_processed{false};
  bool discovered_during_initialization{false};
  std::atomic<bool> initialization_finished{false};
  rmw_gid_t discovered_gid{};
  std::unordered_map<std::string, std::string> discovered_backends;
  std::string error;
};

void DiscoveryObserver::on_data_reader_discovery(
  eprosima::fastdds::dds::DomainParticipant * participant,
  eprosima::fastdds::rtps::ReaderDiscoveryStatus reason,
  const eprosima::fastdds::dds::SubscriptionBuiltinTopicData & info,
  bool & ignored)
{
  listener_->on_data_reader_discovery(participant, reason, info, ignored);
  if (reason == eprosima::fastdds::rtps::ReaderDiscoveryStatus::DISCOVERED_READER &&
    info.topic_name.to_string() == gate_.dds_topic)
  {
    std::lock_guard<std::mutex> lock(gate_.mutex);
    if (gate_.reader_processed) {
      return;
    }
    gate_.discovered_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
      rmw_get_implementation_identifier(), info.guid);
    const auto & user_data = info.user_data.getValue();
    gate_.discovered_backends = parse_buffer_backends_from_user_data(
      user_data.data(), user_data.size());
    gate_.discovered_during_initialization = !gate_.initialization_finished.load();
    gate_.reader_processed = true;
    gate_.cv.notify_all();
  }
}

struct Context
{
  bool initialize(size_t domain)
  {
    auto allocator = rcutils_get_default_allocator();
    auto options = rmw_get_zero_initialized_init_options();
    if (rmw_init_options_init(&options, allocator) != RMW_RET_OK) {
      return false;
    }
    auto ret = rmw_enclave_options_copy("/", &allocator, &options.enclave);
    options.domain_id = domain;
    options.discovery_options.automatic_discovery_range = RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST;
    if (ret == RMW_RET_OK) {
      ret = rmw_init(&options, &context);
      initialized = ret == RMW_RET_OK;
    }
    EXPECT_EQ(RMW_RET_OK, rmw_init_options_fini(&options));
    return initialized;
  }

  ~Context()
  {
    if (subscription) {
      EXPECT_EQ(RMW_RET_OK, rmw_destroy_subscription(node, subscription));
    }
    if (node) {
      EXPECT_EQ(RMW_RET_OK, rmw_destroy_node(node));
    }
    if (initialized) {
      EXPECT_EQ(RMW_RET_OK, rmw_shutdown(&context));
      EXPECT_EQ(RMW_RET_OK, rmw_context_fini(&context));
    }
  }

  rmw_context_t context{rmw_get_zero_initialized_context()};
  rmw_node_t * node{nullptr};
  rmw_subscription_t * subscription{nullptr};
  bool initialized{false};
};

struct Initializer
{
  Initializer(Context & target, InitializationGate & gate)
  : gate_(gate), thread_([&target, &gate] {
      active_gate = &gate;
      target.node = rmw_create_node(&target.context, "initializing_node", "/");
      active_gate = nullptr;
      gate.initialization_finished.store(true);
    }) {}

  ~Initializer()
  {
    gate_.release();
    join();
  }

  void join()
  {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  InitializationGate & gate_;
  std::thread thread_;
};

struct TrackedReaders
{
  std::mutex mutex;
  std::vector<rmw_fastrtps_cpp::BufferEndpointInfo> readers;
};

void check_initialization_order(bool discover_during_initialization)
{
  SCOPED_TRACE(
    discover_during_initialization ? "discovery during initialization" :
    "discovery after initialization");
  const std::string topic = "/buffer_init_contract_" + std::to_string(getpid()) +
    (discover_during_initialization ? "_during" : "_after");
  // The observer must outlive both participants, including failure cleanup.
  InitializationGate gate(topic);
  Context peer;
  Context target;
  const size_t domain = 180 + static_cast<size_t>(getpid() % 30);
  ASSERT_TRUE(peer.initialize(domain));
  peer.node = rmw_create_node(&peer.context, "peer_node", "/");
  ASSERT_NE(nullptr, peer.node) << rmw_get_error_string().str;
  ASSERT_TRUE(target.initialize(domain));

  Initializer initializer(target, gate);
  ASSERT_TRUE(gate.wait_for_participant()) << "Initialization did not reach the participant gate";
  if (!discover_during_initialization) {
    gate.release();
    initializer.join();
    ASSERT_NE(nullptr, target.node) << rmw_get_error_string().str;
  }

  auto subscription_options = rmw_get_default_subscription_options();
  subscription_options.acceptable_buffer_backends = "cpu";
  const auto * type_support = ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Image);
  peer.subscription = rmw_create_subscription(
    peer.node, type_support, topic.c_str(), &rmw_qos_profile_default, &subscription_options);
  ASSERT_NE(nullptr, peer.subscription) << rmw_get_error_string().str;
  ASSERT_TRUE(gate.wait_for_discovery()) << "DDS did not deliver the target reader discovery";
  const auto expected_gid = rmw_fastrtps_shared_cpp::create_rmw_gid(
    rmw_get_implementation_identifier(),
    rmw_fastrtps_cpp::get_datareader(peer.subscription)->guid());
  EXPECT_EQ(
    0, std::memcmp(expected_gid.data, gate.discovered_gid.data, RMW_GID_STORAGE_SIZE));
  EXPECT_EQ(discover_during_initialization, gate.discovered_during_initialization);
  const std::unordered_map<std::string, std::string> expected_backends{{"cpu", ""}};
  ASSERT_EQ(expected_backends, gate.discovered_backends);

  gate.release();
  initializer.join();
  ASSERT_NE(nullptr, target.node) << rmw_get_error_string().str;
  ASSERT_TRUE(gate.error.empty()) << gate.error;
  ASSERT_TRUE(gate.initialization_finished.load());

  auto * common = static_cast<rmw_dds_common::Context *>(target.context.impl->common);
  auto * registry = static_cast<rmw_fastrtps_cpp::BufferEndpointRegistry *>(
    target.context.impl->buffer_endpoint_registry);
  auto allocator = rcutils_get_default_allocator();
  auto readers = rmw_get_zero_initialized_topic_endpoint_info_array();
  ASSERT_EQ(
    RMW_RET_OK, common->graph_cache.get_readers_info_by_topic(
      gate.dds_topic, [](const std::string & name) {return name;}, &allocator, &readers));
  bool graph_has_reader = false;
  for (size_t i = 0; i < readers.size; ++i) {
    if (std::memcmp(readers.info_array[i].endpoint_gid, expected_gid.data, RMW_GID_STORAGE_SIZE) == 0) {
      graph_has_reader = true;
      EXPECT_EQ(RMW_ENDPOINT_SUBSCRIPTION, readers.info_array[i].endpoint_type);
    }
  }
  EXPECT_EQ(RMW_RET_OK, rmw_topic_endpoint_info_array_fini(&readers, &allocator));
  ASSERT_TRUE(graph_has_reader);

  // Observe the production registry through its public known-endpoint replay.
  // The test never installs the buffer callback or inserts endpoint information.
  auto tracked = std::make_shared<TrackedReaders>();
  registry->register_subscriber_discovery_callback(
    topic, common->gid,
    [tracked](const rmw_fastrtps_cpp::BufferEndpointInfo & info) {
      std::lock_guard<std::mutex> lock(tracked->mutex);
      tracked->readers.push_back(info);
    });
  registry->unregister_callbacks(common->gid);
  std::lock_guard<std::mutex> lock(tracked->mutex);
  std::cout << "INITIALIZATION_CASE "
            << (discover_during_initialization ? "during" : "after")
            << " graph_has_reader=" << graph_has_reader
            << " tracked_readers=" << tracked->readers.size() << std::endl;
  ASSERT_EQ(1u, tracked->readers.size())
    << "Initialization completed, but the discovered subscriber is missing from Buffer tracking";
  const auto & info = tracked->readers.front();
  EXPECT_EQ(0, std::memcmp(expected_gid.data, info.gid.data, RMW_GID_STORAGE_SIZE));
  EXPECT_EQ(topic, info.topic_name);
  EXPECT_EQ(expected_backends, info.backend_metadata);
}

}  // namespace

namespace rmw_fastrtps_shared_cpp
{

// Linux ELF interposition is confined to this test executable. Forward to the
// real exported function, then hold its return until the selected DDS event has
// been processed. rmw_create_node still executes the production initialization.
CustomParticipantInfo * create_participant(
  const char * identifier,
  size_t domain_id,
  const rmw_security_options_t * security_options,
  const rmw_discovery_options_t * discovery_options,
  const char * enclave,
  rmw_dds_common::Context * common_context)
{
  using Function = decltype(&create_participant);
  static const auto original = []() -> Function {
      Dl_info symbol{};
      if (!dladdr(reinterpret_cast<void *>(&create_participant), &symbol) || !symbol.dli_sname) {
        return nullptr;
      }
      return reinterpret_cast<Function>(dlsym(RTLD_NEXT, symbol.dli_sname));
    }();
  if (!original) {
    RMW_SET_ERROR_MSG("Cannot resolve the real participant factory for the test");
    return nullptr;
  }
  auto * info = original(
    identifier, domain_id, security_options, discovery_options, enclave, common_context);
  if (info && active_gate) {
    active_gate->after_participant_creation(info);
  }
  return info;
}

}  // namespace rmw_fastrtps_shared_cpp

TEST(BufferDiscoveryInitialization, tracks_readers_across_initialization)
{
  check_initialization_order(false);
  check_initialization_order(true);
}
