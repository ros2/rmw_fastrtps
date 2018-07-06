# Copyright 2016-2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

find_library(FastCDR_LIBRARY_RELEASE
  NAMES fastcdr-${fastcdr_MAJOR_MINOR_VERSION} fastcdr)

find_library(FastCDR_LIBRARY_DEBUG
  NAMES fastcdrd-${fastcdr_MAJOR_MINOR_VERSION})

if(FastCDR_LIBRARY_RELEASE AND FastCDR_LIBRARY_DEBUG)
  set(FastCDR_LIBRARIES
    optimized ${FastCDR_LIBRARY_RELEASE}
    debug ${FastCDR_LIBRARY_DEBUG}
  )
elseif(FastCDR_LIBRARY_RELEASE)
  set(FastCDR_LIBRARIES
    ${FastCDR_LIBRARY_RELEASE}
  )
elseif(FastCDR_LIBRARY_DEBUG)
  set(FastCDR_LIBRARIES
    ${FastCDR_LIBRARY_DEBUG}
  )
else()
  set(FastCDR_LIBRARIES "")
endif()

find_library(FastRTPS_LIBRARY_RELEASE
  NAMES fastrtps-${fastrtps_MAJOR_MINOR_VERSION} fastrtps)

find_library(FastRTPS_LIBRARY_DEBUG
  NAMES fastrtpsd-${fastrtps_MAJOR_MINOR_VERSION})

if(FastRTPS_LIBRARY_RELEASE AND FastRTPS_LIBRARY_DEBUG)
  set(FastRTPS_LIBRARIES
    optimized ${FastRTPS_LIBRARY_RELEASE}
    debug ${FastRTPS_LIBRARY_DEBUG}
    ${FastCDR_LIBRARIES}
  )
elseif(FastRTPS_LIBRARY_RELEASE)
  set(FastRTPS_LIBRARIES
    ${FastRTPS_LIBRARY_RELEASE}
    ${FastCDR_LIBRARIES}
  )
elseif(FastRTPS_LIBRARY_DEBUG)
  set(FastRTPS_LIBRARIES
    ${FastRTPS_LIBRARY_DEBUG}
    ${FastCDR_LIBRARIES}
  )
else()
  set(FastRTPS_LIBRARIES "")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FastRTPS
  FOUND_VAR FastRTPS_FOUND
  REQUIRED_VARS
    FastRTPS_INCLUDE_DIR
    FastRTPS_LIBRARIES
)
