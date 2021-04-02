This document is a declaration of software quality for the `rmw_fastrtps_cpp` package, based on the guidelines in [REP-2004](https://www.ros.org/reps/rep-2004.html).

# `rmw_fastrtps_cpp` Quality Declaration

The package `rmw_fastrtps_cpp` claims to be in the **Quality Level 2** category.

Below are the rationales, notes, and caveats for this claim, organized by each requirement listed in the [Package Requirements for Quality Level 2 in REP-2004](https://www.ros.org/reps/rep-2004.html).

## Version Policy [1]

### Version Scheme [1.i]

`rmw_fastrtps_cpp` uses `semver` according to the recommendation for ROS Core packages in the [ROS 2 Developer Guide](https://docs.ros.org/en/foxy/Contributing/Developer-Guide.html#versioning).

### Version Stability [1.ii]

`rmw_fastrtps_cpp` is at a stable version, i.e. `>= 1.0.0`.
The current version can be found in its [package.xml](package.xml), and its change history can be found in its [CHANGELOG](CHANGELOG.rst).

### Public API Declaration [1.iii]

All symbols in the installed headers are considered part of the public API.

All installed headers are in the `include` directory of the package, headers in any other folders are not installed and considered private.

### API Stability Within a Released ROS Distribution [1.iv]/[1.vi]

`rmw_fastrtps_cpp` will not break public API within a released ROS distribution, i.e. no major releases once the ROS distribution is released.

### ABI Stability Within a Released ROS Distribution [1.v]/[1.vi]

`rmw_fastrtps_cpp` contains C and C++ code and therefore must be concerned with ABI stability, and will maintain ABI stability within a ROS distribution.

## Change Control Process [2]

`rmw_fastrtps_cpp` follows the recommended guidelines for ROS Core packages in the [ROS 2 Developer Guide](https://docs.ros.org/en/foxy/Contributing/Developer-Guide.html#quality-practices).

### Change Requests [2.i]

This package requires that all changes occur through a pull request.

### Contributor Origin [2.ii]

This package uses DCO as its confirmation of contributor origin policy. More information can be found in [CONTRIBUTING](../CONTRIBUTING.md).

### Peer Review Policy [2.iii]

Following the recommended guidelines for ROS Core packages, all pull requests must have at least 1 peer review.

### Continuous Integration [2.iv]

All pull request must pass CI on all [tier 1 platforms](https://www.ros.org/reps/rep-2000.html#support-tiers).

### Documentation Policy [2.v]

All pull requests must resolve related documentation changes before merging.

## Documentation [3]

### Feature Documentation [3.i]

Some of the `rmw_fastrtps_cpp` features are documented on the repository level [README](../README.md).
Much of Fast DDS itself has feature documentation [hosted publicly](https://fast-dds.docs.eprosima.com/en/latest/).

### Public API Documentation [3.ii]

Most of `rmw_fastrtps_cpp` has embedded API documentation. It is not yet hosted publicly.

### License [3.iii]

The license for `rmw_fastrtps_cpp` is Apache 2.0, and a summary is in each source file, the type is declared in the [package.xml](package.xml) manifest file, and a full copy of the license is in the [LICENSE](../LICENSE) file.

There is an automated test which runs a linter that ensures each file has a license statement.

Most recent test results can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rmw_fastrtps_cpp/copyright/)

### Copyright Statements [3.iv]

The copyright holders each provide a statement of copyright in each source code file in `rmw_fastrtps_cpp`.

There is an automated test which runs a linter that ensures each file has at least one copyright statement.

The results of the test can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rmw_fastrtps_cpp/copyright/).

## Testing [4]

### Feature Testing [4.i]

All `rmw_fastrtps_cpp` public features are ROS middleware features. Integration and system tests up the stack, such as those found in [`test_rclcpp`](https://github.com/ros2/system_tests/tree/foxy/test_rclcpp) and [`test_communication`](https://github.com/ros2/system_tests/tree/foxy/test_communication) packages, provide coverage. Nightly CI jobs in [`ci.ros2.org`](https://ci.ros2.org/) and [`build.ros2.org`](https://build.ros2.org/), where `rmw_fastrtps_cpp` is the default `rmw` implementation, thoroughly exercise this middleware.

### Public API Testing [4.ii]

There is no API testing in `rmw_fastrtps_cpp`.

### Coverage [4.iii]

`rmw_fastrtps_cpp` follows the recommendations for ROS Core packages in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#coverage), and opts to use branch coverage instead of line coverage.

This includes:

- tracking and reporting line coverage statistics
- achieving and maintaining a reasonable branch line coverage (90-100%)
- no lines are manually skipped in coverage calculations

Changes are required to make a best effort to keep or increase coverage before being accepted, but decreases are allowed if properly justified and accepted by reviewers.

Current coverage statistics can be viewed [here](https://ci.ros2.org/job/ci_linux_coverage/lastSuccessfulBuild/cobertura/src_ros2_rmw_fastrtps_rmw_fastrtps_cpp_src/).

This package claims to meet the coverage requirements for the current quality level, even though it doesn't have 95% line coverage.
The justification is that the only uncovered lines have to do with system resource exhaustion and Fast DDS internal failure.

A summary of how these statistics are calculated can be found in the [ROS 2 On-boarding guide](https://docs.ros.org/en/foxy/Contributing/Developer-Guide.html#note-on-coverage-runs).

### Performance [4.iv]

`rmw_fastrtps_cpp` does not currently have performance tests.

### Linters and Static Analysis [4.v]

`rmw_fastrtps_cpp` uses and passes all the standard linters and static analysis tools for a C++ package as described in the [ROS 2 Developer Guide](https://docs.ros.org/en/foxy/Contributing/Developer-Guide.html#linters-and-static-analysis).

Results of the nightly linter tests can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rmw_fastrtps_cpp).

## Dependencies [5]

### Direct Runtime ROS Dependencies [5.i]/[5.ii]

`rmw_fastrtps_cpp` has the following runtime ROS dependencies:
* `fastrtps_cmake_module`: [QUALITY DECLARATION](https://github.com/ros2/rosidl_typesupport_fastrtps/blob/foxy/fastrtps_cmake_module/QUALITY_DECLARATION.md)
* `rcutils`: [QUALITY DECLARATION](https://github.com/ros2/rcutils/blob/foxy/QUALITY_DECLARATION.md)
* `rmw`: [QUALITY DECLARATION](https://github.com/ros2/rmw/blob/foxy/rmw/QUALITY_DECLARATION.md)
* `rmw_dds_common`: [QUALITY DECLARATION](https://github.com/ros2/rmw_dds_common/blob/foxy/rmw_dds_common/QUALITY_DECLARATION.md)
* `rmw_fastrtps_shared_cpp`: [QUALITY DECLARATION](https://github.com/ros2/rmw_fastrtps/blob/foxy/rmw_fastrtps_shared_cpp/QUALITY_DECLARATION.md)
* `rosidl_runtime_c`: [QUALITY DECLARATION](https://github.com/ros2/rosidl/blob/foxy/rosidl_runtime_c/QUALITY_DECLARATION.md)
* `rosidl_runtime_cpp`: [QUALITY DECLARATION](https://github.com/ros2/rosidl/blob/foxy/rosidl_runtime_cpp/QUALITY_DECLARATION.md)
* `rosidl_typesupport_fastrtps_c`: [QUALITY DECLARATION](https://github.com/ros2/rosidl_typesupport_fastrtps/blob/foxy/rosidl_typesupport_fastrtps_c/QUALITY_DECLARATION.md)
* `rosidl_typesupport_fastrtps_cpp`: [QUALITY DECLARATION](https://github.com/ros2/rosidl_typesupport_fastrtps/blob/foxy/rosidl_typesupport_fastrtps_cpp/QUALITY_DECLARATION.md)

It has several "buildtool" dependencies, which do not affect the resulting quality of the package, because they do not contribute to the public library API.
It also has several test dependencies, which do not affect the resulting quality of the package, because they are only used to build and run the test code.

### Direct Runtime Non-ROS Dependencies [5.iii]

`rmw_fastrtps_cpp` has the following runtime non-ROS dependencies.
* `fastcdr`: *eProsima Fast CDR* claims to be Quality Level 2. For more information, please refer to its [QUALITY DECLARATION](https://github.com/eProsima/Fast-CDR/blob/master/QUALITY.md)
* `fastrtps`: *eProsima Fast DDS* claims to be Quality Level 2. For more information, please refer to its [QUALITY DECLARATION](https://github.com/eProsima/Fast-DDS/blob/2.0.x/QUALITY.md)

## Platform Support [6]

`rmw_fastrtps_cpp` supports all of the tier 1 platforms as described in [REP-2000](https://www.ros.org/reps/rep-2000.html#support-tiers), and tests each change against all of them.

Currently nightly results can be seen here:
* [linux-aarch64_release](https://ci.ros2.org/view/nightly/job/nightly_linux-aarch64_release/lastBuild/testReport/rmw_fastrtps_cpp/)
* [linux_release](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rmw_fastrtps_cpp/)
* [mac_osx_release](https://ci.ros2.org/view/nightly/job/nightly_osx_release/lastBuild/testReport/rmw_fastrtps_cpp/)
* [windows_release](https://ci.ros2.org/view/nightly/job/nightly_win_rel/lastBuild/testReport/rmw_fastrtps_cpp/)

## Security [7]

### Vulnerability Disclosure Policy [7.i]

This package conforms to the Vulnerability Disclosure Policy in [REP-2006](https://www.ros.org/reps/rep-2006.html).

# Current status Summary

The chart below compares the requirements in the REP-2004 with the current state of the `rmw_fastrtps_cpp` package.

|Number| Requirement| Current state |
|--|--|--|
|1| **Version policy** |---|
|1.i|Version Policy available | ✓ |
|1.ii|Stable version |✓ |
|1.iii|Declared public API|✓|
|1.iv|API stability policy|✓|
|1.v|ABI stability policy|✓|
|1.vi_|API/ABI stable within ros distribution|✓|
|2| **Change control process** |---|
|2.i| All changes occur on change request | ✓|
|2.ii| Contributor origin (DCO, CLA, etc) | ✓|
|2.iii| Peer review policy | ✓ |
|2.iv| CI policy for change requests | ✓ |
|2.v| Documentation policy for change requests | ✓ |
|3| **Documentation** | --- |
|3.i| Per feature documentation | ✓ |
|3.ii| Per public API item documentation | * |
|3.iii| Declared License(s) | ✓ |
|3.iv| Copyright in source files| ✓ |
|3.v.a| Quality declaration linked to README | ✓ |
|3.v.b| Centralized declaration available for peer review |✓|
|4| **Testing** | --- |
|4.i| Feature items tests | ✓ |
|4.ii| Public API tests | * |
|4.iii.a| Using coverage | ✓ |
|4.iii.a| Coverage policy | ✓ |
|4.iv.a| Performance tests (if applicable) | ☓ |
|4.iv.b| Performance tests policy| ✓ |
|4.v.a| Code style enforcement (linters)| ✓ |
|4.v.b| Use of static analysis tools | ✓ |
|5| **Dependencies** | --- |
|5.i| Must not have ROS lower level dependencies | ✓ |
|5.ii| Optional ROS lower level dependencies| ✓ |
|5.iii| Justifies quality use of non-ROS dependencies |✓|
|6| **Platform support** | --- |
|6.i| Support targets Tier1 ROS platforms| ✓ |
|7| **Security** | --- |
|7.i| Vulnerability Disclosure Policy | ✓ |
