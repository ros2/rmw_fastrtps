# ROS 2 Middleware Implementation for eProsima's Fast DDS

`rmw_fastrtps` is a [ROS 2](https://docs.ros.org/en/rolling) middleware implementation, providing an interface between ROS 2 and [eProsima's](https://www.eprosima.com/index.php) [Fast DDS](https://github.com/eProsima/Fast-DDS) middleware.

## Getting started

This implementation is available in all ROS 2 distributions, both from binaries and from sources.
You can specify Fast DDS as your ROS 2 middleware layer in two different ways:

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

ROS 2 only allows for the configuration of certain middleware features.
For example, see [ROS 2 QoS policies](https://docs.ros.org/en/rolling/Concepts/About-Quality-of-Service-Settings.html#qos-policies).
In addition to ROS 2 QoS policies, `rmw_fastrtps` sets the following Fast DDS configurable parameters:

* History memory policy: `PREALLOCATED_WITH_REALLOC_MEMORY_MODE`
* Publication mode: `SYNCHRONOUS_PUBLISH_MODE`
* Data Sharing: `OFF`

However, `rmw_fastrtps` offers the possibility to further configure Fast DDS:

* [Change publication mode](#change-publication-mode)
* [Full QoS configuration](#full-qos-configuration)
* [Change participant discovery options](#change-participant-discovery-options)
* [Enable Zero Copy Data Sharing](#enable-zero-copy-data-sharing)
* [Large data transfer over lossy network](#large-data-transfer-over-lossy-network)

### Change publication mode

Fast DDS features two different [publication modes](https://fast-dds.docs.eprosima.com/en/v2.1.0/fastdds/dds_layer/core/policy/eprosimaExtensions.html?highlight=synchronous#publishmodeqospolicykind): synchronous and asynchronous.
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

If `RMW_FASTRTPS_PUBLICATION_MODE` is not set, then both `rmw_fastrtps_cpp` and `rmw_fastrtps_dynamic_cpp` behave as if it were set to `SYNCHRONOUS`.

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
[datasharing]: https://fast-dds.docs.eprosima.com/en/latest/fastdds/transport/datasharing.html

Note: Setting `RMW_FASTRTPS_USE_QOS_FROM_XML` to 1 effectively overrides whatever configuration was set with `RMW_FASTRTPS_PUBLICATION_MODE`.
Furthermore, If `RMW_FASTRTPS_USE_QOS_FROM_XML` is set to 1, and [history memory policy] or [publication mode] are not specified in the XML, then the Fast DDS' default configurations will be used:

* [history memory policy] : `PREALLOCATED_MEMORY_MODE`.
* [publication mode] : `SYNCHRONOUS_PUBLISH_MODE`.
* [datasharing] : `AUTO`.

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

To configure a publisher/subscription, define a `<publisher>`/`<subscriber>` profile with attribute `profile_name=topic_name`, where topic name is the name of the topic prepended by the node namespace (which defaults to `""` if not specified), i.e. the node's namespace followed by topic name used to create the publisher/subscription.
Mind that topic names always start with `/` (it is added when creating the topic if not present), and that namespace and topic name are always separated by one `/`.
If such profile is not defined, `rmw_fastrtps` attempts to load the `<publisher>`/`<subscriber>` profile with attribute `is_default_profile="true"`.
The following table presents different combinations of node namespaces and user specified topic names, as well as the resulting topic names and the suitable profile names:

| User specified topic name | Node namespace | Final topic name | Profile name |
|-|-|-|-|
| `chatter` | DEFAULT (`""`) | `/chatter` | `/chatter` |
| `chatter` | `test_namespace` | `/test_namespace/chatter` | `/test_namespace/chatter` |
| `chatter` | `/test_namespace` | `/test_namespace/chatter` | `/test_namespace/chatter` |
| `/chatter` | `test_namespace` | `/chatter` | `/chatter` |
| `/chatter` | `/test_namespace` | `/chatter` | `/chatter` |


**IMPORTANT**: As shown in the table, node namespaces are NOT prepended to user specified topic names starting with `/`, a.k.a Fully Qualified Names (FQN).
For a complete description of topic name remapping please refer to [Remapping Names](http://design.ros2.org/articles/static_remapping.html).

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

The following example configures Fast DDS to publish synchronously, to have a pre-allocated history that can be expanded whenever it gets filled, and to use Data Sharing if possible.

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
                    <data_sharing>
                        <kind>AUTOMATIC</kind>
                    </data_sharing>
                </qos>
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </publisher>

            <!-- Default subscriber profile -->
            <subscriber profile_name="default subscriber profile" is_default_profile="true">
                <qos>
                    <data_sharing>
                        <kind>AUTOMATIC</kind>
                    </data_sharing>
                </qos>
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </subscriber>

            <!-- Publisher profile for topic helloworld -->
            <publisher profile_name="helloworld">
                <qos>
                    <publishMode>
                        <kind>SYNCHRONOUS</kind>
                    </publishMode>
                    <data_sharing>
                        <kind>AUTOMATIC</kind>
                    </data_sharing>
                </qos>
            </publisher>

            <!-- Request subscriber profile for services -->
            <subscriber profile_name="service">
                <qos>
                    <data_sharing>
                        <kind>AUTOMATIC</kind>
                    </data_sharing>
                </qos>
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </subscriber>

            <!-- Request publisher profile for clients -->
            <publisher profile_name="client">
                <qos>
                    <publishMode>
                        <kind>ASYNCHRONOUS</kind>
                    </publishMode>
                    <data_sharing>
                        <kind>AUTOMATIC</kind>
                    </data_sharing>
                </qos>
            </publisher>

            <!-- Request subscriber profile for server of service "add_two_ints" -->
            <subscriber profile_name="rq/add_two_intsRequest">
                <qos>
                    <data_sharing>
                        <kind>AUTOMATIC</kind>
                    </data_sharing>
                </qos>
                <historyMemoryPolicy>PREALLOCATED_WITH_REALLOC</historyMemoryPolicy>
            </subscriber>

            <!-- Reply subscriber profile for client of service "add_two_ints" -->
            <subscriber profile_name="rr/add_two_intsReply">
                <qos>
                    <data_sharing>
                        <kind>AUTOMATIC</kind>
                    </data_sharing>
                </qos>
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

### Change participant discovery options

ROS 2 allows controlling participant discovery with two environment variables: `ROS_AUTOMATIC_DISCOVERY_RANGE` and `ROS_STATIC_PEERS`.
Full configuration of participant discovery can also be set with XML files; however, the ROS specific environment variables should be disabled to prevent them from interfering.
Set `ROS_AUTOMATIC_DISCOVERY_RANGE` to the value `SYSTEM_DEFAULT` to disable both ROS specific environment variables.
See more details for [Improved Dynamic Discovery](https://docs.ros.org/en/rolling/Tutorials/Advanced/Improved-Dynamic-Discovery.html).

### Enable Zero Copy Data Sharing

ROS 2 provides [Loaned Messages](https://design.ros2.org/articles/zero_copy.html) that allow the user application to loan the messages memory from the RMW implementation to eliminate the data copy between the ROS 2 application and the RMW implementation.
Furthermore, `rmw_fastrtps`, through Fast DDS, provides both a [Shared Memory Transport](https://fast-dds.docs.eprosima.com/en/latest/fastdds/transport/shared_memory/shared_memory.html) and [Data Sharing delivery mechanism](https://fast-dds.docs.eprosima.com/en/latest/fastdds/transport/datasharing.html) to speed up the intra-host communication.
Combining these two features (message loaning and Data Sharing), it is possible to achieve a zero-copy message delivery pipeline, thus bringing significant performance improvements to ROS 2 application.

By default, both `rmw_fastrtps_cpp` and `rmw_fastrtps_dynamic_cpp` use [Shared Memory Transport](https://fast-dds.docs.eprosima.com/en/latest/fastdds/transport/shared_memory/shared_memory.html) for intra-host communication, along with network based transports (UDPv4) for inter-host message delivery.

In order to achieve a Zero Copy message delivery, applications need to both enable Fast DDS Data Sharing mechanism, and use the [Loaned Messages](https://design.ros2.org/articles/zero_copy.html) API:

1. To enable Loaned Messages in `Iron Irwini` or later, the only requirement is for the data type to be [Plain Old Data](https://en.wikipedia.org/wiki/Passive_data_structure).
   For `Humble Hawksbill`, in addition to POD types, enabling Fast DDS Data Sharing is also required.
1. To enable Fast DDS Data Sharing delivery mechanism, the following XML profiles need to be loaded, and environment variable `RMW_FASTRTPS_USE_QOS_FROM_XML` needs to be set to 1 (see [Full QoS configuration](#full-qos-configuration))

    ```xml
    <?xml version="1.0" encoding="UTF-8" ?>
    <profiles xmlns="http://www.eprosima.com/XMLSchemas/fastRTPS_Profiles">

    <!-- Default publisher profile -->
    <publisher profile_name="default publisher profile" is_default_profile="true">
        <qos>
        <data_sharing>
            <kind>AUTOMATIC</kind>
        </data_sharing>
        </qos>
    </publisher>

    <!-- Default subscription profile -->
    <subscriber profile_name="default subscription profile" is_default_profile="true">
        <qos>
        <data_sharing>
            <kind>AUTOMATIC</kind>
        </data_sharing>
        </qos>
    </subscriber>
    </profiles>
    ```

### Large data transfer over lossy network

Out of the box, Fast DDS uses UDPv4 for the data communication over the network.
Although UDP has its own merit for realtime communications, with many applications relying on UDP, depending on application requirements, a more reliable network transmission may be needed.
Such cases included but are not limited to sending large data samples over lossy networks, where TCP's builtin reliability and flow control tend to perform better.

Because of this reason, Fast DDS provides the possibility to modify its builtin transports via an environmental variable `FASTDDS_BUILTIN_TRANSPORTS`, allowing for easily changing the transport layer to TCP when needed:

```bash
export FASTDDS_BUILTIN_TRANSPORTS=LARGE_DATA
```

This `LARGE_DATA` mode adds a TCP transport for data communication, restricting the use of the UDP transport to the first part of the discovery process, thus achieving a reliable transmission with automatic discovery capabilities.
This will improve the transmission of large data samples over lossy networks.

> [!NOTE]
> The environmental variable needs to be set on both publisher and subscription side.

For more information, please refer to [FASTDDS_BUILTIN_TRANSPORTS](https://fast-dds.docs.eprosima.com/en/latest/fastdds/env_vars/env_vars.html#fastdds-builtin-transports).

## Quality Declaration files

Quality Declarations for each package in this repository:

* [`rmw_fastrtps_shared_cpp`](rmw_fastrtps_shared_cpp/QUALITY_DECLARATION.md)
* [`rmw_fastrps_cpp`](rmw_fastrtps_cpp/QUALITY_DECLARATION.md)
* [`rmw_fastrtps_dynamic_cpp`](rmw_fastrtps_dynamic_cpp/QUALITY_DECLARATION.md)

Quality Declarations for the external dependencies of these packages can be found in:

* [Fast DDS Quality Declaration](https://github.com/eProsima/Fast-DDS/blob/master/QUALITY.md)
* [Fast CDR Quality Declaration](https://github.com/eProsima/Fast-CDR/blob/master/QUALITY.md)
* [`foonathan_memory` Quality Declaration](https://github.com/eProsima/Fast-DDS/blob/master/Quality_Declaration_foonathan_memory.md)
