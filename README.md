# ROS 2 Middleware Implementation for eProsima's Fast DDS

`rmw_fastrtps` constitutes __[ROS 2](https://index.ros.org/doc/ros2/) default middleware implementation__, providing an interface between ROS 2 and [eProsima's](https://www.eprosima.com/index.php) [Fast DDS](https://github.com/eProsima/Fast-DDS) middleware.

## Getting started
This implementation is available in all ROS 2 distributions, both from binaries and from sources.
You do not need to do anything in order to use Fast DDS as your ROS 2 middleware layer (since it is the default implementation).
However, you can still specify it in two different ways:

1. Exporting `RMW_IMPLEMENTATION` environment variable:
    ```bash
    export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
    ```
1. When launching your ROS 2 application:
    ```bash
    RMW_IMPLEMENTATION=rmw_fastrtps_cpp ros2 run <your_package> <your application>
    ```

## Two different RMW implementations

`rmw_fastrtps` actually provides not one but two different ROS 2 middleware implementations, both of them using Fast DDS as middleware layer: `rmw_fastrtps_cpp` and `rmw_fastrtps_dynamic_cpp` (note that directory `rmw_fastrtps_shared_cpp` just contains the code that the two implementations share, and does not constitute a layer on its own).

The main difference between the two is that `rmw_fastrtps_dynamic_cpp` uses introspection typesupport at run time to decide on the serialization/deserialization mechanism.
On the other hand, `rmw_fastrtps_cpp` uses its own typesupport, which generates the mapping for each message type at build time.

Mind that the default ROS 2 RMW implementation is `rmw_fastrtps_cpp`.
You can however set it to `rmw_fastrtps_dynamic_cpp` using the environment variable `RMW_IMPLEMENTATION` as described above.

## Advance usage

`rmw_fastrtps` sets some of the Fast DDS configurable parameters:
* History memory policy: `PREALLOCATED_WITH_REALLOC_MEMORY_MODE`
* Publication mode: `ASYNCHRONOUS_PUBLISH_MODE`

However, it is possible to fully configure Fast DDS (including the history memory policy and the publication mode) using an XML file as described in [Fast DDS documentation](https://fast-dds.docs.eprosima.com/en/latest/fastdds/xml_configuration/xml_configuration.html).
Then, you just need to set environment variable `RMW_FASTRTPS_USE_QOS_FROM_XML` to 1 (it is set to 0 by default).
This tells `rmw_fastrtps` that it should not override neither the history memory policy nor the publication mode.

You have two ways of telling you ROS 2 application which XML to use:
1. Placing your XML file in the running directory under the name `DEFAULT_FASTRTPS_PROFILES.xml`.
2. Setting environment variable `FASTRTPS_DEFAULT_PROFILES_FILE` to your XML file.

## Example

The following example configures Fast DDS to publish synchronously, and to have a pre-allocated history that can be expanded whenever it gets filled.

1. Create a Fast DDS XML file with:

    ```XML
    <?xml version="1.0" encoding="UTF-8"?>
    <dds xmlns="http://www.eprosima.com/XMLSchemas/fastRTPS_Profiles">
        <profiles>
            <publisher profile_name="publisher profile" is_default_profile="true">
                <qos>
                    <publishMode>
                        <kind>SYNCHRONOUS</kind>
                    </publishMode>
                </qos>
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </publisher>

            <subscriber profile_name="subscriber profile" is_default_profile="true">
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </subscriber>
        </profiles>
    </dds>
    ```

1. Run the talker/listener ROS 2 demo:
    1. In one terminal

        ```bash
        FASTRTPS_DEFAULT_PROFILES_FILE=<path_to_xml_file> RMW_FASTRTPS_USE_QOS_FROM_XML=1 RMW_IMPLEMENTATION=rmw_fastrtps_cpp ros2 run demo_nodes_cpp talker
        ```

    1. In another terminal

        ```bash
        FASTRTPS_DEFAULT_PROFILES_FILE=<path_to_xml_file> RMW_FASTRTPS_USE_QOS_FROM_XML=1 RMW_IMPLEMENTATION=rmw_fastrtps_cpp ros2 run demo_nodes_cpp listener
        ```

## Quality Declaration files

Quality Declarations for the external dependencies of these packages can be found in:

* [Fast DDS Quality Declaration](https://github.com/eProsima/Fast-DDS/blob/master/QUALITY.md)
* [Fast CDR Quality Declaration](https://github.com/eProsima/Fast-CDR/blob/master/QUALITY.md)
* [`foonathan_memory` Quality Declaration](https://github.com/eProsima/Fast-DDS/blob/master/Quality_Declaration_foonathan_memory.md)  