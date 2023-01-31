#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

#include "rcutils/logging_macros.h"
#include "rmw_fastrtps_shared_cpp/utils/net_utils.hpp"

namespace rmw_fastrtps_shared_cpp
{
namespace utils
{
std::string get_fqdn_for_host(const std::string& hostname) {
  struct addrinfo hints, *addresses = nullptr;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_CANONNAME;

  if (getaddrinfo(hostname.c_str(), nullptr, &hints, &addresses) != 0) {
    RCUTILS_LOG_WARN_NAMED("rmw_fastrtps_shared_cpp", "Failed to look up fully-qualified domain name for host %s", hostname.c_str());
    return hostname;
  }

  return std::string(addresses->ai_canonname);
}

std::unordered_set<std::string> get_peer_aliases(const std::string& peer) {
  std::unordered_set<std::string> aliases;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  auto result = inet_pton(addr.sin_family, peer.c_str(), &addr.sin_addr);
  if (result != 1) {
    // Could not interpret the string as an IPv4 address, so try IPv6

    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    result = inet_pton(addr6.sin6_family, peer.c_str(), &addr6.sin6_addr);
    if (result != 1) {
      // Failure to convert the string to a binary IP address means we treat it
      // as a hostname, and find all its IP addresses
      std::string ip_addr;

      struct addrinfo *addresses = nullptr;
      if (getaddrinfo(peer.c_str(), nullptr, nullptr, &addresses) != 0) {
        // Failed to look up the hostname
        RCUTILS_LOG_WARN_NAMED("rmw_fastrtps_shared_cpp",
                               "Could not look up hostname %s", peer.c_str());
      } else {
        for (struct addrinfo *address_entry = addresses;
             address_entry != nullptr; address_entry = address_entry->ai_next) {
          void * address_data = nullptr;
          char address_string[INET6_ADDRSTRLEN];
          if (address_entry->ai_family == AF_INET) { // IPv4
            address_data = &reinterpret_cast<struct sockaddr_in *>(address_entry->ai_addr)->sin_addr;
          } else { // IPv6
            address_data = &reinterpret_cast<struct sockaddr_in6 *>(address_entry->ai_addr)->sin6_addr;
          }
          inet_ntop(address_entry->ai_family, address_data, address_string, sizeof(address_string));
          // Avoid duplicate aliases (e.g. when two identical IPs are returned for SOCK_STREAM and SOCK_DGRAM)
          aliases.insert(address_string);
          RCUTILS_LOG_DEBUG_NAMED(
              "rmw_fastrtps_shared_cpp",
              "Added IP address %s for host %s to static peers", address_string,
              peer.c_str());
          
        }

        freeaddrinfo(addresses);
      }

      // Also add the fully-qualified domain name for the host as an alias
      std::string fqdn = get_fqdn_for_host(peer);
     
      RCUTILS_LOG_DEBUG_NAMED(
          "rmw_fastrtps_shared_cpp",
          "Added fully-qualified domain name %s as alias for host %s to static peers", fqdn.c_str(),
          peer.c_str());
      aliases.insert(fqdn);
      
    } else {
      // Success converting the string to a binary IPv6 address, so find the hostname for that address
      char hostname[NI_MAXHOST];

      aliases.insert(hostname);
      RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "Added host %s as alias for IPv6 address %s", hostname, peer.c_str());

    }
  } else {
    // Success converting the string to a binary IPv4 address, so find the hostname for that address
    char hostname[NI_MAXHOST];

    if (getnameinfo(reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr), hostname, sizeof(hostname), nullptr, 0, NI_NAMEREQD) == 0) {
      aliases.insert(hostname);
      RCUTILS_LOG_DEBUG_NAMED("rmw_fastrtps_shared_cpp", "Added host %s as alias for IPv4 address %s", hostname, peer.c_str());
    }
  }

  return aliases;
}
}
} // namespace rmw_fastrtps_shared_cpp