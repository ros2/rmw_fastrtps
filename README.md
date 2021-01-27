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

ROS 2 only allows for the configuration of certain middleware QoS (see [ROS 2 QoS policies](https://index.ros.org/doc/ros2/Concepts/About-Quality-of-Service-Settings/#qos-policies)).
In addition to ROS 2 QoS policies, `rmw_fastrtps` sets two more Fast DDS configurable parameters:

* History memory policy: `PREALLOCATED_WITH_REALLOC_MEMORY_MODE`
* Publication mode: `ASYNCHRONOUS_PUBLISH_MODE`

However, `rmw_fastrtps` offers the possibility to further configure Fast DDS:

* [Change publication mode](#change-publication-mode)
* [Full QoS configuration](#full-qos-configuration)

### Change publication mode

Fast DDS feats two different [publication modes](https://fast-dds.docs.eprosima.com/en/v2.1.0/fastdds/dds_layer/core/policy/eprosimaExtensions.html?highlight=synchronous#publishmodeqospolicykind): synchronous and asynchronous.
To learn more about the implications of choosing one mode over the other, please refer to [DDS: Asynchronous vs Synchronous Publishing](https://www.eprosima.com/index.php/resources-all/performance/dds-asynchronous-vs-synchronous-publishing):

`rmw_fastrtps` offers an easy way to change Fast DDS' publication mode without the need of defining a XML file. That is environment variable `RMW_FASTRTPS_PUBLICATION_MODE`.
The admissible values are:

* `ASYNCHRONOUS`: asynchronous publication mode.
Setting this mode implies that when the publisher invokes the write operation, the data is copied into a queue, a notification about the addition to the queue is performed, and control of the thread is returned to the user before the data is actually sent.
A background thread (asynchronous thread) is in turn in charge of consuming the queue and sending the data to every matched reader.
* `SYNCHRONOUS`: synchronous publication mode.
Setting this mode implies that the data is sent directly within the context of the user thread.
This entails that any blocking call occurring during the write operation would block the user thread, thus preventing the application with continuing its operation.
It is important to note that this mode typically yields higher throughput rates at lower latencies, since the notification and context switching between threads is not present.
* `AUTO`: let Fast DDS select the publication mode. This implies using the publication mode set in the XML file or, failing that, the default value set in Fast DDS (which currently is set to `SYNCHRONOUS`).

If `RMW_FASTRTPS_PUBLICATION_MODE` is not set, then both `rmw_fastrtps_cpp` and `rmw_fastrtps_dynamic_cpp` behave as if it were set to `ASYNCHRONOUS`.

### Full QoS configuration

Fast DDS QoS policies can be fully configured through a combination of the [rmw QoS profile] API, and the [Fast DDS XML] file's QoS elements. Configuration depends on the environment variable `RMW_FASTRTPS_USE_QOS_FROM_XML`.

1. ROS 2 QoS contained in [`rmw_qos_profile_t`](http://docs.ros2.org/latest/api/rmw/structrmw__qos__profile__t.html) are always honored, unless set to `*_SYSTEM_DEFAULT`.
In that case, XML values, or Fast DDS default values in the absence of XML ones, are applied.
Setting any QoS in `rmw_qos_profile_t` to something other than `*_SYSTEM_DEFAULT` entails that specifying it via XML files has no effect, since they do not override what was used to create the publisher, subscription, service, or client.
1. In order to modify the history memory policy or publication mode using XML files, environment variable `RMW_FASTRTPS_USE_QOS_FROM_XML` must be set to 1 (it is set to 0 by default).
This tells `rmw_fastrtps` that it should override both the history memory policy and the publication mode using the XML.
Bear in mind that setting this environment variable but not setting either of these policies in the XML results in Fast DDS' defaults configurations being used.

| RMW_FASTRTPS_USE_QOS_FROM_XML | [rmw QoS profile]         | [Fast DDS XML] QoS                          | [Fast DDS XML] [history memory policy] and [publication mode] |
| ----------------------------- | ------------------------- | ------------------------------------------- | ------------------------------------------------------------- |
| 0 (default)                   | Use default values        | Ignored - overridden by `rmw_qos_profile_t` | Ignored - overrided by `rmw_fastrtps`                         |
| 0 (default)                   | Set to non system default | Ignored - overridden by `rmw_qos_profile_t` | Ignored - overrided by `rmw_fastrtps`                         |
| 0 (default)                   | Set to system default     | Used                                        | Ignored - overrided by `rmw_fastrtps`                         |
| 1                             | Use default values        | Ignored - overridden by `rmw_qos_profile_t` | Used                                                          |
| 1                             | Set to non system default | Ignored - overridden by `rmw_qos_profile_t` | Used                                                          |
| 1                             | Set to system default     | Used                                        | Used                                                          |

[rmw QoS profile]: http://docs.ros2.org/latest/api/rmw/structrmw__qos__profile__t.html
[Fast DDS XML]: https://fast-dds.docs.eprosima.com/en/latest/fastdds/xml_configuration/xml_configuration.html
[history memory policy]: https://fast-dds.docs.eprosima.com/en/latest/fastdds/dds_layer/core/policy/eprosimaExtensions.html#rtpsendpointqos
[publication mode]: https://fast-dds.docs.eprosima.com/en/latest/fastdds/dds_layer/core/policy/eprosimaExtensions.html#publishmodeqospolicy

Note: Setting `RMW_FASTRTPS_USE_QOS_FROM_XML` to 1 effectively overrides whatever configuration was set with `RMW_FASTRTPS_PUBLICATION_MODE`.
Furthermore, If `RMW_FASTRTPS_USE_QOS_FROM_XML` is set to 1, and [history memory policy] or [publication mode] are not specified in the XML, then the Fast DDS' default configurations will be used:

* [history memory policy] : `PREALLOCATED_MEMORY_MODE`.
* [publication mode] : `SYNCHRONOUS_PUBLISH_MODE`.

There are two ways of telling a ROS 2 application which XML to use:

1. Placing your XML file in the running directory under the name `DEFAULT_FASTRTPS_PROFILES.xml`.
1. Setting environment variable `FASTRTPS_DEFAULT_PROFILES_FILE` to contain the path to your XML file (relative to the working directory).

To verify the actual QoS settings using rmw:

```cpp
// Create a publisher within a node with specific topic, type support, options, and QoS
rmw_publisher_t* rmw_publisher = rmw_create_publisher(node, type_support, topic_name, qos_profile, publisher_options);
// Check the actual QoS set on the publisher
rmw_qos_profile_t qos;
rmw_publisher_get_actual_qos(rmw_publisher, &qos);
```

#### Applying different profiles to different entities

`rmw_fastrtps` allows for the configuration of different entities with different QoS using the same XML file.
For doing so, `rmw_fastrtps` locates profiles in the XML based on topic names abiding to the following rules:

##### Creating publishers/subscriptions with different profiles

To configure a publisher/subscription, define a `<publisher>`/`<subscriber>` profile with attribute `profile_name=topic_name`, where topic name is the name of the topic before mangling, i.e. the topic name used to create the publisher/subscription.
If such profile is not defined, `rmw_fastrtps` attempts to load the `<publisher>`/`<subscriber>` profile with attribute `is_default_profile="true"`.

##### Creating services with different profiles

ROS 2 services contain a subscription for receiving requests, and a publisher to reply to them.
`rmw_fastrtps` allows for configuring each of these endpoints separately in the following manner:

1. To configure the request subscription, define a `<subscriber>` profile with attribute `profile_name=topic_name`, where topic name is the name of the service after mangling. For more information on name mangling, please refer to [Topic and Service name mapping to DDS](https://design.ros2.org/articles/topic_and_service_names.html).
If such profile is not defined, `rmw_fastrtps` attempts to load a `<subscriber>` profile with attribute `profile_name="service"`.
If neither of the previous profiles exist, `rmw_fastrtps` attempts to load the `<subscriber>` profile with attribute `is_default_profile="true"`.
1. To configure the reply publisher, define a `<publisher>` profile with attribute `profile_name=topic_name`, where topic name is the name of the service after mangling.
If such profile is not defined, `rmw_fastrtps` attempts to load a `<publisher>` profile with attribute `profile_name="service"`.
If neither of the previous profiles exist, `rmw_fastrtps` attempts to load the `<publisher>` profile with attribute `is_default_profile="true"`.

##### Creating clients with different profiles

ROS 2 clients contain a publisher to send requests, and a subscription to receive the service's replies.
`rmw_fastrtps` allows for configuring each of these endpoints separately in the following manner:

1. To configure the requests publisher, define a `<publisher>` profile with attribute `profile_name=topic_name`, where topic name is the name of the service after mangling.
If such profile is not defined, `rmw_fastrtps` attempts to load a `<publisher>` profile with attribute `profile_name="client"`.
If neither of the previous profiles exist, `rmw_fastrtps` attempts to load the `<publisher>` profile with attribute `is_default_profile="true"`.
1. To configure the reply subscription, define a `<subscriber>` profile with attribute `profile_name=topic_name`, where topic name is the name of the service after mangling.
If such profile is not defined, `rmw_fastrtps` attempts to load a `<subscriber>` profile with attribute `profile_name="client"`.
If neither of the previous profiles exist, `rmw_fastrtps` attempts to load the `<subscriber>` profile with attribute `is_default_profile="true"`.

#### Example

The following example configures Fast DDS to publish synchronously, and to have a pre-allocated history that can be expanded whenever it gets filled.

1. Create a Fast DDS XML file with:

    ```XML
    <?xml version="1.0" encoding="UTF-8"?>
    <dds xmlns="http://www.eprosima.com/XMLSchemas/fastRTPS_Profiles">
        <profiles>
            <!-- Default publisher profile -->
            <publisher profile_name="default publisher profile" is_default_profile="true">
                <qos>
                    <publishMode>
                        <kind>SYNCHRONOUS</kind>
                    </publishMode>
                </qos>
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </publisher>

            <!-- Publisher profile for topic helloworld -->
            <publisher profile_name="helloworld">
                <qos>
                    <publishMode>
                        <kind>SYNCHRONOUS</kind>
                    </publishMode>
                </qos>
            </publisher>

            <!-- Request subscriber profile for services -->
            <subscriber profile_name="service">
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </subscriber>

            <!-- Request publisher profile for clients -->
            <publisher profile_name="client">
                <qos>
                    <publishMode>
                        <kind>ASYNCHRONOUS</kind>
                    </publishMode>
                </qos>
            </publisher>

            <!-- Request subscriber profile for server of service "add_two_ints" -->
            <subscriber profile_name="rq/add_two_intsRequest">
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </subscriber>

            <!-- Reply subscriber profile for client of service "add_two_ints" -->
            <subscriber profile_name="rr/add_two_intsReply">
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

Quality Declarations for each package in this repository:

* [`rmw_fastrtps_shared_cpp`](rmw_fastrtps_shared_cpp/QUALITY_DECLARATION.md)
* [`rmw_fastrps_cpp`](rmw_fastrtps_cpp/QUALITY_DECLARATION.md)
* [`rmw_fastrtps_dynamic_cpp`](rmw_fastrtps_dynamic_cpp/QUALITY_DECLARATION.md)

Quality Declarations for the external dependencies of these packages can be found in:

* [Fast DDS Quality Declaration](https://github.com/eProsima/Fast-DDS/blob/master/QUALITY.md)
* [Fast CDR Quality Declaration](https://github.com/eProsima/Fast-CDR/blob/master/QUALITY.md)
* [`foonathan_memory` Quality Declaration](https://github.com/eProsima/Fast-DDS/blob/master/Quality_Declaration_foonathan_memory.md)
