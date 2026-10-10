// Copyright 2020 Open Source Robotics Foundation, Inc.
// Copyright 2026 Dennis Lanov
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

// Regression tests for https://github.com/ros2/rmw_fastrtps/issues/929:
// subscription QoS events share the DataReader's StatusCondition, so an
// untaken sample must not wake an rmw_wait that contains only the
// subscription's events, while a real QoS event still must.

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "fastdds/dds/core/condition/WaitSet.hpp"
#include "fastdds/dds/core/status/StatusMask.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"

#include "rcutils/allocator.h"

#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/rmw.h"
#include "rmw/subscription_content_filter_options.h"

#include "rmw_fastrtps_cpp/get_subscriber.hpp"

#include "test_msgs/msg/basic_types.h"

using namespace std::chrono_literals;

namespace
{

// Long enough that only a missed wakeup reaches it.
constexpr std::chrono::nanoseconds kEventTimeout = 10s;
// Used where rmw_wait is expected to time out.
constexpr std::chrono::nanoseconds kShortTimeout = 200ms;

// The events rclpy creates for every subscription, plus one that cannot fire with the
// default (infinite) deadline. None of them fire between compatible endpoints.
const std::vector<rmw_event_type_t> unrelated_event_types = {
  RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE,
  RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE,
  RMW_EVENT_REQUESTED_DEADLINE_MISSED,
};

rmw_time_t to_rmw_time(std::chrono::nanoseconds timeout)
{
  const auto sec = std::chrono::duration_cast<std::chrono::seconds>(timeout);
  rmw_time_t rmw_timeout;
  rmw_timeout.sec = static_cast<uint64_t>(sec.count());
  rmw_timeout.nsec = static_cast<uint64_t>((timeout - sec).count());
  return rmw_timeout;
}

struct WaitResult
{
  rmw_ret_t ret{RMW_RET_ERROR};
  std::vector<void *> ready;
  std::chrono::nanoseconds elapsed{0};
};

}  // namespace

class TestRmwWaitEvents : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    rmw_init_options_t options = rmw_get_zero_initialized_init_options();
    rmw_ret_t ret = rmw_init_options_init(&options, allocator);
    ASSERT_EQ(RMW_RET_OK, ret) << rcutils_get_error_string().str;
    ret = rmw_enclave_options_copy("/", &allocator, &options.enclave);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    // Only the local participant, so nothing outside this process can interfere.
    options.discovery_options.automatic_discovery_range = RMW_AUTOMATIC_DISCOVERY_RANGE_OFF;
    ret = rmw_init(&options, &context);
    rmw_ret_t fini_ret = rmw_init_options_fini(&options);
    EXPECT_EQ(RMW_RET_OK, fini_ret) << rmw_get_error_string().str;
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    context_initialized = true;
    node = rmw_create_node(&context, "test_rmw_wait_events", "/");
    ASSERT_NE(nullptr, node) << rmw_get_error_string().str;
    wait_set = rmw_create_wait_set(&context, 0);
    ASSERT_NE(nullptr, wait_set) << rmw_get_error_string().str;

    rmw_subscription_options_t sub_options = rmw_get_default_subscription_options();
    sub = rmw_create_subscription(node, ts, topic_name, &reliable_qos(), &sub_options);
    ASSERT_NE(nullptr, sub) << rmw_get_error_string().str;
  }

  void TearDown() override
  {
    rmw_ret_t ret;
    for (rmw_event_t & event : events) {
      ret = rmw_event_fini(&event);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
    for (rmw_publisher_t * pub : publishers) {
      ret = rmw_destroy_publisher(node, pub);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
    for (rmw_subscription_t * extra_sub : extra_subscriptions) {
      ret = rmw_destroy_subscription(node, extra_sub);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
    if (sub) {
      ret = rmw_destroy_subscription(node, sub);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
    if (wait_set) {
      ret = rmw_destroy_wait_set(wait_set);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
    if (node) {
      ret = rmw_destroy_node(node);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
    if (context_initialized) {
      ret = rmw_shutdown(&context);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
      ret = rmw_context_fini(&context);
      EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
  }

  static const rmw_qos_profile_t & reliable_qos()
  {
    static const rmw_qos_profile_t qos = [] {
        rmw_qos_profile_t qos = rmw_qos_profile_default;
        qos.reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
        qos.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
        qos.depth = 10;
        qos.durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
        return qos;
      }();
    return qos;
  }

  rmw_publisher_t * create_publisher(rmw_qos_reliability_policy_t reliability)
  {
    rmw_qos_profile_t qos = reliable_qos();
    qos.reliability = reliability;
    rmw_publisher_options_t options = rmw_get_default_publisher_options();
    rmw_publisher_t * pub = rmw_create_publisher(node, ts, topic_name, &qos, &options);
    if (pub) {
      publishers.push_back(pub);
    }
    return pub;
  }

  // Initializes subscription events, which are finalized in TearDown().
  void init_events(const std::vector<rmw_event_type_t> & event_types)
  {
    events.reserve(event_types.size());
    for (rmw_event_type_t event_type : event_types) {
      events.push_back(rmw_get_zero_initialized_event());
      rmw_ret_t ret = rmw_subscription_event_init(&events.back(), sub, event_type);
      ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    }
  }

  // rmw_wait() nulls the entries that are not ready, so the arrays are rebuilt on every call.
  WaitResult wait(bool include_subscription, bool include_events, std::chrono::nanoseconds timeout)
  {
    std::vector<void *> subscribers;
    if (include_subscription) {
      subscribers.push_back(sub->data);
    }
    std::vector<void *> event_handles;
    if (include_events) {
      for (rmw_event_t & event : events) {
        event_handles.push_back(&event);
      }
    }
    rmw_subscriptions_t subscriptions{subscribers.size(), subscribers.data()};
    rmw_events_t rmw_events{event_handles.size(), event_handles.data()};
    const rmw_time_t rmw_timeout = to_rmw_time(timeout);

    WaitResult result;
    const auto start = std::chrono::steady_clock::now();
    result.ret = rmw_wait(
      include_subscription ? &subscriptions : nullptr, nullptr, nullptr, nullptr,
      include_events ? &rmw_events : nullptr, wait_set, &rmw_timeout);
    result.elapsed = std::chrono::steady_clock::now() - start;
    result.ready = include_events ? event_handles : subscribers;
    return result;
  }

  WaitResult wait_for_events(std::chrono::nanoseconds timeout)
  {
    return wait(false, true, timeout);
  }

  bool subscription_has_data()
  {
    WaitResult result = wait(true, false, 0ns);
    return RMW_RET_OK == result.ret && nullptr != result.ready[0];
  }

  void wait_for_match(rmw_publisher_t * pub)
  {
    const auto deadline = std::chrono::steady_clock::now() + kEventTimeout;
    size_t sub_count = 0;
    size_t pub_count = 0;
    while (std::chrono::steady_clock::now() < deadline) {
      ASSERT_EQ(RMW_RET_OK, rmw_publisher_count_matched_subscriptions(pub, &sub_count));
      ASSERT_EQ(RMW_RET_OK, rmw_subscription_count_matched_publishers(sub, &pub_count));
      if (sub_count > 0 && pub_count > 0) {
        break;
      }
      std::this_thread::sleep_for(1ms);
    }
    ASSERT_GT(sub_count, 0u) << "publisher and subscription never matched";
    ASSERT_GT(pub_count, 0u) << "publisher and subscription never matched";
  }

  void publish_sample(rmw_publisher_t * pub)
  {
    test_msgs__msg__BasicTypes msg;
    ASSERT_TRUE(test_msgs__msg__BasicTypes__init(&msg));
    msg.int32_value = 929;
    rmw_ret_t ret = rmw_publish(pub, &msg, nullptr);
    test_msgs__msg__BasicTypes__fini(&msg);
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  }

  // Publishes one sample once both sides are matched and waits until it is pending,
  // without reading or taking it.
  void publish_unread_sample(rmw_publisher_t * pub)
  {
    ASSERT_NO_FATAL_FAILURE(wait_for_match(pub));
    ASSERT_NO_FATAL_FAILURE(publish_sample(pub));
    WaitResult result = wait(true, false, kEventTimeout);
    ASSERT_EQ(RMW_RET_OK, result.ret) << rmw_get_error_string().str;
    ASSERT_NE(nullptr, result.ready[0]) << "published sample never became pending";
  }

  void expect_sample_still_pending_and_take_it()
  {
    EXPECT_TRUE(subscription_has_data());
    test_msgs__msg__BasicTypes msg;
    ASSERT_TRUE(test_msgs__msg__BasicTypes__init(&msg));
    bool taken = false;
    rmw_ret_t ret = rmw_take(sub, &msg, &taken, nullptr);
    EXPECT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    EXPECT_TRUE(taken);
    EXPECT_EQ(929, msg.int32_value);
    test_msgs__msg__BasicTypes__fini(&msg);
  }

  // Takes a requested or offered QoS incompatibility caused by RELIABLE vs BEST_EFFORT.
  void take_qos_incompatible_event(
    rmw_event_t & event, rmw_qos_incompatible_event_status_t * status)
  {
    *status = {};
    bool taken = false;
    rmw_ret_t ret = rmw_take_event(&event, status, &taken);
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    EXPECT_TRUE(taken);
    EXPECT_GE(status->total_count, 1);
    EXPECT_EQ(RMW_QOS_POLICY_RELIABILITY, status->last_policy_kind);
  }

  rmw_subscription_t * create_extra_subscription(const char * topic)
  {
    rmw_subscription_options_t options = rmw_get_default_subscription_options();
    rmw_subscription_t * extra_sub =
      rmw_create_subscription(node, ts, topic, &reliable_qos(), &options);
    if (extra_sub) {
      extra_subscriptions.push_back(extra_sub);
    }
    return extra_sub;
  }

  void expect_no_ready_event()
  {
    WaitResult result = wait_for_events(kShortTimeout);
    EXPECT_EQ(RMW_RET_TIMEOUT, result.ret);
    for (size_t j = 0; j < result.ready.size(); ++j) {
      EXPECT_EQ(nullptr, result.ready[j]) << "event " << j << " reported ready";
    }
    EXPECT_GE(result.elapsed, kShortTimeout / 2) << "rmw_wait returned early";
  }

  void set_content_filter(const char * filter_expression)
  {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    rmw_subscription_content_filter_options_t options =
      rmw_get_zero_initialized_content_filter_options();
    rmw_ret_t ret = rmw_subscription_content_filter_options_init(
      filter_expression, 0, nullptr, &allocator, &options);
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ret = rmw_subscription_set_content_filter(sub, &options);
    EXPECT_EQ(RMW_RET_OK, rmw_subscription_content_filter_options_fini(&options, &allocator));
    ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
    ASSERT_EQ('\0' != *filter_expression, sub->is_cft_enabled);
  }

  size_t attached_condition_count()
  {
    auto fastdds_wait_set = static_cast<eprosima::fastdds::dds::WaitSet *>(wait_set->data);
    eprosima::fastdds::dds::ConditionSeq conditions;
    EXPECT_EQ(eprosima::fastdds::dds::RETCODE_OK, fastdds_wait_set->get_conditions(conditions));
    return conditions.size();
  }

  // Starts an event-only rmw_wait on another thread and returns once it has attached its
  // conditions, i.e. once it has decided what to wait on and is about to block.
  std::future<WaitResult> start_blocked_event_only_wait(std::chrono::nanoseconds timeout)
  {
    EXPECT_EQ(0u, attached_condition_count());
    std::future<WaitResult> waiter = std::async(
      std::launch::async, [this, timeout]() {return wait_for_events(timeout);});

    // rmw_wait attaches its conditions immediately before blocking in WaitSet::wait().
    bool attached = false;
    const auto deadline = std::chrono::steady_clock::now() + kEventTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
      if (attached_condition_count() > 0) {
        attached = true;
        break;
      }
      if (std::future_status::ready == waiter.wait_for(1ms)) {
        break;
      }
    }
    // Nothing has happened yet that should end the wait.
    EXPECT_TRUE(attached) << "rmw_wait returned without blocking";
    return waiter;
  }

  // Calls cause_event() while an event-only wait on events[0] is blocked, expects the wait to
  // be woken with that event ready, and takes the event into *status.
  void expect_qos_event_wakes_blocked_wait(
    const std::function<bool()> & cause_event, rmw_qos_incompatible_event_status_t * status)
  {
    std::future<WaitResult> waiter = start_blocked_event_only_wait(kEventTimeout);

    ASSERT_TRUE(cause_event()) << rmw_get_error_string().str;

    ASSERT_EQ(std::future_status::ready, waiter.wait_for(kEventTimeout + 5s));
    WaitResult result = waiter.get();
    ASSERT_EQ(RMW_RET_OK, result.ret) <<
      "the QoS event did not wake rmw_wait before its " <<
      std::chrono::duration_cast<std::chrono::seconds>(kEventTimeout).count() <<
      "s timeout";
    ASSERT_EQ(1u, result.ready.size());
    ASSERT_NE(nullptr, result.ready[0]) << "rmw_wait woke up without a ready event";
    EXPECT_LT(result.elapsed, kEventTimeout);
    ASSERT_NO_FATAL_FAILURE(take_qos_incompatible_event(events[0], status));
  }

  // Creates a BEST_EFFORT publisher, which is incompatible with the RELIABLE subscription,
  // while an event-only wait is blocked. The REQUESTED_QOS_INCOMPATIBLE event must wake it.
  void expect_incompatible_publisher_wakes_blocked_wait(
    rmw_qos_incompatible_event_status_t * status = nullptr)
  {
    rmw_qos_incompatible_event_status_t ignored;
    expect_qos_event_wakes_blocked_wait(
      [this]() {return nullptr != create_publisher(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT);},
      status ? status : &ignored);
  }

  rmw_context_t context{rmw_get_zero_initialized_context()};
  bool context_initialized{false};
  rmw_node_t * node{nullptr};
  rmw_wait_set_t * wait_set{nullptr};
  rmw_subscription_t * sub{nullptr};
  std::vector<rmw_publisher_t *> publishers;
  std::vector<rmw_subscription_t *> extra_subscriptions;
  std::vector<rmw_event_t> events;
  // Outlives the subscription, whose new message callback may use it until it is destroyed.
  std::atomic<size_t> new_message_count{0};

  const rosidl_message_type_support_t * ts{
    ROSIDL_GET_MSG_TYPE_SUPPORT(test_msgs, msg, BasicTypes)};
  static constexpr const char * topic_name = "/test_rmw_wait_events";
};

// The reported bug: an untaken sample must not wake a wait on the subscription's
// unrelated QoS events when the subscription itself is not in the wait set.
TEST_F(TestRmwWaitEvents, unread_sample_does_not_wake_event_only_wait) {
  ASSERT_NO_FATAL_FAILURE(init_events(unrelated_event_types));
  rmw_publisher_t * pub = create_publisher(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  ASSERT_NO_FATAL_FAILURE(publish_unread_sample(pub));
  ASSERT_TRUE(subscription_has_data());

  // Repeat to show the wait keeps blocking while the sample stays unread.
  for (int i = 0; i < 3; ++i) {
    WaitResult result = wait_for_events(kShortTimeout);
    EXPECT_EQ(RMW_RET_TIMEOUT, result.ret) << "iteration " << i;
    for (size_t j = 0; j < result.ready.size(); ++j) {
      EXPECT_EQ(nullptr, result.ready[j]) << "event " << j << " reported ready, iteration " << i;
    }
    EXPECT_GE(result.elapsed, kShortTimeout / 2) <<
      "rmw_wait returned early, iteration " << i;
  }

  ASSERT_NO_FATAL_FAILURE(expect_sample_still_pending_and_take_it());
}

// A sample arriving while an event-only wait is already blocked must not end it either.
TEST_F(TestRmwWaitEvents, sample_arriving_during_event_only_wait_does_not_wake_it) {
  ASSERT_NO_FATAL_FAILURE(init_events(unrelated_event_types));
  rmw_publisher_t * pub = create_publisher(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  ASSERT_NO_FATAL_FAILURE(wait_for_match(pub));
  eprosima::fastdds::dds::DataReader * reader = rmw_fastrtps_cpp::get_datareader(sub);
  ASSERT_NE(nullptr, reader);

  // Every attempt must time out without a ready event. An attempt only proves the point if
  // the sample also arrived before the wait ended, so on a slow machine where delivery
  // outlasts the wait, the sample is taken and the attempt repeated.
  constexpr std::chrono::nanoseconds timeout = 1s;
  constexpr int max_attempts = 3;
  bool arrived_while_waiting = false;
  for (int attempt = 0; attempt < max_attempts && !arrived_while_waiting; ++attempt) {
    ASSERT_FALSE(subscription_has_data()) << "attempt " << attempt;
    std::future<WaitResult> waiter = start_blocked_event_only_wait(timeout);
    ASSERT_NO_FATAL_FAILURE(publish_sample(pub));

    // get_unread_count(false) neither consumes the sample nor clears DATA_AVAILABLE.
    while (std::future_status::ready != waiter.wait_for(1ms)) {
      if (reader->get_unread_count(false) > 0) {
        arrived_while_waiting = std::future_status::ready != waiter.wait_for(0ms);
        break;
      }
    }

    ASSERT_EQ(std::future_status::ready, waiter.wait_for(timeout + 5s));
    WaitResult result = waiter.get();
    EXPECT_EQ(RMW_RET_TIMEOUT, result.ret) << "attempt " << attempt;
    for (size_t j = 0; j < result.ready.size(); ++j) {
      EXPECT_EQ(nullptr, result.ready[j]) << "event " << j << " reported ready, attempt " <<
        attempt;
    }
    EXPECT_GE(result.elapsed, timeout / 2) << "rmw_wait returned early, attempt " << attempt;
    if (::testing::Test::HasFailure()) {
      break;
    }

    // Wait for a late sample, so that the next attempt starts with no data pending.
    WaitResult pending = wait(true, false, kEventTimeout);
    ASSERT_EQ(RMW_RET_OK, pending.ret) << "the sample was never delivered";
    ASSERT_NO_FATAL_FAILURE(expect_sample_still_pending_and_take_it());
  }
  EXPECT_TRUE(arrived_while_waiting) <<
    "the sample never arrived while an event-only wait was blocked";
}

// Control: a QoS event raised while an event-only wait is blocked wakes it.
TEST_F(TestRmwWaitEvents, qos_event_wakes_blocked_event_only_wait) {
  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  ASSERT_FALSE(subscription_has_data());

  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_blocked_wait());
}

// Same as above while the subscription has an untaken sample, so the shared
// StatusCondition is already triggered by DATA_AVAILABLE when the wait starts.
TEST_F(TestRmwWaitEvents, qos_event_wakes_blocked_event_only_wait_with_unread_sample) {
  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  rmw_publisher_t * pub = create_publisher(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  ASSERT_NO_FATAL_FAILURE(publish_unread_sample(pub));
  ASSERT_TRUE(subscription_has_data());

  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_blocked_wait());

  ASSERT_NO_FATAL_FAILURE(expect_sample_still_pending_and_take_it());
}

// A QoS status raised before the event exists and before the wait starts is reported
// immediately, with an untaken sample pending.
TEST_F(TestRmwWaitEvents, qos_event_raised_before_wait_is_reported_with_unread_sample) {
  rmw_publisher_t * pub = create_publisher(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  ASSERT_NO_FATAL_FAILURE(publish_unread_sample(pub));
  ASSERT_NE(nullptr, create_publisher(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT)) <<
    rmw_get_error_string().str;

  // get_status_changes() only reads the status; get_*_status() would clear it.
  eprosima::fastdds::dds::DataReader * reader = rmw_fastrtps_cpp::get_datareader(sub);
  ASSERT_NE(nullptr, reader);
  const auto incompatible_qos = eprosima::fastdds::dds::StatusMask::requested_incompatible_qos();
  const auto deadline = std::chrono::steady_clock::now() + kEventTimeout;
  while (!reader->get_status_changes().is_active(incompatible_qos) &&
    std::chrono::steady_clock::now() < deadline)
  {
    std::this_thread::sleep_for(1ms);
  }
  ASSERT_TRUE(reader->get_status_changes().is_active(incompatible_qos));

  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  ASSERT_TRUE(subscription_has_data());

  WaitResult result = wait_for_events(kEventTimeout);
  ASSERT_EQ(RMW_RET_OK, result.ret);
  ASSERT_NE(nullptr, result.ready[0]) << "pending QoS event was not reported";
  EXPECT_LT(result.elapsed, kEventTimeout);
  rmw_qos_incompatible_event_status_t status;
  ASSERT_NO_FATAL_FAILURE(take_qos_incompatible_event(events[0], &status));

  ASSERT_NO_FATAL_FAILURE(expect_sample_still_pending_and_take_it());
}

// Removing an event callback must keep the event waitable: its guard is raised by the same
// listener callback.
TEST_F(TestRmwWaitEvents, qos_event_wakes_blocked_wait_after_event_callback_removed) {
  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  std::atomic<size_t> callback_count{0};
  auto callback = [](const void * user_data, size_t number_of_events) {
      auto count = static_cast<std::atomic<size_t> *>(const_cast<void *>(user_data));
      *count += number_of_events;
    };
  rmw_ret_t ret = rmw_event_set_callback(&events[0], callback, &callback_count);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  ret = rmw_event_set_callback(&events[0], nullptr, nullptr);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_blocked_wait());
  EXPECT_EQ(0u, callback_count.load());
}

// Setting a content filter re-creates the DataReader, which must keep reporting the
// subscription's events.
TEST_F(TestRmwWaitEvents, qos_event_wakes_blocked_wait_after_content_filter_set) {
  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  ASSERT_NO_FATAL_FAILURE(set_content_filter("int32_value = 929"));

  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_blocked_wait());
}

// The new DataReader created by a content filter change matches an existing incompatible
// publisher right away. That event must be reported too.
TEST_F(TestRmwWaitEvents, qos_event_raised_by_content_filter_reader_creation) {
  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_blocked_wait());

  ASSERT_NO_FATAL_FAILURE(set_content_filter("int32_value = 929"));

  WaitResult result = wait_for_events(kEventTimeout);
  ASSERT_EQ(RMW_RET_OK, result.ret) << "the new DataReader's QoS event was not reported";
  ASSERT_NE(nullptr, result.ready[0]);
  rmw_qos_incompatible_event_status_t status;
  ASSERT_NO_FATAL_FAILURE(take_qos_incompatible_event(events[0], &status));
}

// A new message callback registered before content filter changes keeps being called.
// Setting and then resetting the filter re-creates the DataReader twice; the reset leaves
// an unfiltered reader, so the test does not depend on filter evaluation.
TEST_F(TestRmwWaitEvents, new_message_callback_survives_content_filter_changes) {
  auto callback = [](const void * user_data, size_t number_of_events) {
      auto count = static_cast<std::atomic<size_t> *>(const_cast<void *>(user_data));
      *count += number_of_events;
    };
  rmw_ret_t ret = rmw_subscription_set_on_new_message_callback(sub, callback, &new_message_count);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;
  ASSERT_NO_FATAL_FAILURE(set_content_filter("int32_value = 929"));
  ASSERT_NO_FATAL_FAILURE(set_content_filter(""));

  rmw_publisher_t * pub = create_publisher(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  ASSERT_NO_FATAL_FAILURE(publish_unread_sample(pub));

  // The callback runs on a middleware thread, possibly just after the sample is pending.
  const auto deadline = std::chrono::steady_clock::now() + kEventTimeout;
  while (0u == new_message_count.load() && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(1ms);
  }
  EXPECT_EQ(1u, new_message_count.load());
  ASSERT_NO_FATAL_FAILURE(expect_sample_still_pending_and_take_it());
}

// A delivered and taken event must not be reported again, and a later event must wake the
// wait again.
TEST_F(TestRmwWaitEvents, subscription_qos_event_rearms_after_take) {
  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));

  rmw_qos_incompatible_event_status_t status;
  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_blocked_wait(&status));
  EXPECT_EQ(1, status.total_count);
  EXPECT_EQ(1, status.total_count_change);
  ASSERT_NO_FATAL_FAILURE(expect_no_ready_event());

  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_blocked_wait(&status));
  EXPECT_EQ(2, status.total_count);
  EXPECT_EQ(1, status.total_count_change);
  ASSERT_NO_FATAL_FAILURE(expect_no_ready_event());
}

// Initializing another event of an already delivered and taken type must not report the
// old status again.
TEST_F(TestRmwWaitEvents, event_init_after_take_does_not_report_stale_event) {
  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  ASSERT_NO_FATAL_FAILURE(expect_incompatible_publisher_wakes_blocked_wait());

  ASSERT_NO_FATAL_FAILURE(init_events({RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE}));
  ASSERT_NO_FATAL_FAILURE(expect_no_ready_event());
}

// Publisher events: a reader that requests RELIABLE raises OFFERED_QOS_INCOMPATIBLE on a
// BEST_EFFORT writer, which must wake a blocked wait on the writer's event, once per reader.
TEST_F(TestRmwWaitEvents, publisher_qos_event_wakes_blocked_wait_and_rearms_after_take) {
  constexpr char other_topic[] = "/test_rmw_wait_events_publisher";
  rmw_qos_profile_t best_effort_qos = reliable_qos();
  best_effort_qos.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
  rmw_publisher_options_t pub_options = rmw_get_default_publisher_options();
  rmw_publisher_t * pub =
    rmw_create_publisher(node, ts, other_topic, &best_effort_qos, &pub_options);
  ASSERT_NE(nullptr, pub) << rmw_get_error_string().str;
  publishers.push_back(pub);
  events.push_back(rmw_get_zero_initialized_event());
  rmw_ret_t ret = rmw_publisher_event_init(&events.back(), pub, RMW_EVENT_OFFERED_QOS_INCOMPATIBLE);
  ASSERT_EQ(RMW_RET_OK, ret) << rmw_get_error_string().str;

  auto create_subscription = [this, other_topic]() {
      return nullptr != create_extra_subscription(other_topic);
    };
  rmw_qos_incompatible_event_status_t status;
  ASSERT_NO_FATAL_FAILURE(expect_qos_event_wakes_blocked_wait(create_subscription, &status));
  EXPECT_EQ(1, status.total_count);
  EXPECT_EQ(1, status.total_count_change);
  ASSERT_NO_FATAL_FAILURE(expect_no_ready_event());

  ASSERT_NO_FATAL_FAILURE(expect_qos_event_wakes_blocked_wait(create_subscription, &status));
  EXPECT_EQ(2, status.total_count);
  EXPECT_EQ(1, status.total_count_change);
  ASSERT_NO_FATAL_FAILURE(expect_no_ready_event());
}
