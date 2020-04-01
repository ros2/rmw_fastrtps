# **External Dependency Quality declaration** `fast-cdr` 

This document is a declaration of software quality for the FastCDR external dependency, based on the guidelines in [REP-2004](https://github.com/ros-infrastructure/rep/blob/rep-2004/rep-2004.rst).

As stated in their public repository, *eProsima FastCDR is a C++ library that provides two serialization mechanisms. One is the standard CDR serialization mechanism, while the other is a faster implementation that modifies the standard*.

## Summary
The `fast-cdr` library is used as one of the dependencies of the `fastrtps`, it is important for the eProsima team to consider this software to be reliable. On its current state, meets most of the requirements to qualify as a level 1 package.

As for the missing topics, eProsima does not state their versioning policies for the repository and does not state explicitly the API/ABI policies for the code in  `fast-cdr`. As the code deals with the standard RPTS implementation, and it is used by their software `fastrtps` it is not expected that the basic API functionalities will be changed. In any case, the code is actively being tested against their  [CI](http://jenkins.eprosima.com:8080/) (both directly with the included test cases and indirectly through the tests of `fastrtps`)  and indirectly in the [ROS2 CI](https://ci.ros2.org/).

There is no coverage information summary/report and, apparently, no checks being made in terms of linters/static analysis, however, as the core parts used in ROS2 are actively being tested under the packages that use  `fast-cdr`.

Considering the previously mentioned reasons, especially the fact that this software is being tested both directly and indirectly with [CI jobs](http://jenkins.eprosima.com:8080/), we consider this library to be robust and reliable, and hence we declare it to qualify as a level 1 external dependency.

## Comparison with ROS packages quality standards

### Version policy

 1. *Must have a version policy*

There is no public information related to how the eProsima team handles the versioning of `fast-cdr`. However, [each release](https://github.com/eProsima/Fast-CDR/releases) is documented in their documentation with the included features and bug fixes.

 2. Must be at a stable version (e.g. for semver that means version >= 1.0.0)

The current version is [1.0.13](https://github.com/eProsima/Fast-CDR/releases/tag/v1.0.13)

3.  *Must have a strictly declared public API*
    
The API is declared using Doxygen documentation available to be [generated](https://github.com/eProsima/Fast-CDR/blob/master/utils/doxygen/doxyfile) with the source code. They have it hosted in this [page](https://www.eprosima.com/docs/fast-buffers/0.3.0/html/group___f_a_s_t_c_d_r_a_p_i_r_e_f_e_r_e_n_c_e.html) for the version 0.3.0 (currently at 1.0.13, outdated version)

4.  *Must have a policy for API stability*
    
There is no explicit policy related to API stability. However, as eProsima widely announces that their implementation is [adopted in ROS2](https://www.eprosima.com/index.php/company-all/news/135-fast-rtps-demonstrates-its-reliability-in-ros-2-navigation-2), it is not expected that this dependency will generate API issues with the ROS2 code.

5. *Must have a policy for ABI stability*
    
There is no explicit policy related to ABI stability. However, as eProsima widely announces that their implementation is [adopted in ROS2](https://www.eprosima.com/index.php/company-all/news/135-fast-rtps-demonstrates-its-reliability-in-ros-2-navigation-2), it is not expected that this dependency will generate API issues with the ROS2 code.

6.  *Must have a policy that keeps API and ABI stability within a released ROS distribution*
   
There is no direct correlation between API and ABI stability of the library within ROS distributions.

### Change Control Process

7.  *Must have all code changes occur through a change request (e.g. pull request, merge request, etc.)*
    
Browsing through the public GitHub repository [history of commits](https://github.com/eProsima/Fast-CDR/commits/master) of `fast-cdr` shows that most of the code passes through a Pull Request, except some minor cases related to styling issues or formatting.

8.  *Must have confirmation of contributor origin (e.g. [DCO](https://developercertificate.org/), CLA, etc.)*
    
Based on the public GitHub repository [history of commits](https://github.com/eProsima/Fast-CDR/commits/master) of `fast-cdr` it can be seen that there is no confirmation of contributor origin enforcement used in the repository.

9.  *Must have peer review policy for all change requests (e.g. require one or more reviewers)*
    
Not publicly stated, but browsing through their [list of closed pull requests](https://github.com/eProsima/Fast-CDR/pulls?q=is:pr%20is:closed) it can be seen that their workflow requires at least one peer review approving the changes and building against CI before merging code to the master branch in the repository.

10.  *Must have Continuous Integration (CI) policy for all change requests*
    
Not publicly stated, but browsing through their [list of closed pull requests](https://github.com/eProsima/Fast-CDR/pulls?q=is:pr%20is:closed) it can be seen that their workflow requires at least one peer review approving the changes and building against CI before merging code to the master branch in the repository.


11.  *Must have documentation policy for all change requests*
    
Changes are not documented on each software change request, although the changes explicitly state what’s fixed/changed, there is no clear consistency among how to document these changes. This is better reflected in releases summaries, where the main new features are shown with the list bugs/issues fixed.

### Documentation:

12.  *Must have documentation for each "feature" (e.g. for rclcpp: create a node, publish a message, spin, etc.)*
    
A complete manual with the whole overview of how the Fast-CDR libraryworks is provided [here](https://github.com/eProsima/Fast-CDR/blob/master/doc/Users%20Manual.odt).

13.  *Must have documentation for each item in the public API (e.g. functions, classes, etc.)*
    
The public API is documented through their generated [Doxygen documentation](https://www.eprosima.com/docs/fast-buffers/0.3.0/html/group___f_a_s_t_c_d_r_a_p_i_r_e_f_e_r_e_n_c_e.html) and also in the [User Manual](https://github.com/eProsima/Fast-CDR/blob/master/doc/Users%20Manual.odt) document provided in their repository.

14.  *Must have a declared license or set of licenses*

The declared License for the code in the repository is [Apache License 2.0](https://github.com/eProsima/Fast-CDR/blob/master/LICENSE).

15.  *Must have a copyright statement in each source file*

A check to the source files on their repository shows that the source files include copyright statements to *“Proyectos y Sistemas de Mantenimiento SL (eProsima)”*. It is not stated if this is enforced with any kind of linter analysis.

16.  *Must have a "quality declaration" document, which declares the quality level and justifies how the package meets each of the requirements*
    
This document will be the quality declaration supporting that `fast-cdr` can be qualified as Quality Level 1 to be used within the ROS2 ecosystem.

### Testing:

17.  *Must have system tests which cover all items in the "feature" documentation*
    
There are various [tests](https://github.com/eProsima/Fast-CDR/tree/master/test) in the `fast-cdr` repository, specifically the `SimpleTest.cpp` seems to be testing all the features of the library.

18.  *Must have system, integration, and/or unit tests which cover all of the public API*
    
There are several [tests](https://github.com/eProsima/Fast-CDR/tree/master/test) in the `fast-cdr` repository, specifically the `SimpleTest.cpp` seems to be testing all the functions/types/operations of the library.

19.  *Must have code coverage, and a policy for changes*
    
Checking their [CI jobs](http://jenkins.eprosima.com:8080/), and the testing folders, there seems to be no automatic code coverage analysis being made or anything related to code coverage analysis.

20.  *Performance tests, and performance regression policy*
    
Checking their [tests folder](https://github.com/eProsima/Fast-CDR/tree/master/test), it is shown there are no performance tests available for this library. As the library provides mainly basic types, and a couple of serialization mechanisms it is not needed for this case to create performance tests.

21.  *Linters and Static Analysis*
    
Based on the [CI jobs test results](http://jenkins.eprosima.com:8080/job/FastCDR%20Manual%20Linux/lastSuccessfulBuild/testReport/projectroot/test/), there seems there is no automatic static analysis being made on the code developed.


### Dependencies:

16.  Must not have direct runtime "ROS" dependencies which are not at the same level as the package in question ('Level N'), but…

`fast-cdr` only depends on functions provided by the standard C++ definitions.

### Platform Support:

23.  Must support all tier 1 platforms for ROS 2, as defined in [REP-2000](https://www.ros.org/reps/rep-2000.html#support-tiers)
    
There is no explicit support for any platform in their [repository page](https://github.com/eProsima/Fast-CDR). However, their CI jobs are triggered for both Linux and Windows. MacOS is not tested directly, but as this library is a dependence for the `Fast-RTPS` software, and that one is tested in the three platforms, it is safe to assume this library is supported in MacOS as well.