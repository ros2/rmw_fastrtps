# **External Dependency Quality declaration** `fastrtps` 

This document is a declaration of software quality for the `fastrtps` external dependency, based on the guidelines in [REP-2004](https://github.com/ros-infrastructure/rep/blob/rep-2004/rep-2004.rst).

As stated in their public repository, eprosima Fast RTPS is a C++ implementation of the RTPS (Real Time Publish Subscribe) protocol, which provides publisher-subscriber communications over unreliable transports such as UDP, as defined and maintained by the Object Management Group (OMG) consortium.

## Summary

The `fastrtps` as a complete software solution manages to meet most of the ROS2 package requirements for Quality Level 1, including a complete established workflow for new changes in its repository, testing for the features of the software and documentation with examples.

As for the missing topics, eProsima does not state their versioning policies for the repository and does not state explicitly the API/ABI policies for the code in `fastrtps`. As the code deals with the standard RTPS implementation, it is not expected that the basic API functionalities will be changed. In any case, the code is actively being tested against their [CI](http://jenkins.eprosima.com:8080/) and in the [ROS2 CI](https://ci.ros2.org/).

There is no coverage information summary/report and, apparently, no checks being made in terms of linters/static analysis, however, as the core parts used in ROS2 are actively being tested under the packages that use `fastrtps` (`rmw_fastrtps_cpp, rmw_fastrtps_shared_cpp, rosidl_typesupport_fastrtps_c, rosidl_typesupport_fastrtps_cpp`).

In terms of ROS2 package metrics this library is considered to be Quality Level 4. Adding unit testing for the functions used in ROS2 packages, coverage statistics and version pinning will be needed to achieve Quality Level 1.

# Comparison with ROS packages quality standards

## Version Policy [1]

 ### Version Scheme [1.i]

There is no public information related to how the eProsima team handles the versioning of `fastrtps`. However, each version is documented in their documentation with the included features and bug fixes.

### Version Stability [1.ii]

The library has version >= 1.0.0.

### Public API Declaration [1.iii]
    
Fast RTPS has embedded documentation generated using Doxygen. It is currently [hosted](http://www.eprosima.com/docs/fast-rtps/1.5.0/html/group___f_a_s_t_r_t_p_s___g_e_n_e_r_a_l___a_p_i.html) with the source code for version 1.5 (currently at 1.9.4).

### API Stability Policy [1.iv]
    
There is no explicit policy related to API stability. This package should be pinned to a particular Fast-RTPS version to be considered high quality.

### ABI Stability Policy [1.v]
    
There is no explicit policy related to ABI stability. This package should be pinned to a particular Fast-RTPS version to be considered high quality.

### ABI and ABI Stability Within a Released ROS Distribution [1.vi]
   
There is no direct correlation between API and ABI stability of the library within ROS distributions. This package should be pinned to a particular Fast-RTPS version to be considered high quality.

## Change Control Process [2]

### Change Requests [2.i]
    
Fast RTPS does not have a declared change control process. However, it appears in their [history of commits](https://github.com/eProsima/Fast-RTPS/commits/master) that most of the code passes through a Pull Request, except some minor cases related to styling issues or formatting.

### Contributor Origin [2.ii]
    
Based on the public GitHub repository [history of commits](https://github.com/eProsima/Fast-RTPS/commits/master) of `fastrtps` it can be seen that there is no confirmation of contributor origin enforcement used in the repository.

### Peer Review Policy [2.iii]
    
Not publicly stated, but browsing through their [list of closed pull requests](https://github.com/eProsima/Fast-RTPS/pulls?q=is%3Apr+is%3Aclosed) it can be seen that their workflow requires at least one peer review approving the changes and building against CI before merging code to the master branch in the repository.

### Continuous Integration [2.iv]
    
Not publicly stated, but browsing through their [list of closed pull requests](https://github.com/eProsima/Fast-RTPS/pulls?q=is%3Apr+is%3Aclosed) it can be seen that their workflow requires at least one peer review approving the changes and building against CI before merging code to the master branch in the repository.

###  Documentation Policy [2.v]
    
Changes are not documented on each software change request, although the changes explicitly state what’s fixed/changed, there is no clear consistency among how to document these changes. This is better reflected in releases summaries, where the main new features are shown with the list bugs/issues fixed.

## Documentation [3]

### Feature Documentation [3.i]
    
A complete manual with the whole overview of how the Fast RTPS protocol works, and how the implementation can be used, is provided [here](https://fast-rtps.docs.eprosima.com/en/latest/).

### Public API Documentation [3.ii]
    
The public API is documented through their generated [Doxygen documentation](http://www.eprosima.com/docs/fast-rtps/1.5.0/html/group___f_a_s_t_r_t_p_s___g_e_n_e_r_a_l___a_p_i.html) and also in the [User Manual](https://github.com/eProsima/Fast-RTPS/blob/master/doc/pdf/Fast%20RTPS%20-%20User%20Manual.odt) document provided in their repository.

### License [3.iii]

The declared License for the code in the repository is [Apache License 2.0](https://github.com/eProsima/Fast-RTPS/blob/master/LICENSE).

### Copyright Statements [3.iv]

A check to the source files on their repository shows that the source files include copyright statements to *“Proyectos y Sistemas de Mantenimiento SL (eProsima)”*. It is not stated if this is enforced with any kind of linter analysis.

## Testing [4]

### Feature Testing [4.i]
    
There are several [tests](https://github.com/eProsima/Fast-RTPS/tree/master/test) in the `fastrtps` repository, the unit tests folder and the communication folders seem to be testing all the features of the software.

### Public API Testing [4.ii]
    
There are several [tests](https://github.com/eProsima/Fast-RTPS/tree/master/test) in the `fastrtps` repository, the unit tests folder, the communication, the profiling, and the realtime folders seem to be testing all the public API of the software.

### Coverage [4.iii]
    
Checking their [CI jobs](http://jenkins.eprosima.com:8080/), and the testing folders, there seems to be no automatic code coverage analysis being made or anything related to code coverage analysis.

Coverage checks should be made to consider this dependency to apply for a higher quality level.

### Performance [4.iv]
    
Fast RTPS includes a directory of performance tests in their [test directory](https://github.com/eProsima/Fast-RTPS/tree/master/test). Some [CI jobs](http://jenkins.eprosima.com:8080/job/FastRTPS%20Performance%20Tests%20Plots/) relating to performance analysis have been run. Based on their frequency, it is unlikely there is a performance regression policy.

### Linters and Static Analysis [4.v]
    
Based on the [CI jobs test results](http://jenkins.eprosima.com:8080/job/FastRTPS%20Manual%20Linux/lastSuccessfulBuild/testReport/), there seems there is no automatic static analysis being made on the code developed.

## Dependencies [5]

### Direct Runtime ROS Dependencies [5.i]

`fastrtps` has not direct runtime ROS dependencies.

### Optional Direct Runtime ROS Dependencies [5.ii]

`fastrtps` has no run-time or build-time dependencies that need to be considered for this declaration.

### Direct Runtime non-ROS Dependency [5.iii]

`fastrtps` depends on the following packages:

  - `libasio-dev` 
  - `libtinyxml2-dev`
  - `fast-cdr`
  - `foonathan_memory`

The first two dependencies are suggested to be installed for Linux using apt package manager, which would pull them from the Debian upstream. This is a [list of advantages](https://wiki.debian.org/AdvantagesForUpstream) of using upstream packages, the fact that software and their patches are tested and the code is maintained available leads us to decide that these libraries as dependencies can be considered of Quality Level 1 for their use within this library once there is testing covered for the functions of this libray used in ROS2 packages.

The other two are installed from the eProsima repositories as well, their current quality declarations can be found in [Quality Declaration `fast-cdr`](https://github.com/ros2/rmw_fastrtps/pull/360) and [Quality Declaration `foonathan_memory`](https://github.com/eProsima/foonathan_memory_vendor/pull/22).

## Platform Support [6]
    
As stated in their [repository page](https://github.com/eProsima/Fast-RTPS) the library is supported for Linux, Windows, and Mac, but there is no specific information about the OS covered nor any claims for linux-aarch64. This item is considered partially covered.

##  Vulnerability Disclosure Policy [7.i]

`fastrtps` does not have a Vulnerability Disclosure Policy.
