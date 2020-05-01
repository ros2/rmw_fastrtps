# **External Dependency Quality declaration** `fast-cdr` 

This document is a declaration of software quality for the FastCDR external dependency, based on the guidelines in [REP-2004](https://github.com/ros-infrastructure/rep/blob/rep-2004/rep-2004.rst).

As stated in their public repository, *eProsima FastCDR is a C++ library that provides two serialization mechanisms. One is the standard CDR serialization mechanism, while the other is a faster implementation that modifies the standard*.

## Summary
The `fast-cdr` library is used as one of the dependencies of the `fastrtps`, it is important for the eProsima team to consider this software to be reliable. On its current state, meets most of the requirements to qualify as a level 1 package.

As for the missing topics, eProsima does not state their versioning policies for the repository and does not state explicitly the API/ABI policies for the code in  `fast-cdr`. As the code deals with the standard RPTS implementation, and it is used by their software `fastrtps` it is not expected that the basic API functionalities will be changed. In any case, the code is actively being tested against their  [CI](http://jenkins.eprosima.com:8080/) (both directly with the included test cases and indirectly through the tests of `fastrtps`)  and indirectly in the [ROS2 CI](https://ci.ros2.org/).

In terms of ROS2 package metrics this library is considered to be Quality Level 4. Adding unit testing for the functions used in ROS2 packages, coverage statistics and version pinning will be needed to achieve Quality Level 1.

# Comparison with ROS packages quality standards

## Version Policy [1]

### Version Scheme [1.i]

There is no public information related to how the eProsima team handles the versioning of `fast-cdr`. However, [each release](https://github.com/eProsima/Fast-CDR/releases) is documented in their documentation with the included features and bug fixes.

### Version Stability [1.ii]

The library has version >= 1.0.0.

### Public API Declaration [1.iii]
    
The API is declared using Doxygen documentation available to be [generated](https://github.com/eProsima/Fast-CDR/blob/master/utils/doxygen/doxyfile) with the source code. They have it hosted in this [page](https://www.eprosima.com/docs/fast-buffers/0.3.0/html/group___f_a_s_t_c_d_r_a_p_i_r_e_f_e_r_e_n_c_e.html) for the version 0.3.0. However, that is an outdated version, as currently the software version is 1.0.13.

### API Stability Policy [1.iv]
    
There is no explicit policy related to API stability. This package should be pinned to a particular Fast-CDR version to be considered high quality.

### ABI Stability Policy [1.v]
    
There is no explicit policy related to ABI stability. This package should be pinned to a particular Fast-CDR version to be considered high quality.

### ABI and ABI Stability Within a Released ROS Distribution [1.vi]
   
There is no direct correlation between API and ABI stability of the library within ROS distributions. This package should be pinned to a particular Fast-CDR version to be considered high quality.

## Change Control Process [2]

### Change Requests [2.i]
    
Fast-CDR does not have a declared change control process. However, browsing through its public GitHub repository [history of commits](https://github.com/eProsima/Fast-CDR/commits/master) shows that most of the code passes through a Pull Request, except some minor cases related to styling issues or formatting.

### Contributor Origin [2.ii]
    
Based on the public GitHub repository [history of commits](https://github.com/eProsima/Fast-CDR/commits/master) of `fast-cdr` it can be seen that there is no confirmation of contributor origin enforcement used in the repository.

### Peer Review Policy [2.iii]
    
Not publicly stated, but browsing through their [list of closed pull requests](https://github.com/eProsima/Fast-CDR/pulls?q=is:pr%20is:closed) it can be seen that their workflow requires at least one peer review approving the changes and building against CI before merging code to the master branch in the repository.

### Continuous Integration [2.iv]
    
Not publicly stated, but browsing through their [list of closed pull requests](https://github.com/eProsima/Fast-CDR/pulls?q=is:pr%20is:closed) it can be seen that their workflow requires at least one peer review approving the changes and building against CI before merging code to the master branch in the repository.

###  Documentation Policy [2.v]
    
Changes are not documented on each software change request, although the changes explicitly state what’s fixed/changed, there is no clear consistency among how to document these changes. This is better reflected in releases summaries, where the main new features are shown with the list bugs/issues fixed.

## Documentation [3]

### Feature Documentation [3.i]
    
A complete manual with the whole overview of how the Fast-CDR library works is provided [here](https://github.com/eProsima/Fast-CDR/blob/master/doc/Users%20Manual.odt).

### Public API Documentation [3.ii]
    
The public API is documented through their generated [Doxygen documentation](https://www.eprosima.com/docs/fast-buffers/0.3.0/html/group___f_a_s_t_c_d_r_a_p_i_r_e_f_e_r_e_n_c_e.html) and also in the [User Manual](https://github.com/eProsima/Fast-CDR/blob/master/doc/Users%20Manual.odt) document provided in their repository. However, the generated API shown is outdated.

### License [3.iii]

The declared License for the code in the repository is [Apache License 2.0](https://github.com/eProsima/Fast-CDR/blob/master/LICENSE).

### Copyright Statements [3.iv]

A check to the source files on their repository shows that the source files include copyright statements to *“Proyectos y Sistemas de Mantenimiento SL (eProsima)”*. It is not stated if this is enforced with any kind of linter analysis.

## Testing [4]

### Feature Testing [4.i]
    
There are various [tests](https://github.com/eProsima/Fast-CDR/tree/master/test) in the `fast-cdr` repository, specifically the `SimpleTest.cpp` seems to be testing all the features of the library.

### Public API Testing [4.ii]
    
There are several [tests](https://github.com/eProsima/Fast-CDR/tree/master/test) in the `fast-cdr` repository, specifically the `SimpleTest.cpp` seems to be testing all the functions/types/operations of the library.

### Coverage [4.iii]
    
Checking their [CI jobs](http://jenkins.eprosima.com:8080/), and the testing folders, there seems to be no automatic code coverage analysis being made or anything related to code coverage analysis.

Coverage checks should be made to consider this dependency to apply for a higher quality level.

### Performance [4.iv]
    
Checking their [tests folder](https://github.com/eProsima/Fast-CDR/tree/master/test), it is shown there are no performance tests available for this library. As the library provides mainly basic types, and a couple of serialization mechanisms it is probably not needed for this case to create performance tests.

### Linters and Static Analysis [4.v]

Based on the [CI jobs test results](http://jenkins.eprosima.com:8080/job/FastCDR%20Manual%20Linux/lastSuccessfulBuild/testReport/projectroot/test/), there seems there is no automatic static analysis being made on the code developed.

## Dependencies [5]

`fast-cdr` only depends on functions provided by the standard C++ definitions.

## Platform Support [6]
    
There is no explicit support for any platform in their [repository page](https://github.com/eProsima/Fast-CDR). However, their CI jobs are triggered for both Linux and Windows. MacOS is not tested directly, but as this library is a dependence for the `Fast-RTPS` software, and that one is tested in the three platforms, it is safe to assume this library is supported in MacOS as well.

##  Vulnerability Disclosure Policy [7.i]

`fast-cdr` does not have a Vulnerability Disclosure Policy.
