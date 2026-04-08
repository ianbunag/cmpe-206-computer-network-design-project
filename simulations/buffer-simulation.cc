#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include <iostream>
#include <map>
#include <string>
#include "../constants.h"

using namespace ns3;

namespace {
    std::map<std::string, std::string> configs = {
        {"Small_2", "2p"},
        {"Small_10", "10p"},
        {"Normal_50", "50p"},
        {"Large_100", "100p"},
        {"Large_500", "500p"}
    };

    const std::string DATA_RATE_10MBPS = "10Mbps";
    constexpr double SIMULATION_START_TIME = 1.0;
    constexpr double SIMULATION_STOP_TIME = 20.0;
    const uint32_t ON_TIME = 20;
    const uint32_t OFF_TIME = 0;
    const UintegerValue SIZE_2_MB = 2 * 1024 * 1024;
    const UintegerValue SIZE_1_KB = 1024;

    const std::string filePrefix = "buffer_simulation_";

    struct Topology {
        NodeContainer nodeContainerSources,
            nodeContainerRouters,
            nodeContainerDestinations;
        PointToPointHelper pointToPointHelper,
            pointToPointHelper_r0_r1;
        NetDeviceContainer netDeviceContainer_s0_r0,
            netDeviceContainer_s1_r0,
            netDeviceContainer_r0_r1,
            netDeviceContainer_r1_d0,
            netDeviceContainer_r1_d1;
        InternetStackHelper internetStackHelper;
        Ipv4AddressHelper ipv4AddressHelper;
        Ipv4InterfaceContainer ipv4InterfaceContainer_s0_r0,
            ipv4InterfaceContainer_s1_r0,
            ipv4InterfaceContainer_r0_r1,
            ipv4InterfaceContainer_r1_d0,
            ipv4InterfaceContainer_r1_d1;
        MobilityHelper mobility;
    };

    struct Monitors {
        FlowMonitorHelper flowMonitorHelper;
        Ptr<FlowMonitor> flowMonitor;
        std::unique_ptr<AnimationInterface> animationInterface;
    };

    void runBufferSimulationWithDefaultTopology(
        std::string testName,
        std::string queueSize,
        std::function<void(Topology&)> configureScene,
        std::function<void(Monitors&, Topology&)> configureMonitors
    ) {
        Topology t;

        t.nodeContainerSources.Create(2);
        t.nodeContainerRouters.Create(2);
        t.nodeContainerDestinations.Create(2);

        t.pointToPointHelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
        t.pointToPointHelper.SetChannelAttribute("Delay", StringValue("1ms"));

        t.pointToPointHelper_r0_r1.SetDeviceAttribute("DataRate", StringValue(DATA_RATE_10MBPS));
        t.pointToPointHelper_r0_r1.SetChannelAttribute("Delay", StringValue("10ms"));
        t.pointToPointHelper_r0_r1.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize(queueSize)));

        t.netDeviceContainer_s0_r0 = t.pointToPointHelper.Install(
            t.nodeContainerSources.Get(0),
            t.nodeContainerRouters.Get(0)
        );
        t.netDeviceContainer_s1_r0 = t.pointToPointHelper.Install(
            t.nodeContainerSources.Get(1),
            t.nodeContainerRouters.Get(0)
        );
        t.netDeviceContainer_r0_r1 = t.pointToPointHelper_r0_r1.Install(
            t.nodeContainerRouters.Get(0),
            t.nodeContainerRouters.Get(1)
        );
        t.netDeviceContainer_r1_d0 = t.pointToPointHelper.Install(
            t.nodeContainerRouters.Get(1),
            t.nodeContainerDestinations.Get(0)
        );
        t.netDeviceContainer_r1_d1 = t.pointToPointHelper.Install(
            t.nodeContainerRouters.Get(1),
            t.nodeContainerDestinations.Get(1)
        );

        t.internetStackHelper.Install(t.nodeContainerSources);
        t.internetStackHelper.Install(t.nodeContainerRouters);
        t.internetStackHelper.Install(t.nodeContainerDestinations);

        t.ipv4AddressHelper.SetBase("10.1.1.0", "255.255.255.0");
        t.ipv4InterfaceContainer_s0_r0 = t.ipv4AddressHelper.Assign(t.netDeviceContainer_s0_r0);

        t.ipv4AddressHelper.SetBase("10.1.2.0", "255.255.255.0");
        t.ipv4InterfaceContainer_s1_r0 = t.ipv4AddressHelper.Assign(t.netDeviceContainer_s1_r0);

        t.ipv4AddressHelper.SetBase("10.1.3.0", "255.255.255.0");
        t.ipv4InterfaceContainer_r0_r1 = t.ipv4AddressHelper.Assign(t.netDeviceContainer_r0_r1);

        t.ipv4AddressHelper.SetBase("10.1.4.0", "255.255.255.0");
        t.ipv4InterfaceContainer_r1_d0 = t.ipv4AddressHelper.Assign(t.netDeviceContainer_r1_d0);

        t.ipv4AddressHelper.SetBase("10.1.5.0", "255.255.255.0");
        t.ipv4InterfaceContainer_r1_d1 = t.ipv4AddressHelper.Assign(t.netDeviceContainer_r1_d1);

        Ipv4GlobalRoutingHelper::PopulateRoutingTables();

        t.mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue(0.0),
                                       "MinY", DoubleValue(0.0),
                                       "DeltaX", DoubleValue(100.0),
                                       "DeltaY", DoubleValue(100.0),
                                       "GridWidth", UintegerValue(3),
                                       "LayoutType", StringValue("RowFirst"));
        t.mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        t.mobility.Install(t.nodeContainerSources);
        t.mobility.Install(t.nodeContainerRouters);
        t.mobility.Install(t.nodeContainerDestinations);

        configureScene(t);

        Monitors m;

        m.flowMonitor = m.flowMonitorHelper.InstallAll();
        std::string animationFileName = ANIMATIONS_DIRECTORY + "/" + filePrefix + testName + "_animation.xml";
        m.animationInterface = std::make_unique<AnimationInterface>(animationFileName);
        m.animationInterface->SetMaxPktsPerTraceFile(1000000);

        m.animationInterface->SetConstantPosition(t.nodeContainerSources.Get(0), 0, 25);
        m.animationInterface->SetConstantPosition(t.nodeContainerSources.Get(1), 0, 75);
        m.animationInterface->SetConstantPosition(t.nodeContainerRouters.Get(0), 50, 50);
        m.animationInterface->SetConstantPosition(t.nodeContainerRouters.Get(1), 150, 50);
        m.animationInterface->SetConstantPosition(t.nodeContainerDestinations.Get(0), 200, 25);
        m.animationInterface->SetConstantPosition(t.nodeContainerDestinations.Get(1), 200, 75);

        m.animationInterface->UpdateNodeDescription(t.nodeContainerSources.Get(0), "Source 0");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerSources.Get(1), "Source 1");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerRouters.Get(0), "Router 0");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerRouters.Get(1), "Router 1");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerDestinations.Get(0), "Destination 1");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerDestinations.Get(1), "Destination 1");

        std::string baselineLinkDescription = "Baseline link (Rate: 100Mbps, Delay: 1ms)";
        std::string bottleneckLinkDescription = "Bottleneck link (Rate: 10Mbps, Delay: 10ms, Queue size: " + queueSize + ")";
        m.animationInterface->UpdateLinkDescription(
            t.nodeContainerSources.Get(0),
            t.nodeContainerRouters.Get(0),
            baselineLinkDescription
        );
        m.animationInterface->UpdateLinkDescription(
            t.nodeContainerSources.Get(1),
            t.nodeContainerRouters.Get(0),
            baselineLinkDescription
        );
        m.animationInterface->UpdateLinkDescription(
            t.nodeContainerRouters.Get(0),
            t.nodeContainerRouters.Get(1),
            bottleneckLinkDescription
        );
        m.animationInterface->UpdateLinkDescription(
            t.nodeContainerRouters.Get(1),
            t.nodeContainerDestinations.Get(0),
            baselineLinkDescription
        );
        m.animationInterface->UpdateLinkDescription(
            t.nodeContainerRouters.Get(1),
            t.nodeContainerDestinations.Get(1),
            baselineLinkDescription
        );

        configureMonitors(m, t);

        Simulator::Stop(Seconds(SIMULATION_STOP_TIME));
        Simulator::Run();
        std::string flowFileName = MONITORS_DIRECTORY + "/" + filePrefix + testName + "_flow.xml";
        m.flowMonitor->SerializeToXmlFile(flowFileName, true, true);
        Simulator::Destroy();
    }

    /**
     * TCP simulation.
     */
    void runBufferSimulationTcp(std::string testName, std::string queueSize) {
        NS_LOG_UNCOND("Starting runBufferSimulationTcp test" << testName << " with queue size: " << queueSize);

        Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
        Config::SetDefault("ns3::TcpSocket::SndBufSize", SIZE_2_MB);
        Config::SetDefault("ns3::TcpSocket::RcvBufSize", SIZE_2_MB);

        runBufferSimulationWithDefaultTopology(
            testName + "_tcp_only",
            queueSize,
            [](Topology& t) {
                BulkSendHelper bulkSendHelper_d0("ns3::TcpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d0.GetAddress(1), 9));
                bulkSendHelper_d0.SetAttribute("MaxBytes", UintegerValue(0)); // unlimited
                ApplicationContainer applicationContainer_s0 = bulkSendHelper_d0.Install(t.nodeContainerSources.Get(0));
                applicationContainer_s0.Start(Seconds(SIMULATION_START_TIME));
                applicationContainer_s0.Stop(Seconds(SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d0("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
                ApplicationContainer applicationContainer_d0 = packetSinkHelper_d0.Install(t.nodeContainerDestinations.Get(0));
                applicationContainer_d0.Start(Seconds(0.0));
                applicationContainer_d0.Stop(Seconds(SIMULATION_STOP_TIME));

                BulkSendHelper bulkSendHelper_d1("ns3::TcpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d1.GetAddress(1), 10));
                bulkSendHelper_d1.SetAttribute("MaxBytes", UintegerValue(0)); // unlimited
                ApplicationContainer applicationContainer_s1 = bulkSendHelper_d1.Install(t.nodeContainerSources.Get(1));
                applicationContainer_s1.Start(Seconds(SIMULATION_START_TIME));
                applicationContainer_s1.Stop(Seconds(SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10));
                ApplicationContainer applicationContainer_d1 = packetSinkHelper_d1.Install(t.nodeContainerDestinations.Get(1));
                applicationContainer_d1.Start(Seconds(0.0));
                applicationContainer_d1.Stop(Seconds(SIMULATION_STOP_TIME));      // No additional scene setup needed for TCP-only
            },
            [](Monitors& m, Topology& t) {
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(0), 0, 0, 255); // Blue
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(1), 0, 0, 255); // Blue
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(0), 0, 0, 255); // Blue
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(1), 0, 0, 255); // Blue
            }
        );

        NS_LOG_UNCOND("Ended runBufferSimulationTcp");
    }

    /**
     * UDP simulation.
     */
    void runBufferSimulationUdp(std::string testName, std::string queueSize) {
        NS_LOG_UNCOND("Starting runBufferSimulationUdp test " << testName << " with queue size: " << queueSize);

        runBufferSimulationWithDefaultTopology(
            testName + "_udp_only",
            queueSize,
            [](Topology& t) {
                OnOffHelper onOffHelper_d0("ns3::UdpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d0.GetAddress(1), 9));
                onOffHelper_d0.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(ON_TIME) + "]"));
                onOffHelper_d0.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(OFF_TIME) + "]"));
                onOffHelper_d0.SetAttribute("DataRate", StringValue(DATA_RATE_10MBPS));
                onOffHelper_d0.SetAttribute("PacketSize", SIZE_1_KB);
                ApplicationContainer applicationContainer_s0 = onOffHelper_d0.Install(t.nodeContainerSources.Get(0));
                applicationContainer_s0.Start(Seconds(SIMULATION_START_TIME));
                applicationContainer_s0.Stop(Seconds(SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d0("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
                ApplicationContainer applicationContainer_d0 = packetSinkHelper_d0.Install(t.nodeContainerDestinations.Get(0));
                applicationContainer_d0.Start(Seconds(0.0));
                applicationContainer_d0.Stop(Seconds(SIMULATION_STOP_TIME));

                OnOffHelper onOffHelper_d1("ns3::UdpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d1.GetAddress(1), 10));
                onOffHelper_d1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(ON_TIME) + "]"));
                onOffHelper_d1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(OFF_TIME) + "]"));
                onOffHelper_d1.SetAttribute("DataRate", StringValue(DATA_RATE_10MBPS));
                onOffHelper_d1.SetAttribute("PacketSize", SIZE_1_KB);
                ApplicationContainer applicationContainer_s1 = onOffHelper_d1.Install(t.nodeContainerSources.Get(1));
                applicationContainer_s1.Start(Seconds(SIMULATION_START_TIME));
                applicationContainer_s1.Stop(Seconds(SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10));
                ApplicationContainer applicationContainer_d1 = packetSinkHelper_d1.Install(t.nodeContainerDestinations.Get(1));
                applicationContainer_d1.Start(Seconds(0.0));
                applicationContainer_d1.Stop(Seconds(SIMULATION_STOP_TIME));
            },
            [](Monitors& m, Topology& t) {
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(0), 255, 0, 0); // Red
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(1), 255, 0, 0); // Red
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(0), 255, 0, 0); // Red
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(1), 255, 0, 0); // Red
            }
        );

        NS_LOG_UNCOND("Ended runBufferSimulationUdp");
    }

    /**
     * TCP (80%) + UDP (20%) simulation.
     */
    void runBufferSimulationMixed(std::string testName, std::string queueSize) {
        NS_LOG_UNCOND("Starting runBufferSimulationMixed test " << testName << " with queue size: " << queueSize);

        Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
        Config::SetDefault("ns3::TcpSocket::SndBufSize", SIZE_2_MB);
        Config::SetDefault("ns3::TcpSocket::RcvBufSize", SIZE_2_MB);

        runBufferSimulationWithDefaultTopology(
            testName + "_mixed",
            queueSize,
            [](Topology& t) {
                OnOffHelper onOffHelper_d0("ns3::TcpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d0.GetAddress(1), 9));
                onOffHelper_d0.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(ON_TIME) + "]"));
                onOffHelper_d0.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(OFF_TIME) + "]"));
                onOffHelper_d0.SetAttribute("DataRate", StringValue("8Mbps")); // 80% of total traffic
                onOffHelper_d0.SetAttribute("PacketSize", UintegerValue(SIZE_1_KB));
                ApplicationContainer applicationContainer_s0 = onOffHelper_d0.Install(t.nodeContainerSources.Get(0));
                applicationContainer_s0.Start(Seconds(SIMULATION_START_TIME));
                applicationContainer_s0.Stop(Seconds(SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d0("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
                ApplicationContainer applicationContainer_d0 = packetSinkHelper_d0.Install(t.nodeContainerDestinations.Get(0));
                applicationContainer_d0.Start(Seconds(0.0));
                applicationContainer_d0.Stop(Seconds(SIMULATION_STOP_TIME));

                OnOffHelper onOffHelper_d1("ns3::UdpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d1.GetAddress(1), 10));
                onOffHelper_d1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(ON_TIME) + "]"));
                onOffHelper_d1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(OFF_TIME) + "]"));
                onOffHelper_d1.SetAttribute("DataRate", StringValue("2Mbps")); // 20% of total traffic
                onOffHelper_d1.SetAttribute("PacketSize", SIZE_1_KB);
                ApplicationContainer applicationContainer_s1 = onOffHelper_d1.Install(t.nodeContainerSources.Get(1));
                applicationContainer_s1.Start(Seconds(SIMULATION_START_TIME));
                applicationContainer_s1.Stop(Seconds(SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10));
                ApplicationContainer applicationContainer_d1 = packetSinkHelper_d1.Install(t.nodeContainerDestinations.Get(1));
                applicationContainer_d1.Start(Seconds(0.0));
                applicationContainer_d1.Stop(Seconds(SIMULATION_STOP_TIME));
            },
            [](Monitors& m, Topology& t) {
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(0), 0, 0, 255); // Blue
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(1), 255, 0, 0); // Red
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(0), 0, 0, 255); // Blue
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(1), 255, 0, 0); // Red
            }
        );

        NS_LOG_UNCOND("Ended runBufferSimulationMixed");
    }
}

void runBufferSimulation() {
    NS_LOG_COMPONENT_DEFINE("BufferSimulation");

    std::cout << "Available Test Configurations:" << std::endl;
    for (const auto& config : configs) {
        std::cout << "  - " << config.first << " (Queue Size: " << config.second << ")" << std::endl;
    }

    std::string testName;
    std::cout << "\nEnter Test Name: ";
    std::getline(std::cin, testName);

    if (configs.find(testName) == configs.end()) {
        std::cerr << "Invalid test name.";
        return;
    }

    std::string queueSize = configs[testName];

    std::cout << "\nAvailable Traffic Modes:" << std::endl;
    std::cout << "  -  tcp_only" << std::endl;
    std::cout << "  -  udp_only" << std::endl;
    std::cout << "  -  mixed" << std::endl;

    std::string trafficMode;
    std::cout << "\nEnter Traffic Mode: ";
    std::getline(std::cin, trafficMode);

    if (trafficMode == "tcp_only") {
        runBufferSimulationTcp(testName, queueSize);
    } else if (trafficMode == "udp_only") {
        runBufferSimulationUdp(testName, queueSize);
    } else if (trafficMode == "mixed") {
        runBufferSimulationMixed(testName, queueSize);
    } else {
        std::cerr << "Invalid traffic mode." << std::endl;
        return;
    }
}