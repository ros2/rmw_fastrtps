This document is a declaration of software quality for the `rmw_fastrtps_cpp` package, based on the guidelines in [REP-2004](https://www.ros.org/reps/rep-2004.html).

# `rmw_fastrtps_cpp` Quality Declaration

The package `rmw_fastrtps_cpp` claims to be in the **Quality Level 4** category.

Below are the rationales, notes, and caveats for this claim, organized by each requirement listed in the [Package Requirements for Quality Level 4 in REP-2004](https://www.ros.org/reps/rep-2004.html).

## Version Policy [1]

### Version Scheme [1.i]

`rmw_fastrtps_cpp` uses `semver` according to the recommendation for ROS Core packages in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#versioning).

### Version Stability [1.ii]

`rmw_fastrtps_cpp` is not yet at a stable version, i.e. `>= 1.0.0`.

### Public API Declaration [1.iii]

All symbols in the installed headers are considered part of the public API.

All installed headers are in the `include` directory of the package, headers in any other folders are not installed and considered private.

### API Stability Within a Released ROS Distribution [1.iv]/[1.vi]

`rmw_fastrtps_cpp` will not break public API within a released ROS distribution, i.e. no major releases once the ROS distribution is released.

### ABI Stability Within a Released ROS Distribution [1.v]/[1.vi]

`rmw_fastrtps_cpp` contains C and C++ code and therefore must be concerned with ABI stability, and will maintain ABI stability within a ROS distribution.

## Change Control Process [2]

`rmw_fastrtps_cpp` follows the recommended guidelines for ROS Core packages in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#package-requirements).

### Change Requests [2.i]

This package requires that all changes occur through a pull request.

### Contributor Origin [2.ii]

This package uses DCO as its confirmation of contributor origin policy. More information can be found in [CONTRIBUTING](../CONTRIBUTING.md).

### Peer Review Policy [2.iii]

Following the recommended guidelines for ROS Core packages, all pull requests must have at least 1 peer review.

### Continuous Integration [2.iv]

All pull request must pass CI on all [tier 1 platforms](https://www.ros.org/reps/rep-2000.html#support-tiers)

### Documentation Policy [2.v]

All pull requests must resolve related documentation changes before merging.

## Documentation [3]

### Feature Documentation [3.i]

Some of the `rmw_fastrtps_cpp` features are documented on the repository level [README](../README.md).
Much of Fast RTPS itself has feature documentation [hosted publicly](https://fast-rtps.docs.eprosima.com/en/latest).

### Public API Documentation [3.ii]

Most of `rmw_fastrtps_cpp` has embedded API documentation. It is not yet hosted publicly.

### License [3.iii]

The license for `rmw_fastrtps_cpp` is Apache 2.0, and a summary is in each source file, the type is declared in the [package.xml](package.xml) manifest file, and a full copy of the license is in the [LICENSE](../LICENSE) file.

There is an automated test which runs a linter that ensures each file has a license statement.

Most recent test results can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/1525/testReport/rmw_fastrtps_cpp/copyright/)

### Copyright Statements [3.iv]

The copyright holders each provide a statement of copyright in each source code file in `rmw_fastrtps_cpp`.

There is an automated test which runs a linter that ensures each file has at least one copyright statement.

The results of the test can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/1525/testReport/rmw_fastrtps_cpp/copyright/).

## Testing [4]

### Feature Testing [4.i]

All `rmw_fastrtps_cpp` public features are ROS middleware features. Integration and system tests up the stack, such as those found in [`test_rclcpp`](https://github.com/ros2/system_tests/tree/master/test_rclcpp) and [`test_communication`](https://github.com/ros2/system_tests/tree/master/test_communication) packages, provide coverage. Nightly CI jobs in [`ci.ros2.org`](https://ci.ros2.org/) and [`build.ros2.org`](https://build.ros2.org/), where `rmw_fastrtps_cpp` is the default `rmw` implementation, thoroughly exercise this middleware.

### Public API Testing [4.ii]

There is no API testing in `rmw_fastrtps_cpp`.

### Coverage [4.iii]

`rmw_fastrtps_cpp` does not currently track test coverage.

### Performance [4.iv]

`rmw_fastrtps_cpp` does not currently have performance tests.

### Linters and Static Analysis [4.v]

`rmw_fastrtps_cpp` uses and passes all the standard linters and static analysis tools for a C++ package as described in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#linters).

Results of the nightly linter tests can be found [here](https://ci.ros2.org/view/nightly/job/nightly_linux_release/1525/testReport/rmw_fastrtps_cpp).

## Dependencies [5]

### Direct Runtime ROS Dependencies [5.i]/[5.ii]

`rmw_fastrtps_cpp` has the following runtime ROS dependencies:
* `fastrtps_cmake_module`
* `rcutils`
* `rmw`
* `rmw_dds_common`
* `rmw_fastrtps_shared_cpp`
* `rosidl_runtime_c`
* `rosidl_runtime_cpp`
* `rosidl_typesupport_fastrtps_c`
* `rosidl_typesupport_fastrtps_cpp`

It has several "buildtool" dependencies, which do not affect the resulting quality of the package, because they do not contribute to the public library API.
It also has several test dependencies, which do not affect the resulting quality of the package, because they are only used to build and run the test code.

### Direct Runtime Non-ROS Dependencies [5.iii]

`rmw_fastrtps_cpp` has the following runtime non-ROS dependencies.
* `fastcdr`
* `fastrtps`

## Platform Support [6]

`rmw_fastrtps_cpp` supports all of the tier 1 platforms as described in [REP-2000](https://www.ros.org/reps/rep-2000.html#support-tiers), and tests each change against all of them.

Currently nightly results can be seen here:
* [linux-aarch64_release](https://ci.ros2.org/view/nightly/job/nightly_linux-aarch64_release/lastBuild/testReport/rmw_fastrtps_cpp/)
* [linux_release](https://ci.ros2.org/view/nightly/job/nightly_linux_release/lastBuild/testReport/rmw_fastrtps_cpp/)
* [mac_osx_release](https://ci.ros2.org/view/nightly/job/nightly_osx_release/lastBuild/testReport/rmw_fastrtps_cpp/)
* [windows_release](https://ci.ros2.org/view/nightly/job/nightly_win_rel/lastBuild/testReport/rmw_fastrtps_cpp/)

## Vulnerability Disclosure Policy [7.i]

This package does not yet have a Vulnerability Disclosure Policy
