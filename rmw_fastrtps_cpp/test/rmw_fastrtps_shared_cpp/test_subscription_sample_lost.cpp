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

#include "gtest/gtest.h"

#include "rmw/events_statuses/message_lost.h"
#include "rmw/event.h"

#include "rmw_fastrtps_shared_cpp/custom_subscriber_info.hpp"

// Buffer-aware samples are received on a buffer channel DataReader, so the samples it loses
// must be reported together with the ones the main DataReader loses.
// Every update below marks the status as changed, so take_event does not query a DataReader.
TEST(TestSubscriptionSampleLost, main_and_buffer_channel_counts_are_added) {
  RMWSubscriptionEvent event(nullptr);

  event.update_sample_lost(2, 2);
  event.update_buffer_channel_sample_lost(3, 3);

  rmw_message_lost_status_t status{};
  ASSERT_TRUE(event.take_event(RMW_EVENT_MESSAGE_LOST, &status));
  EXPECT_EQ(5u, status.total_count);
  EXPECT_EQ(5u, status.total_count_change);

  event.update_buffer_channel_sample_lost(4, 1);

  ASSERT_TRUE(event.take_event(RMW_EVENT_MESSAGE_LOST, &status));
  EXPECT_EQ(6u, status.total_count);
  EXPECT_EQ(1u, status.total_count_change);

  event.update_sample_lost(3, 1);

  ASSERT_TRUE(event.take_event(RMW_EVENT_MESSAGE_LOST, &status));
  EXPECT_EQ(7u, status.total_count);
  EXPECT_EQ(1u, status.total_count_change);
}
