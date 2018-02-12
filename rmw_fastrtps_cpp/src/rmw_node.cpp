// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#include <array>
#include <utility>
#include <set>
#include <string>

#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"
#include "rcutils/get_env.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

#include "fastrtps/config.h"
#include "fastrtps/Domain.h"
#include "fastrtps/participant/Participant.h"
#include "fastrtps/attributes/ParticipantAttributes.h"
#include "fastrtps/publisher/Publisher.h"
#include "fastrtps/attributes/PublisherAttributes.h"
#include "fastrtps/publisher/PublisherListener.h"
#include "fastrtps/subscriber/Subscriber.h"
#include "fastrtps/subscriber/SubscriberListener.h"
#include "fastrtps/subscriber/SampleInfo.h"
#include "fastrtps/attributes/SubscriberAttributes.h"

#include "fastrtps/rtps/RTPSDomain.h"

#include "fastrtps/rtps/reader/RTPSReader.h"
#include "fastrtps/rtps/reader/StatefulReader.h"
#include "fastrtps/rtps/reader/ReaderListener.h"
#include "fastrtps/rtps/builtin/discovery/endpoint/EDPSimple.h"

#include "rmw_fastrtps_cpp/identifier.hpp"
#include "rmw_fastrtps_cpp/custom_participant_info.hpp"

#define ROS_SECURITY_ROOT_DIRECTORY_VAR_NAME "ROS_SECURITY_ROOT_DIRECTORY"

using Domain = eprosima::fastrtps::Domain;
using Participant = eprosima::fastrtps::Participant;
using ParticipantAttributes = eprosima::fastrtps::ParticipantAttributes;
using StatefulReader = eprosima::fastrtps::rtps::StatefulReader;

extern "C"
{
rmw_node_t *
create_node(
  const char * name,
  const char * namespace_,
  ParticipantAttributes participantAttrs)
{
  if (!name) {
    RMW_SET_ERROR_MSG("name is null");
    return nullptr;
  }

  if (!namespace_) {
    RMW_SET_ERROR_MSG("namespace_ is null");
    return nullptr;
  }

  // Declare everything before beginning to create things.
  ::ParticipantListener * listener = nullptr;
  Participant * participant = nullptr;
  rmw_guard_condition_t * graph_guard_condition = nullptr;
  CustomParticipantInfo * node_impl = nullptr;
  rmw_node_t * node_handle = nullptr;
  ReaderInfo * tnat_1 = nullptr;
  WriterInfo * tnat_2 = nullptr;
  std::pair<StatefulReader *, StatefulReader *> edp_readers;

  try {
    listener = new ::ParticipantListener();
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG("failed to allocate participant listener");
    goto fail;
  }

  participant = Domain::createParticipant(participantAttrs, listener);
  if (!participant) {
    RMW_SET_ERROR_MSG("create_node() could not create participant");
    return nullptr;
  }

  graph_guard_condition = rmw_create_guard_condition();
  if (!graph_guard_condition) {
    // error already set
    goto fail;
  }

  try {
    node_impl = new CustomParticipantInfo();
  } catch (std::bad_alloc &) {
    RMW_SET_ERROR_MSG("failed to allocate node impl struct");
    goto fail;
  }

  node_handle = rmw_node_allocate();
  if (!node_handle) {
    RMW_SET_ERROR_MSG("failed to allocate rmw_node_t");
    goto fail;
  }
  node_handle->implementation_identifier = eprosima_fastrtps_identifier;
  node_impl->participant = participant;
  node_impl->listener = listener;
  node_impl->graph_guard_condition = graph_guard_condition;
  node_handle->data = node_impl;

  node_handle->name =
    static_cast<const char *>(rmw_allocate(sizeof(char) * strlen(name) + 1));
  if (!node_handle->name) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    node_handle->namespace_ = nullptr;  // to avoid free on uninitialized memory
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->name), name, strlen(name) + 1);

  node_handle->namespace_ =
    static_cast<const char *>(rmw_allocate(sizeof(char) * strlen(namespace_) + 1));
  if (!node_handle->namespace_) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->namespace_), namespace_, strlen(namespace_) + 1);

  tnat_1 = new ReaderInfo(participant, graph_guard_condition);
  tnat_2 = new WriterInfo(participant, graph_guard_condition);

  node_impl->secondarySubListener = tnat_1;
  node_impl->secondaryPubListener = tnat_2;

  edp_readers = participant->getEDPReaders();
  if (!edp_readers.first) {
    RMW_SET_ERROR_MSG("edp_readers.first is null");
    goto fail;
  }

  if (!edp_readers.second) {
    RMW_SET_ERROR_MSG("edp_readers.second is null");
    goto fail;
  }

  if (!(edp_readers.first->setListener(tnat_1) & edp_readers.second->setListener(tnat_2))) {
    RMW_SET_ERROR_MSG("Failed to attach ROS related logic to the Participant");
    goto fail;
  }

  return node_handle;
fail:
  delete tnat_2;
  delete tnat_1;
  if (node_handle) {
    rmw_free(const_cast<char *>(node_handle->namespace_));
    node_handle->namespace_ = nullptr;
    rmw_free(const_cast<char *>(node_handle->name));
    node_handle->name = nullptr;
  }
  rmw_node_free(node_handle);
  delete node_impl;
  if (graph_guard_condition) {
    rmw_ret_t ret = rmw_destroy_guard_condition(graph_guard_condition);
    if (ret != RMW_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_fastrtps_cpp",
        "failed to destroy guard condition during error handling")
    }
  }
  rmw_free(listener);
  if (participant) {
    Domain::removeParticipant(participant);
  }
  return nullptr;
}

bool
get_security_file_paths(
  std::array<std::string, 6> & security_files_paths, const char * node_secure_root)
{
  // here assume only 6 files for security
  const char * file_names[6] = {
    "ca.cert.pem", "cert.pem", "key.pem",
    "ca.cert.pem", "governance.p7s", "permissions.p7s"
  };
  size_t num_files = sizeof(file_names) / sizeof(char *);

  std::string file_prefix("file://");

  for (size_t i = 0; i < num_files; i++) {
    char * file_path;

    if (i != 3) {
      file_path = rcutils_join_path(node_secure_root, file_names[i]);
    } else {
      const char * ros_secure_root_env = NULL;
      if (rcutils_get_env(ROS_SECURITY_ROOT_DIRECTORY_VAR_NAME, &ros_secure_root_env) == NULL) {
        file_path = rcutils_join_path(ros_secure_root_env, file_names[i]);
      } else {
        return false;
      }
    }

    if (!file_path) {
      return false;
    }

    if (rcutils_is_readable(file_path)) {
      security_files_paths[i] = file_prefix + std::string(file_path);
    } else {
      free(file_path);
      return false;
    }

    free(file_path);
  }

  return true;
}

rmw_node_t *
rmw_create_node(
  const char * name,
  const char * namespace_,
  size_t domain_id,
  const rmw_node_security_options_t * security_options)
{
  if (!name) {
    RMW_SET_ERROR_MSG("name is null");
    return nullptr;
  }
  if (!security_options) {
    RMW_SET_ERROR_MSG("security_options is null");
    return nullptr;
  }

  ParticipantAttributes participantAttrs;

  // Load default XML profile.
  Domain::getDefaultParticipantAttributes(participantAttrs);

  participantAttrs.rtps.builtin.domainId = static_cast<uint32_t>(domain_id);
  // since the participant name is not part of the DDS spec
  participantAttrs.rtps.setName(name);
  // the node name is also set in the user_data
  size_t name_length = strlen(name);
  const char prefix[6] = "name=";
  participantAttrs.rtps.userData.resize(name_length + sizeof(prefix));
  memcpy(participantAttrs.rtps.userData.data(), prefix, sizeof(prefix) - 1);
  for (size_t i = 0; i < name_length; ++i) {
    participantAttrs.rtps.userData[sizeof(prefix) - 1 + i] = name[i];
  }
  participantAttrs.rtps.userData[sizeof(prefix) - 1 + name_length] = ';';

  if (security_options->security_root_path) {
    // if security_root_path provided, try to find the key and certificate files
#if HAVE_SECURITY
    std::array<std::string, 6> security_files_paths;

    if (get_security_file_paths(security_files_paths, security_options->security_root_path)) {
      eprosima::fastrtps::rtps::PropertyPolicy property_policy;
      using Property = eprosima::fastrtps::rtps::Property;
      property_policy.properties().emplace_back(
        Property("dds.sec.auth.plugin", "builtin.PKI-DH"));
      property_policy.properties().emplace_back(
        Property("dds.sec.auth.builtin.PKI-DH.identity_ca",
        security_files_paths[0]));
      property_policy.properties().emplace_back(
        Property("dds.sec.auth.builtin.PKI-DH.identity_certificate",
        security_files_paths[1]));
      property_policy.properties().emplace_back(
        Property("dds.sec.auth.builtin.PKI-DH.private_key",
        security_files_paths[2]));
      property_policy.properties().emplace_back(
        Property("dds.sec.crypto.plugin", "builtin.AES-GCM-GMAC"));

      property_policy.properties().emplace_back(Property(
          "dds.sec.access.plugin", "builtin.Access-Permissions"));
      property_policy.properties().emplace_back(Property(
          "dds.sec.access.builtin.Access-Permissions.permissions_ca", security_files_paths[3]));
      property_policy.properties().emplace_back(Property(
          "dds.sec.access.builtin.Access-Permissions.governance", security_files_paths[4]));
      property_policy.properties().emplace_back(Property(
          "dds.sec.access.builtin.Access-Permissions.permissions", security_files_paths[5]));

      participantAttrs.rtps.properties = property_policy;
    } else if (security_options->enforce_security) {
      RMW_SET_ERROR_MSG("couldn't find all security files!");
      return nullptr;
    }
#else
    RMW_SET_ERROR_MSG(
      "This Fast-RTPS version doesn't have the security libraries\n"
      "Please compile Fast-RTPS using the -DSECURITY=ON CMake option");
    return nullptr;
#endif
  }
  return create_node(name, namespace_, participantAttrs);
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  rmw_ret_t result_ret = RMW_RET_OK;
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != eprosima_fastrtps_identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return RMW_RET_ERROR;
  }

  Participant * participant = impl->participant;

  // Begin deleting things in the same order they were created in rmw_create_node().
  std::pair<StatefulReader *, StatefulReader *> edp_readers = participant->getEDPReaders();
  if (!edp_readers.first || !edp_readers.second) {
    RMW_SET_ERROR_MSG("failed to get EDPReader listener");
    result_ret = RMW_RET_ERROR;
  }

  if (edp_readers.first && !edp_readers.first->setListener(nullptr)) {
    RMW_SET_ERROR_MSG("failed to unset EDPReader listener");
    result_ret = RMW_RET_ERROR;
  }
  delete impl->secondarySubListener;
  if (edp_readers.second && !edp_readers.second->setListener(nullptr)) {
    RMW_SET_ERROR_MSG("failed to unset EDPReader listener");
    result_ret = RMW_RET_ERROR;
  }
  delete impl->secondaryPubListener;

  rmw_free(const_cast<char *>(node->name));
  node->name = nullptr;
  rmw_free(const_cast<char *>(node->namespace_));
  node->namespace_ = nullptr;
  rmw_node_free(node);

  if (RMW_RET_OK != rmw_destroy_guard_condition(impl->graph_guard_condition)) {
    RMW_SET_ERROR_MSG("failed to destroy graph guard condition");
    result_ret = RMW_RET_ERROR;
  }

  Domain::removeParticipant(participant);

  delete impl->listener;
  impl->listener = nullptr;
  delete impl;

  return result_ret;
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  auto impl = static_cast<CustomParticipantInfo *>(node->data);
  if (!impl) {
    RMW_SET_ERROR_MSG("node impl is null");
    return nullptr;
  }
  return impl->graph_guard_condition;
}
}  // extern "C"
