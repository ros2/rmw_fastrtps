# **External Dependency Quality declaration** `fastrtps` 

This document is a declaration of software quality for the `fastrtps` external dependency, based on the guidelines in [REP-2004](https://github.com/ros-infrastructure/rep/blob/rep-2004/rep-2004.rst).

As stated in their public repository, eprosima Fast RTPS is a C++ implementation of the RTPS (Real Time Publish Subscribe) protocol, which provides publisher-subscriber communications over unreliable transports such as UDP, as defined and maintained by the Object Management Group (OMG) consortium.

## Summary

The `fastrtps` as a complete software solution manages to meet most of the ROS2 package requirements for Quality Level 1, including a complete established workflow for new changes in its repository, testing for the features of the software and documentation with examples.

As for the missing topics, eProsima does not state their versioning policies for the repository and does not state explicitly the API/ABI policies for the code in `fastrtps`. As the code deals with the standard RTPS implementation, it is not expected that the basic API functionalities will be changed. In any case, the code is actively being tested against their [CI](http://jenkins.eprosima.com:8080/) and in the [ROS2 CI](https://ci.ros2.org/).

There is no coverage information summary/report and, apparently, no checks being made in terms of linters/static analysis, however, as the core parts used in ROS2 are actively being tested under the packages that use `fastrtps` (`rmw_fastrtps_cpp, rmw_fastrtps_shared_cpp, rosidl_typesupport_fastrtps_c, rosidl_typesupport_fastrtps_cpp`).

In terms of ROS2 package metrics this library is considered to be Quality Level 4.


## Comparison with ROS packages quality standards

### Version policy

 1. *Must have a version policy*

There is no public information related to how the eProsima team handles the versioning of `fastrtps`. However, each version is documented in their documentation with the included features and bug fixes.

 2. Must be at a stable version (e.g. for semver that means version >= 1.0.0)

The current version is 1.94

3.  *Must have a strictly declared public API*
    
Fast RTPS has embedded documentation generated using Doxygen. It is currently [hosted](http://www.eprosima.com/docs/fast-rtps/1.5.0/html/group___f_a_s_t_r_t_p_s___g_e_n_e_r_a_l___a_p_i.html) with the source code for version 1.5 (currently at 1.9.4)

4.  *Must have a policy for API stability*
    
There is no explicit policy related to API stability. However, as eProsima widely announces that their implementation is adopted in ROS2, it is not expected that this dependency will generate API issues with the ROS2 code.

5. *Must have a policy for ABI stability*
    
There is no explicit policy related to ABI stability. However, as eProsima widely announces that their implementation is adopted in ROS2, it is not expected that this dependency will generate API issues with the ROS2 code.

6.  *Must have a policy that keeps API and ABI stability within a released ROS distribution*
   
There is no direct correlation between API and ABI stability of the library within ROS distributions.

### Change Control Process

7.  *Must have all code changes occur through a change request (e.g. pull request, merge request, etc.)*
    
Fast RTPS does not have a declared change control process. However, it appears in their [history of commits](https://github.com/eProsima/Fast-RTPS/commits/master) that most of the code passes through a Pull Request, except some minor cases related to styling issues or formatting.

8.  *Must have confirmation of contributor origin (e.g. [DCO](https://developercertificate.org/), CLA, etc.)*
    
Based on the public GitHub repository [history of commits](https://github.com/eProsima/Fast-RTPS/commits/master) of `fastrtps` it can be seen that there is no confirmation of contributor origin enforcement used in the repository.

9.  *Must have peer review policy for all change requests (e.g. require one or more reviewers)*
    
Not publicly stated, but browsing through their [list of closed pull requests](https://github.com/eProsima/Fast-RTPS/pulls?q=is%3Apr+is%3Aclosed) it can be seen that their workflow requires at least one peer review approving the changes and building against CI before merging code to the master branch in the repository.

10.  *Must have Continuous Integration (CI) policy for all change requests*
    
Not publicly stated, but browsing through their [list of closed pull requests](https://github.com/eProsima/Fast-RTPS/pulls?q=is%3Apr+is%3Aclosed) it can be seen that their workflow requires at least one peer review approving the changes and building against CI before merging code to the master branch in the repository.

11.  *Must have documentation policy for all change requests*
    
Changes are not documented on each software change request, although the changes explicitly state what’s fixed/changed, there is no clear consistency among how to document these changes. This is better reflected in releases summaries, where the main new features are shown with the list bugs/issues fixed.

### Documentation:

12.  *Must have documentation for each "feature" (e.g. for rclcpp: create a node, publish a message, spin, etc.)*
    
A complete manual with the whole overview of how the Fast RTPS protocol works, and how the implementation can be used, is provided [here](https://fast-rtps.docs.eprosima.com/en/latest/).

13.  *Must have documentation for each item in the public API (e.g. functions, classes, etc.)*
    
The public API is documented through their generated [Doxygen documentation](http://www.eprosima.com/docs/fast-rtps/1.5.0/html/group___f_a_s_t_r_t_p_s___g_e_n_e_r_a_l___a_p_i.html) and also in the [User Manual](https://github.com/eProsima/Fast-RTPS/blob/master/doc/pdf/Fast%20RTPS%20-%20User%20Manual.odt) document provided in their repository.

14.  *Must have a declared license or set of licenses*

The declared License for the code in the repository is [Apache License 2.0](https://github.com/eProsima/Fast-RTPS/blob/master/LICENSE).

15.  *Must have a copyright statement in each source file*

A check to the source files on their repository shows that the source files include copyright statements to *“Proyectos y Sistemas de Mantenimiento SL (eProsima)”*. It is not stated if this is enforced with any kind of linter analysis.

16.  *Must have a "quality declaration" document, which declares the quality level and justifies how the package meets each of the requirements*
    
This document will be the quality declaration supporting that `fastrtps` can be qualified as Quality Level 1 to be used within the ROS2 ecosystem.

### Testing:

17.  *Must have system tests which cover all items in the "feature" documentation*
    
There are several [tests](https://github.com/eProsima/Fast-RTPS/tree/master/test) in the `fastrtps` repository, the unit tests folder and the communication folders seem to be testing all the features of the software.

18.  *Must have system, integration, and/or unit tests which cover all of the public API*
    
There are several [tests](https://github.com/eProsima/Fast-RTPS/tree/master/test) in the `fastrtps` repository, the unit tests folder, the communication, the profiling, and the realtime folders seem to be testing all the public API of the software.

19.  *Must have code coverage, and a policy for changes*
    
Checking their [CI jobs](http://jenkins.eprosima.com:8080/), and the testing folders, there seems to be no automatic code coverage analysis being made or anything related to code coverage analysis.

20.  *Performance tests, and performance regression policy*
    
Checking their [tests folder](https://github.com/eProsima/Fast-RTPS/tree/master/test), it is available a folder with performance tests, and also some [CI jobs](http://jenkins.eprosima.com:8080/job/FastRTPS%20Performance%20Tests%20Plots/) related to performance analysis. It is not possible to know whether there is a performance regression policy with the information from their repository.

21.  *Linters and Static Analysis*
    
Based on the [CI jobs test results](http://jenkins.eprosima.com:8080/job/FastRTPS%20Manual%20Linux/lastSuccessfulBuild/testReport/), there seems there is no automatic static analysis being made on the code developed.

### Dependencies:

16.  Must not have direct runtime "ROS" dependencies which are not at the same level as the package in question ('Level N'), but…
    
`fastrtps` depends on the following packages:

`libasio-dev` `libtinyxml2-dev` `fast-cdr` `foonathan_memory`

The first two dependencies are suggested to be installed for Linux using apt package manager, which would pull them from the Debian upstream. This is a [list of advantages](https://wiki.debian.org/AdvantagesForUpstream) of using upstream packages, the fact that software and their patches are tested and the code is maintained available leads us to decide that these libraries as dependencies can be considered of Quality Level 1 as well.

The other two are installed from the eProsima repositories as well, their current quality declarations can be found in [Quality Declaration `fast-cdr`](https://github.com/ros2/rmw_fastrtps/pull/360) and [Quality Declaration `foonathan_memory`](https://github.com/eProsima/foonathan_memory_vendor/pull/22)

TO DO: Windows suggested method to install the dependencies is using [Chocolatey](https://chocolatey.org/) and the list of ROS2 dependencies [choco repo](https://github.com/ros2/choco-packages/releases/tag/2020-02-24). For Windows, the upstream argument won’t apply

TO DO: Add link

### Platform Support:

23.  Must support all tier 1 platforms for ROS 2, as defined in [REP-2000](https://www.ros.org/reps/rep-2000.html#support-tiers)
    
As stated in their [repository page](https://github.com/eProsima/Fast-RTPS) the library is supported for Linux, Windows, and Mac, meaning that this requirement is met.
