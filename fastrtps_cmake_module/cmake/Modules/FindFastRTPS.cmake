###############################################################################
#
# CMake module for finding eProsima FastRTPS.
#
# Input variables:
#
# - FASTRTPSHOME (optional): When specified, header files and libraries
#   will be searched for in `${FASTRTPSHOME}/include`
#   and `${FASTRTPSHOME}/lib` respectively.
#
# Output variables:
#
# - FastRTPS_FOUND: flag indicating if the package was found
# - FastRTPS_INCLUDE_DIR: Paths to the header files
# - FastRTPS_HOME: Root directory for the eProsima FastRTPS install. Might be not set.
# - FastRTPS_LIBRARIES: Name to the C++ libraries including the path
# - FastRTPS_LIBRARY_DIRS: Paths to the libraries
# - FastRTPS_LIBRARY_DIR: Path to libraries; guaranteed to be a single path
# - FastRTPS_DEFINITIONS: Definitions to be passed on
#
# Example usage:
#
#   find_package(fastrtps_cmake_module REQUIRED)
#   find_package(FastRTPS MODULE)
#   # use FastRTPS_* variables
#
###############################################################################

set(FastRTPS_FOUND FALSE)

# -----------------------------------------
# Detect environment variable
# -----------------------------------------
if($ENV{FASTRPCHOME})
    set(FastRTPS_HOME $ENV{FASTRPCHOME})
endif()

# -----------------------------------------
# Search for eProsima FastRTPS include DIR
# -----------------------------------------
set(_fastrtps_INCLUDE_SEARCH_DIRS "")

if(FastRTPS_HOME)
    list(APPEND _fastrtps_INCLUDE_SEARCH_DIRS ${FastRTPS_HOME}/include)
endif()

find_path(FastRTPS_INCLUDE_DIR
    NAMES fastrtps/)
