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

#include <limits.h>
#include <string>

#include "fastrtps/config.h"
#include "fastrtps/Domain.h"
#include "fastrtps/attributes/ParticipantAttributes.h"
#include "fastrtps/attributes/PublisherAttributes.h"
#include "fastrtps/attributes/SubscriberAttributes.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/publisher/PublisherListener.h"
#include "fastrtps/rtps/RTPSDomain.h"
#include "fastrtps/rtps/common/Locator.h"
#include "fastrtps/rtps/builtin/discovery/endpoint/EDPSimple.h"
#include "fastrtps/rtps/reader/ReaderListener.h"
#include "fastrtps/rtps/reader/RTPSReader.h"
#include "fastrtps/rtps/reader/StatefulReader.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/subscriber/SampleInfo.h"

#include "rcutils/filesystem.h"
#include "rcutils/get_env.h"

#include "rmw/allocators.h"

#include "rmw_fastrtps_shared_cpp/custom_participant_info.hpp"
#include "rmw_fastrtps_shared_cpp/participant.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_common.hpp"
#include "rmw_fastrtps_shared_cpp/rmw_security.hpp"

using Domain = eprosima::fastrtps::Domain;
using IPLocator = eprosima::fastrtps::rtps::IPLocator;
using Locator_t = eprosima::fastrtps::rtps::Locator_t;
using Participant = eprosima::fastrtps::Participant;
using ParticipantAttributes = eprosima::fastrtps::ParticipantAttributes;
using StatefulReader = eprosima::fastrtps::rtps::StatefulReader;

static
CustomParticipantInfo *
__create_participant(
  const char * identifier,
  ParticipantAttributes participantAttrs,
  bool leave_middleware_default_qos,
  rmw_dds_common::Context * common_context)
{
  // Declare everything before beginning to create things.
  ::ParticipantListener * listener = nullptr;
  Participant * participant = nullptr;
  CustomParticipantInfo * participant_info = nullptr;

  try {
    listener = new ::ParticipantListener(
      identifier, common_context);
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG("failed to allocate participant listener");
    goto fail;
  }

  participant = Domain::createParticipant(participantAttrs, listener);
  if (!participant) {
    RMW_SET_ERROR_MSG("create_node() could not create participant");
    return nullptr;
  }

  try {
    participant_info = new CustomParticipantInfo();
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG("failed to allocate node impl struct");
    goto fail;
  }
  participant_info->leave_middleware_default_qos = leave_middleware_default_qos;

  participant_info->participant = participant;
  participant_info->listener = listener;

  return participant_info;
fail:
  rmw_free(listener);
  if (participant) {
    Domain::removeParticipant(participant);
  }
  return nullptr;
}

CustomParticipantInfo *
rmw_fastrtps_shared_cpp::create_participant(
  const char * identifier,
  size_t domain_id,
  const rmw_security_options_t * security_options,
  bool localhost_only,
  const char * security_context,
  rmw_dds_common::Context * common_context)
{
  if (!security_options) {
    RMW_SET_ERROR_MSG("security_options is null");
    return nullptr;
  }
  ParticipantAttributes participantAttrs;

  // Load default XML profile.
  Domain::getDefaultParticipantAttributes(participantAttrs);

  participantAttrs.rtps.builtin.domainId = static_cast<uint32_t>(domain_id);

  if (localhost_only) {
    Locator_t local_network_interface_locator;
    static const std::string local_ip_name("127.0.0.1");
    local_network_interface_locator.kind = 1;
    local_network_interface_locator.port = 0;
    IPLocator::setIPv4(local_network_interface_locator, local_ip_name);
    participantAttrs.rtps.builtin.metatrafficUnicastLocatorList.push_back(
      local_network_interface_locator);
    participantAttrs.rtps.builtin.initialPeersList.push_back(local_network_interface_locator);
  }

  participantAttrs.rtps.builtin.domainId = static_cast<uint32_t>(domain_id);

  size_t length = snprintf(nullptr, 0, "securitycontext=%s;", security_context) + 1;
  participantAttrs.rtps.userData.resize(length);
  int written = snprintf(
    reinterpret_cast<char *>(participantAttrs.rtps.userData.data()),
    length, "securitycontext=%s;", security_context);
  if (written < 0 || written > static_cast<int>(length) - 1) {
    RMW_SET_ERROR_MSG("failed to populate user_data buffer");
    return nullptr;
  }
  participantAttrs.rtps.setName(security_context);

  bool leave_middleware_default_qos = false;
  const char * env_value;
  const char * error_str;
  error_str = rcutils_get_env("RMW_FASTRTPS_USE_QOS_FROM_XML", &env_value);
  if (error_str != NULL) {
    RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "Error getting env var: %s\n", error_str);
    return nullptr;
  }
  if (env_value != nullptr) {
    leave_middleware_default_qos = strcmp(env_value, "1") == 0;
  }
  // allow reallocation to support discovery messages bigger than 5000 bytes
  if (!leave_middleware_default_qos) {
    participantAttrs.rtps.builtin.readerHistoryMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
    participantAttrs.rtps.builtin.writerHistoryMemoryPolicy =
      eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;
  }

  eprosima::fastrtps::rtps::PropertyPolicy property_policy;
  if (apply_security_options(*security_options, property_policy)) {
    participantAttrs.rtps.properties = property_policy;
  } else {
    return nullptr;
  }

  return __create_participant(
    identifier,
    participantAttrs,
    leave_middleware_default_qos,
    common_context);
}

rmw_ret_t
rmw_fastrtps_shared_cpp::destroy_participant(CustomParticipantInfo * participant_info)
{
  rmw_ret_t result_ret = RMW_RET_OK;
  if (!participant_info) {
    RMW_SET_ERROR_MSG("participant_info is null");
    return RMW_RET_ERROR;
  }
  Domain::removeParticipant(participant_info->participant);
  delete participant_info->listener;
  participant_info->listener = nullptr;
  delete participant_info;
  return result_ret;
}
