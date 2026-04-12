#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include "../constants.h"

using namespace ns3;

namespace {
    std::map<std::string, std::string> l_configs = {
        {"10ms", "10ms"},
        {"50ms", "50ms"},
        {"100ms", "100ms"},
        {"200ms", "200ms"},
        {"400ms", "400ms"}
    };

    const std::string L_DATA_RATE_10MBPS = "10Mbps";
    const std::string L_DATA_RATE_5MBPS = "5Mbps";
    const std::string L_DATA_RATE_1MBPS = "1Mbps";
    constexpr double L_SIMULATION_START_TIME = 1.0;
    constexpr double L_SIMULATION_STOP_TIME = 40.0;
    const uint32_t L_ON_TIME = 40;
    const uint32_t L_OFF_TIME = 0;
    const UintegerValue L_SIZE_2_MB = 2 * 1024 * 1024;
    const UintegerValue L_SIZE_1_KB = 1024;
    const std::string l_filePrefix = "latency_simulation_";

    struct L_Topology {
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

    struct L_Monitors {
        FlowMonitorHelper flowMonitorHelper;
        Ptr<FlowMonitor> flowMonitor;
        std::unique_ptr<AnimationInterface> animationInterface;
    };

    void runLatencySimulationWithDefaultTopology(
        std::string testName,
        std::string bottleneckDelay,
        std::function<void(L_Topology&)> configureScene,
        std::function<void(L_Monitors&, L_Topology&)> configureMonitors
    ) {
        L_Topology t;

        t.nodeContainerSources.Create(2);
        t.nodeContainerRouters.Create(2);
        t.nodeContainerDestinations.Create(2);

        t.pointToPointHelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
        t.pointToPointHelper.SetChannelAttribute("Delay", StringValue("1ms"));

        t.pointToPointHelper_r0_r1.SetDeviceAttribute("DataRate", StringValue(L_DATA_RATE_10MBPS));
        t.pointToPointHelper_r0_r1.SetChannelAttribute("Delay", StringValue(bottleneckDelay));
        t.pointToPointHelper_r0_r1.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("50p")));

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

        Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel>();
        errorModel->SetAttribute("ErrorRate", DoubleValue(0.001)); // 0.1% Bit Error Rate
        t.netDeviceContainer_r0_r1.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));

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

        L_Monitors m;

        std::string pcapBaseFileName = MONITORS_DIRECTORY + "/" + l_filePrefix + testName + "_r0_r1";
        t.pointToPointHelper_r0_r1.EnablePcap(pcapBaseFileName, t.netDeviceContainer_r0_r1.Get(0), true);

        m.flowMonitor = m.flowMonitorHelper.InstallAll();
        std::string animationFileName = ANIMATIONS_DIRECTORY + "/" + l_filePrefix + testName + "_animation.xml";
        m.animationInterface = std::make_unique<AnimationInterface>(animationFileName);
        m.animationInterface->SetMaxPktsPerTraceFile(1000000);

        m.animationInterface->SetConstantPosition(t.nodeContainerSources.Get(0), 0, 25);
        m.animationInterface->SetConstantPosition(t.nodeContainerSources.Get(1), 0, 75);
        m.animationInterface->SetConstantPosition(t.nodeContainerRouters.Get(0), 50, 50);
        m.animationInterface->SetConstantPosition(t.nodeContainerRouters.Get(1), 150, 50);
        m.animationInterface->SetConstantPosition(t.nodeContainerDestinations.Get(0), 200, 25);
        m.animationInterface->SetConstantPosition(t.nodeContainerDestinations.Get(1), 200, 75);

        m.animationInterface->UpdateNodeColor(t.nodeContainerRouters.Get(0), 0, 255, 0); // Green
        m.animationInterface->UpdateNodeColor(t.nodeContainerRouters.Get(1), 0, 255, 0); // Green

        m.animationInterface->UpdateNodeDescription(t.nodeContainerSources.Get(0), "Source 0");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerSources.Get(1), "Source 1");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerRouters.Get(0), "Router 0");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerRouters.Get(1), "Router 1");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerDestinations.Get(0), "Destination 0");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerDestinations.Get(1), "Destination 1");

        std::string baselineLinkDescription = "Baseline link (Rate: 100Mbps, Delay: 1ms)";
        std::string bottleneckLinkDescription = "Bottleneck link (Rate: 10Mbps, Delay: " + bottleneckDelay + ", BER: 0.1%, Queue: 50p)";
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

        Simulator::Stop(Seconds(L_SIMULATION_STOP_TIME));
        Simulator::Run();
        std::string flowFileName = MONITORS_DIRECTORY + "/" + l_filePrefix + testName + "_flow.xml";
        m.flowMonitor->SerializeToXmlFile(flowFileName, true, true);
        Simulator::Destroy();
    }

    /**
     * TCP simulation.
     */
    void runLatencySimulationTcp(std::string testName, std::string bottleneckDelay) {
        NS_LOG_UNCOND("Starting runLatencySimulationTcp test " << testName << " with bottleneck delay: " << bottleneckDelay);

        Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
        Config::SetDefault("ns3::TcpSocket::SndBufSize", L_SIZE_2_MB);
        Config::SetDefault("ns3::TcpSocket::RcvBufSize", L_SIZE_2_MB);

        runLatencySimulationWithDefaultTopology(
            testName + "_tcp_only",
            bottleneckDelay,
            [](L_Topology& t) {
                BulkSendHelper bulkSendHelper_d0("ns3::TcpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d0.GetAddress(1), 9));
                bulkSendHelper_d0.SetAttribute("MaxBytes", UintegerValue(0)); // unlimited
                ApplicationContainer applicationContainer_s0 = bulkSendHelper_d0.Install(t.nodeContainerSources.Get(0));
                applicationContainer_s0.Start(Seconds(L_SIMULATION_START_TIME));
                applicationContainer_s0.Stop(Seconds(L_SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d0("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
                ApplicationContainer applicationContainer_d0 = packetSinkHelper_d0.Install(t.nodeContainerDestinations.Get(0));
                applicationContainer_d0.Start(Seconds(0.0));
                applicationContainer_d0.Stop(Seconds(L_SIMULATION_STOP_TIME));

                BulkSendHelper bulkSendHelper_d1("ns3::TcpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d1.GetAddress(1), 10));
                bulkSendHelper_d1.SetAttribute("MaxBytes", UintegerValue(0)); // unlimited
                ApplicationContainer applicationContainer_s1 = bulkSendHelper_d1.Install(t.nodeContainerSources.Get(1));
                applicationContainer_s1.Start(Seconds(L_SIMULATION_START_TIME));
                applicationContainer_s1.Stop(Seconds(L_SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10));
                ApplicationContainer applicationContainer_d1 = packetSinkHelper_d1.Install(t.nodeContainerDestinations.Get(1));
                applicationContainer_d1.Start(Seconds(0.0));
                applicationContainer_d1.Stop(Seconds(L_SIMULATION_STOP_TIME));
            },
            [](L_Monitors& m, L_Topology& t) {
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(0), 0, 0, 255); // Blue
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(1), 0, 0, 255); // Blue
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(0), 0, 0, 255); // Blue
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(1), 0, 0, 255); // Blue
            }
        );

        NS_LOG_UNCOND("Ended runLatencySimulationTcp");
    }

    /**
     * UDP simulation.
     */
    void runLatencySimulationUdp(std::string testName, std::string bottleneckDelay) {
        NS_LOG_UNCOND("Starting runLatencySimulationUdp test " << testName << " with bottleneck delay: " << bottleneckDelay);

        runLatencySimulationWithDefaultTopology(
            testName + "_udp_only",
            bottleneckDelay,
            [](L_Topology& t) {
                OnOffHelper onOffHelper_d0("ns3::UdpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d0.GetAddress(1), 9));
                onOffHelper_d0.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(L_ON_TIME) + "]"));
                onOffHelper_d0.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(L_OFF_TIME) + "]"));
                onOffHelper_d0.SetAttribute("DataRate", StringValue(L_DATA_RATE_5MBPS));
                onOffHelper_d0.SetAttribute("PacketSize", L_SIZE_1_KB);
                ApplicationContainer applicationContainer_s0 = onOffHelper_d0.Install(t.nodeContainerSources.Get(0));
                applicationContainer_s0.Start(Seconds(L_SIMULATION_START_TIME));
                applicationContainer_s0.Stop(Seconds(L_SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d0("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
                ApplicationContainer applicationContainer_d0 = packetSinkHelper_d0.Install(t.nodeContainerDestinations.Get(0));
                applicationContainer_d0.Start(Seconds(0.0));
                applicationContainer_d0.Stop(Seconds(L_SIMULATION_STOP_TIME));

                OnOffHelper onOffHelper_d1("ns3::UdpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d1.GetAddress(1), 10));
                onOffHelper_d1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(L_ON_TIME) + "]"));
                onOffHelper_d1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(L_OFF_TIME) + "]"));
                onOffHelper_d1.SetAttribute("DataRate", StringValue(L_DATA_RATE_5MBPS));
                onOffHelper_d1.SetAttribute("PacketSize", L_SIZE_1_KB);
                ApplicationContainer applicationContainer_s1 = onOffHelper_d1.Install(t.nodeContainerSources.Get(1));
                applicationContainer_s1.Start(Seconds(L_SIMULATION_START_TIME));
                applicationContainer_s1.Stop(Seconds(L_SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10));
                ApplicationContainer applicationContainer_d1 = packetSinkHelper_d1.Install(t.nodeContainerDestinations.Get(1));
                applicationContainer_d1.Start(Seconds(0.0));
                applicationContainer_d1.Stop(Seconds(L_SIMULATION_STOP_TIME));
            },
            [](L_Monitors& m, L_Topology& t) {
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(0), 255, 0, 0); // Red
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(1), 255, 0, 0); // Red
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(0), 255, 0, 0); // Red
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(1), 255, 0, 0); // Red
            }
        );

        NS_LOG_UNCOND("Ended runLatencySimulationUdp");
    }

    /**
     * TCP + UDP simulation (both flows competing).
     */
    void runLatencySimulationMixed(std::string testName, std::string bottleneckDelay) {
        NS_LOG_UNCOND("Starting runLatencySimulationMixed test " << testName << " with bottleneck delay: " << bottleneckDelay);

        Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
        Config::SetDefault("ns3::TcpSocket::SndBufSize", L_SIZE_2_MB);
        Config::SetDefault("ns3::TcpSocket::RcvBufSize", L_SIZE_2_MB);

        runLatencySimulationWithDefaultTopology(
            testName + "_mixed",
            bottleneckDelay,
            [](L_Topology& t) {
                BulkSendHelper bulkSendHelper_s0_d0("ns3::TcpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d0.GetAddress(1), 9));
                bulkSendHelper_s0_d0.SetAttribute("MaxBytes", UintegerValue(0)); // unlimited
                ApplicationContainer applicationContainer_s0_tcp = bulkSendHelper_s0_d0.Install(t.nodeContainerSources.Get(0));
                applicationContainer_s0_tcp.Start(Seconds(L_SIMULATION_START_TIME));
                applicationContainer_s0_tcp.Stop(Seconds(L_SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d0_tcp("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
                ApplicationContainer applicationContainer_d0_tcp = packetSinkHelper_d0_tcp.Install(t.nodeContainerDestinations.Get(0));
                applicationContainer_d0_tcp.Start(Seconds(0.0));
                applicationContainer_d0_tcp.Stop(Seconds(L_SIMULATION_STOP_TIME));

                OnOffHelper onOffHelper_s0_d1("ns3::UdpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d1.GetAddress(1), 10));
                onOffHelper_s0_d1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(L_ON_TIME) + "]"));
                onOffHelper_s0_d1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(L_OFF_TIME) + "]"));
                onOffHelper_s0_d1.SetAttribute("DataRate", StringValue(L_DATA_RATE_1MBPS));
                onOffHelper_s0_d1.SetAttribute("PacketSize", L_SIZE_1_KB);
                ApplicationContainer applicationContainer_s0_udp = onOffHelper_s0_d1.Install(t.nodeContainerSources.Get(0));
                applicationContainer_s0_udp.Start(Seconds(L_SIMULATION_START_TIME));
                applicationContainer_s0_udp.Stop(Seconds(L_SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d1_udp("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10));
                ApplicationContainer applicationContainer_d1_udp = packetSinkHelper_d1_udp.Install(t.nodeContainerDestinations.Get(1));
                applicationContainer_d1_udp.Start(Seconds(0.0));
                applicationContainer_d1_udp.Stop(Seconds(L_SIMULATION_STOP_TIME));

                OnOffHelper onOffHelper_s1_d0("ns3::UdpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d0.GetAddress(1), 11));
                onOffHelper_s1_d0.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(L_ON_TIME) + "]"));
                onOffHelper_s1_d0.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(L_OFF_TIME) + "]"));
                onOffHelper_s1_d0.SetAttribute("DataRate", StringValue(L_DATA_RATE_1MBPS));
                onOffHelper_s1_d0.SetAttribute("PacketSize", L_SIZE_1_KB);
                ApplicationContainer applicationContainer_s1_udp = onOffHelper_s1_d0.Install(t.nodeContainerSources.Get(1));
                applicationContainer_s1_udp.Start(Seconds(L_SIMULATION_START_TIME));
                applicationContainer_s1_udp.Stop(Seconds(L_SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d0_udp("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 11));
                ApplicationContainer applicationContainer_d0_udp = packetSinkHelper_d0_udp.Install(t.nodeContainerDestinations.Get(0));
                applicationContainer_d0_udp.Start(Seconds(0.0));
                applicationContainer_d0_udp.Stop(Seconds(L_SIMULATION_STOP_TIME));

                BulkSendHelper bulkSendHelper_s1_d1("ns3::TcpSocketFactory", InetSocketAddress(t.ipv4InterfaceContainer_r1_d1.GetAddress(1), 12));
                bulkSendHelper_s1_d1.SetAttribute("MaxBytes", UintegerValue(0)); // unlimited
                ApplicationContainer applicationContainer_s1_tcp = bulkSendHelper_s1_d1.Install(t.nodeContainerSources.Get(1));
                applicationContainer_s1_tcp.Start(Seconds(L_SIMULATION_START_TIME));
                applicationContainer_s1_tcp.Stop(Seconds(L_SIMULATION_STOP_TIME));

                PacketSinkHelper packetSinkHelper_d1_tcp("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 12));
                ApplicationContainer applicationContainer_d1_tcp = packetSinkHelper_d1_tcp.Install(t.nodeContainerDestinations.Get(1));
                applicationContainer_d1_tcp.Start(Seconds(0.0));
                applicationContainer_d1_tcp.Stop(Seconds(L_SIMULATION_STOP_TIME));
            },
            [](L_Monitors& m, L_Topology& t) {
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(0), 255, 0, 255); // Purple
                m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(1), 255, 0, 255); // Purple
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(0), 255, 0, 255); // Purple
                m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(1), 255, 0, 255); // Purple
            }
        );

        NS_LOG_UNCOND("Ended runLatencySimulationMixed");
    }
}

void runLatencySimulation() {
    NS_LOG_COMPONENT_DEFINE("LatencySimulation");

    std::cout << "Available Bottleneck Delays:" << std::endl;
    for (const auto& config : l_configs) {
        std::cout << "  - " << config.first << std::endl;
    }

    std::string delayName;
    std::cout << "\nEnter Bottleneck Delay: ";
    std::getline(std::cin, delayName);

    if (l_configs.find(delayName) == l_configs.end()) {
        std::cerr << "Invalid bottleneck delay." << std::endl;
        return;
    }

    std::string bottleneckDelay = l_configs[delayName];

    std::cout << "\nAvailable Traffic Modes:" << std::endl;
    std::cout << "  - tcp_only" << std::endl;
    std::cout << "  - udp_only" << std::endl;
    std::cout << "  - mixed" << std::endl;

    std::string trafficMode;
    std::cout << "\nEnter Traffic Mode: ";
    std::getline(std::cin, trafficMode);

    if (trafficMode == "tcp_only") {
        runLatencySimulationTcp(delayName, bottleneckDelay);
    } else if (trafficMode == "udp_only") {
        runLatencySimulationUdp(delayName, bottleneckDelay);
    } else if (trafficMode == "mixed") {
        runLatencySimulationMixed(delayName, bottleneckDelay);
    } else {
        std::cerr << "Invalid traffic mode." << std::endl;
        return;
    }
}

