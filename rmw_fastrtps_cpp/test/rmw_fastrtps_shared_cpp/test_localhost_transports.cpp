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

#include <cstdint>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/domain/qos/DomainParticipantQos.hpp"
#include "fastdds/rtps/transport/UDPv4TransportDescriptor.hpp"
#include "fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp"

#include "rmw/discovery_options.h"
#include "rmw/ret_types.h"
#include "rmw/security_options.h"

#include "rmw_dds_common/context.hpp"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/participant.hpp"

using eprosima::fastdds::dds::DomainParticipantFactory;
using eprosima::fastdds::dds::DomainParticipantQos;
using eprosima::fastdds::rtps::SharedMemTransportDescriptor;
using eprosima::fastdds::rtps::UDPv4TransportDescriptor;

namespace
{
// Smallest maxInitialPeersRange used with the LOCALHOST discovery range.
constexpr uint32_t localhost_min_initial_peers_range = 32;

template<typename DescriptorT>
std::vector<std::shared_ptr<DescriptorT>>
transports_of_type(const DomainParticipantQos & qos)
{
  std::vector<std::shared_ptr<DescriptorT>> transports;
  for (const auto & transport : qos.transport().user_transports) {
    auto casted = std::dynamic_pointer_cast<DescriptorT>(transport);
    if (casted) {
      transports.push_back(casted);
    }
  }
  return transports;
}
}  // namespace

/// Creates participants on top of a default participant QoS, which stands for the QoS
/// loaded from the default XML profile.
class LocalhostTransportsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    factory_ = DomainParticipantFactory::get_shared_instance();
    // Load the default profiles before setting the default QoS, as create_participant() does.
    factory_->load_profiles();
    original_qos_ = factory_->get_default_participant_qos();
  }

  void TearDown() override
  {
    if (participant_info_) {
      EXPECT_EQ(RMW_RET_OK, rmw_fastrtps_shared_cpp::destroy_participant(participant_info_));
    }
    factory_->set_default_participant_qos(original_qos_);
  }

  /// Set the transports as a default XML profile with userTransports does.
  void set_profile_transports(
    std::vector<std::shared_ptr<eprosima::fastdds::rtps::TransportDescriptorInterface>> transports)
  {
    DomainParticipantQos qos = original_qos_;
    qos.transport().user_transports = std::move(transports);
    ASSERT_EQ(
      eprosima::fastdds::dds::RETCODE_OK, factory_->set_default_participant_qos(qos));
  }

  DomainParticipantQos create_participant(rmw_automatic_discovery_range_t discovery_range)
  {
    rmw_security_options_t security_options = rmw_get_zero_initialized_security_options();
    rmw_discovery_options_t discovery_options = rmw_get_zero_initialized_discovery_options();
    discovery_options.automatic_discovery_range = discovery_range;

    participant_info_ = rmw_fastrtps_shared_cpp::create_participant(
      "rmw_fastrtps_cpp", 0u, &security_options, &discovery_options, "/", &common_context_);
    if (!participant_info_) {
      ADD_FAILURE() << "failed to create participant";
      return DomainParticipantQos();
    }
    return participant_info_->participant_->get_qos();
  }

  std::shared_ptr<DomainParticipantFactory> factory_;
  DomainParticipantQos original_qos_;
  rmw_dds_common::Context common_context_;
  CustomParticipantInfo * participant_info_{nullptr};
};

TEST_F(LocalhostTransportsTest, adds_transports_when_profile_has_none)
{
  set_profile_transports({});

  const DomainParticipantQos qos = create_participant(RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST);

  EXPECT_FALSE(qos.transport().use_builtin_transports);
  EXPECT_EQ(2u, qos.transport().user_transports.size());
  EXPECT_EQ(1u, transports_of_type<SharedMemTransportDescriptor>(qos).size());
  const auto udp_transports = transports_of_type<UDPv4TransportDescriptor>(qos);
  ASSERT_EQ(1u, udp_transports.size());
  EXPECT_EQ(localhost_min_initial_peers_range, udp_transports[0]->maxInitialPeersRange);
}

// Regression test for the UDPv4 transport of the profile being ignored, so that a second one
// was added and maxInitialPeersRange could only be raised by duplicating the transport.
TEST_F(LocalhostTransportsTest, reuses_profile_udp_transport_with_larger_range)
{
  auto profile_udp = std::make_shared<UDPv4TransportDescriptor>();
  profile_udp->maxInitialPeersRange = 64;
  profile_udp->sendBufferSize = 123456;
  set_profile_transports({profile_udp});

  const DomainParticipantQos qos = create_participant(RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST);

  EXPECT_FALSE(qos.transport().use_builtin_transports);
  EXPECT_EQ(2u, qos.transport().user_transports.size());
  EXPECT_EQ(1u, transports_of_type<SharedMemTransportDescriptor>(qos).size());
  const auto udp_transports = transports_of_type<UDPv4TransportDescriptor>(qos);
  ASSERT_EQ(1u, udp_transports.size());
  EXPECT_EQ(64u, udp_transports[0]->maxInitialPeersRange);
  EXPECT_EQ(123456u, udp_transports[0]->sendBufferSize);
}

TEST_F(LocalhostTransportsTest, raises_profile_udp_transport_with_smaller_range)
{
  auto profile_udp = std::make_shared<UDPv4TransportDescriptor>();
  const uint32_t profile_range = profile_udp->maxInitialPeersRange;
  ASSERT_LT(profile_range, localhost_min_initial_peers_range);
  profile_udp->sendBufferSize = 123456;
  set_profile_transports({profile_udp});

  const DomainParticipantQos qos = create_participant(RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST);

  const auto udp_transports = transports_of_type<UDPv4TransportDescriptor>(qos);
  ASSERT_EQ(1u, udp_transports.size());
  EXPECT_EQ(localhost_min_initial_peers_range, udp_transports[0]->maxInitialPeersRange);
  EXPECT_EQ(123456u, udp_transports[0]->sendBufferSize);
  // The descriptor of the profile is shared with other participants, it must not change.
  EXPECT_EQ(profile_range, profile_udp->maxInitialPeersRange);
}

TEST_F(LocalhostTransportsTest, reuses_profile_shm_transport)
{
  auto profile_shm = std::make_shared<SharedMemTransportDescriptor>();
  profile_shm->segment_size(2 * 1024 * 1024);
  set_profile_transports({profile_shm});

  const DomainParticipantQos qos = create_participant(RMW_AUTOMATIC_DISCOVERY_RANGE_LOCALHOST);

  EXPECT_FALSE(qos.transport().use_builtin_transports);
  EXPECT_EQ(2u, qos.transport().user_transports.size());
  const auto shm_transports = transports_of_type<SharedMemTransportDescriptor>(qos);
  ASSERT_EQ(1u, shm_transports.size());
  EXPECT_EQ(2u * 1024u * 1024u, shm_transports[0]->segment_size());
  const auto udp_transports = transports_of_type<UDPv4TransportDescriptor>(qos);
  ASSERT_EQ(1u, udp_transports.size());
  EXPECT_EQ(localhost_min_initial_peers_range, udp_transports[0]->maxInitialPeersRange);
}

TEST_F(LocalhostTransportsTest, keeps_profile_transports_with_other_discovery_ranges)
{
  auto profile_udp = std::make_shared<UDPv4TransportDescriptor>();
  const uint32_t profile_range = profile_udp->maxInitialPeersRange;
  set_profile_transports({profile_udp});

  const DomainParticipantQos qos = create_participant(RMW_AUTOMATIC_DISCOVERY_RANGE_SUBNET);

  EXPECT_TRUE(qos.transport().use_builtin_transports);
  EXPECT_EQ(1u, qos.transport().user_transports.size());
  const auto udp_transports = transports_of_type<UDPv4TransportDescriptor>(qos);
  ASSERT_EQ(1u, udp_transports.size());
  EXPECT_EQ(profile_range, udp_transports[0]->maxInitialPeersRange);
}
