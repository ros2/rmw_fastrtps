# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

###############################################################################
#
# CMake module for finding eProsima FastRTPS.
#
# Output variables:
#
# - FastRTPS_FOUND: flag indicating if the package was found
# - FastRTPS_INCLUDE_DIR: Paths to the header files
#
# Example usage:
#
#   find_package(fastrtps_cmake_module REQUIRED)
#   find_package(FastRTPS MODULE)
#   # use FastRTPS_* variables
#
###############################################################################

# lint_cmake: -convention/filename, -package/stdargs

set(FastRTPS_FOUND FALSE)

find_path(FastRTPS_INCLUDE_DIR
  NAMES fastrtps/)

find_package(fastcdr REQUIRED CONFIG)
find_package(fastrtps REQUIRED CONFIG)

string(REGEX MATCH "^[0-9]+\\.[0-9]+" fastcdr_MAJOR_MINOR_VERSION "${fastcdr_VERSION}")
string(REGEX MATCH "^[0-9]+\\.[0-9]+" fastrtps_MAJOR_MINOR_VERSION "${fastrtps_VERSION}")

find_library(FastRTPS_LIBRARY
    NAMES fastrtpsd-${fastrtps_MAJOR_MINOR_VERSION} fastrtps-${fastrtps_MAJOR_MINOR_VERSION} fastrtps)

find_library(FastCDR_LIBRARY
    NAMES fastcdrd-${fastcdr_MAJOR_MINOR_VERSION} fastcdr-${fastcdr_MAJOR_MINOR_VERSION} fastcdr)

set(FastRTPS_LIBRARIES ${FastRTPS_LIBRARY} ${FastCDR_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FastRTPS
  FOUND_VAR FastRTPS_FOUND
  REQUIRED_VARS
    FastRTPS_INCLUDE_DIR
    FastCDR_LIBRARY
    FastRTPS_LIBRARY
    FastRTPS_LIBRARIES
)
