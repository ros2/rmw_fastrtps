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


#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "rmw/rmw.h"
#include "rmw/types.h"

#include "buffer_endpoint_registry.hpp"

namespace
{

rmw_gid_t make_gid(uint8_t id)
{
  rmw_gid_t gid{};
  // rmw_compare_gids_equal() accepts only gids that carry the library's own identifier.
  gid.implementation_identifier = rmw_get_implementation_identifier();
  std::memset(gid.data, 0, sizeof(gid.data));
  gid.data[0] = id;
  return gid;
}

rmw_fastrtps_cpp::BufferEndpointInfo make_subscriber(uint8_t id, const std::string & topic)
{
  rmw_fastrtps_cpp::BufferEndpointInfo info;
  info.gid = make_gid(id);
  info.topic_name = topic;
  info.backend_metadata["cpu"] = "";
  return info;
}

// Records what a buffer-aware publisher learns from the registry.
struct PublisherView
{
  std::vector<uint8_t> discovered;
  std::vector<uint8_t> removed;
};

void register_publisher(
  rmw_fastrtps_cpp::BufferEndpointRegistry & registry, const std::string & topic,
  uint8_t publisher_id, PublisherView & view)
{
  registry.register_subscriber_discovery_callback(
    topic, make_gid(publisher_id),
    [&view](const rmw_fastrtps_cpp::BufferEndpointInfo & info) {
      view.discovered.push_back(info.gid.data[0]);
    },
    [&view](const rmw_gid_t & gid) {
      view.removed.push_back(gid.data[0]);
    });
}

}  // namespace

// A publisher is told when a subscriber it was told about is gone.
TEST(test_buffer_endpoint_registry, removal_reaches_the_publisher)
{
  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  PublisherView view;
  register_publisher(registry, "/chatter", 100, view);

  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));
  registry.notify_subscriber_removed(make_gid(1));

  EXPECT_EQ(std::vector<uint8_t>({1}), view.discovered);
  EXPECT_EQ(std::vector<uint8_t>({1}), view.removed);
}

// A subscriber that was never reported as buffer-aware, or was already removed, is ignored.
TEST(test_buffer_endpoint_registry, removal_of_an_unknown_subscriber_is_ignored)
{
  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  PublisherView view;
  register_publisher(registry, "/chatter", 100, view);

  registry.notify_subscriber_removed(make_gid(7));
  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));
  registry.notify_subscriber_removed(make_gid(1));
  registry.notify_subscriber_removed(make_gid(1));

  EXPECT_EQ(std::vector<uint8_t>({1}), view.removed);
}

// Only publishers on the subscriber's topic are told.
TEST(test_buffer_endpoint_registry, removal_reaches_only_publishers_on_the_same_topic)
{
  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  PublisherView same_topic;
  PublisherView other_topic;
  register_publisher(registry, "/chatter", 100, same_topic);
  register_publisher(registry, "/other", 101, other_topic);

  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));
  registry.notify_subscriber_removed(make_gid(1));

  EXPECT_EQ(std::vector<uint8_t>({1}), same_topic.removed);
  EXPECT_TRUE(other_topic.removed.empty());
}

// A publisher created after the subscriber left must not learn of it.
TEST(test_buffer_endpoint_registry, a_later_publisher_does_not_learn_of_a_removed_subscriber)
{
  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));
  registry.notify_subscriber_discovered(make_subscriber(2, "/chatter"));
  registry.notify_subscriber_removed(make_gid(1));

  PublisherView late;
  register_publisher(registry, "/chatter", 100, late);

  EXPECT_EQ(std::vector<uint8_t>({2}), late.discovered);
}

// A subscriber that comes back with the same GID is reported again.
TEST(test_buffer_endpoint_registry, a_returning_subscriber_is_reported_again)
{
  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  PublisherView view;
  register_publisher(registry, "/chatter", 100, view);

  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));
  registry.notify_subscriber_removed(make_gid(1));
  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));

  EXPECT_EQ(std::vector<uint8_t>({1, 1}), view.discovered);
}

// A publisher that was unregistered is no longer told.
TEST(test_buffer_endpoint_registry, an_unregistered_publisher_is_not_told)
{
  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  PublisherView view;
  register_publisher(registry, "/chatter", 100, view);
  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));

  registry.unregister_callbacks(make_gid(100));
  registry.notify_subscriber_removed(make_gid(1));

  EXPECT_TRUE(view.removed.empty());
}

// A publisher registered without a removal callback keeps working.
TEST(test_buffer_endpoint_registry, a_publisher_without_a_removal_callback_is_skipped)
{
  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  std::vector<uint8_t> discovered;
  registry.register_subscriber_discovery_callback(
    "/chatter", make_gid(100),
    [&discovered](const rmw_fastrtps_cpp::BufferEndpointInfo & info) {
      discovered.push_back(info.gid.data[0]);
    });

  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));
  registry.notify_subscriber_removed(make_gid(1));

  EXPECT_EQ(std::vector<uint8_t>({1}), discovered);
}

// A subscriber that leaves while a new publisher is still being told about it must not
// stay with that publisher: the publisher's last event for it is the removal.
TEST(test_buffer_endpoint_registry, a_subscriber_that_leaves_during_registration_is_removed)
{
  rmw_fastrtps_cpp::BufferEndpointRegistry registry;
  registry.notify_subscriber_discovered(make_subscriber(1, "/chatter"));

  std::vector<std::string> events;
  registry.register_subscriber_discovery_callback(
    "/chatter", make_gid(100),
    [&registry, &events](const rmw_fastrtps_cpp::BufferEndpointInfo & info) {
      // The subscriber leaves after the registry chose to report it, before this callback ends.
      registry.notify_subscriber_removed(info.gid);
      events.push_back("discovered " + std::to_string(info.gid.data[0]));
    },
    [&events](const rmw_gid_t & gid) {
      events.push_back("removed " + std::to_string(gid.data[0]));
    });

  ASSERT_FALSE(events.empty());
  EXPECT_EQ("removed 1", events.back());
}
