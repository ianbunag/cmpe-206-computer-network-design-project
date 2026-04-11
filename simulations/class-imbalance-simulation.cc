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
#include <vector>
#include "../constants.h"

using namespace ns3;

namespace {
    struct CI_Config {
        uint32_t tcpFlows;
        uint32_t udpFlows;
    };
    std::map<std::string, CI_Config> ci_configs = {
        {"9-to-1", {9, 1}},
        {"7-to-3", {7, 3}},
        {"5-to-5", {5, 5}},
        {"3-to-7", {3, 7}},
        {"1-to-9", {1, 9}}
    };
    constexpr double CI_SIMULATION_START_TIME = 1.0;
    constexpr double CI_SIMULATION_STOP_TIME = 20.0;
    const UintegerValue CI_SIZE_2_MB = 2 * 1024 * 1024;
    const UintegerValue CI_SIZE_1_KB = 1024;
    const std::string ci_filePrefix = "class_imbalance_";

    struct CI_Topology {
        NodeContainer nodeContainerSources,
            nodeContainerRouters,
            nodeContainerDestinations;
        PointToPointHelper pointToPointHelper,
            pointToPointHelper_r0_r1;
        std::vector<NetDeviceContainer> netDeviceContainers_sources_to_r0;
        NetDeviceContainer netDeviceContainer_r0_r1;
        std::vector<NetDeviceContainer> netDeviceContainers_r1_to_destinations;
        InternetStackHelper internetStackHelper;
        Ipv4AddressHelper ipv4AddressHelper;
        std::vector<Ipv4InterfaceContainer> ipv4InterfaceContainers_sources_to_r0;
        Ipv4InterfaceContainer ipv4InterfaceContainer_r0_r1;
        std::vector<Ipv4InterfaceContainer> ipv4InterfaceContainers_r1_to_destinations;
        MobilityHelper mobility;
    };
    struct CI_Monitors {
        FlowMonitorHelper flowMonitorHelper;
        Ptr<FlowMonitor> flowMonitor;
        std::unique_ptr<AnimationInterface> animationInterface;
    };
    void runClassImbalanceSimulationWithDefaultTopology(
        std::string testName,
        uint32_t numSources,
        uint32_t numDestinations,
        std::function<void(CI_Topology&)> configureScene,
        std::function<void(CI_Monitors&, CI_Topology&)> configureMonitors
    ) {
        CI_Topology t;

        t.nodeContainerSources.Create(numSources);
        t.nodeContainerRouters.Create(2);
        t.nodeContainerDestinations.Create(numDestinations);

        t.pointToPointHelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
        t.pointToPointHelper.SetChannelAttribute("Delay", StringValue("1ms"));

        t.pointToPointHelper_r0_r1.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
        t.pointToPointHelper_r0_r1.SetChannelAttribute("Delay", StringValue("10ms"));
        t.pointToPointHelper_r0_r1.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("50p")));

        for (uint32_t index = 0; index < numSources; ++index) {
            NetDeviceContainer netDeviceContainer = t.pointToPointHelper.Install(
                t.nodeContainerSources.Get(index),
                t.nodeContainerRouters.Get(0)
            );
            t.netDeviceContainers_sources_to_r0.push_back(netDeviceContainer);
        }

        t.netDeviceContainer_r0_r1 = t.pointToPointHelper_r0_r1.Install(
            t.nodeContainerRouters.Get(0),
            t.nodeContainerRouters.Get(1)
        );

        for (uint32_t index = 0; index < numDestinations; ++index) {
            NetDeviceContainer netDeviceContainer = t.pointToPointHelper.Install(
                t.nodeContainerRouters.Get(1),
                t.nodeContainerDestinations.Get(index)
            );
            t.netDeviceContainers_r1_to_destinations.push_back(netDeviceContainer);
        }

        t.internetStackHelper.Install(t.nodeContainerSources);
        t.internetStackHelper.Install(t.nodeContainerRouters);
        t.internetStackHelper.Install(t.nodeContainerDestinations);

        uint32_t subnetIndex = 1;

        for (uint32_t index = 0; index < numSources; ++index) {
            std::string subnetBase = "10.1." + std::to_string(subnetIndex) + ".0";
            t.ipv4AddressHelper.SetBase(Ipv4Address(subnetBase.c_str()), "255.255.255.0");
            Ipv4InterfaceContainer ipv4InterfaceContainer = t.ipv4AddressHelper.Assign(
                t.netDeviceContainers_sources_to_r0[index]
            );
            t.ipv4InterfaceContainers_sources_to_r0.push_back(ipv4InterfaceContainer);
            ++subnetIndex;
        }

        t.ipv4AddressHelper.SetBase("10.2.0.0", "255.255.255.0");
        t.ipv4InterfaceContainer_r0_r1 = t.ipv4AddressHelper.Assign(t.netDeviceContainer_r0_r1);

        for (uint32_t index = 0; index < numDestinations; ++index) {
            std::string subnetBase = "10.3." + std::to_string(index) + ".0";
            t.ipv4AddressHelper.SetBase(Ipv4Address(subnetBase.c_str()), "255.255.255.0");
            Ipv4InterfaceContainer ipv4InterfaceContainer = t.ipv4AddressHelper.Assign(
                t.netDeviceContainers_r1_to_destinations[index]
            );
            t.ipv4InterfaceContainers_r1_to_destinations.push_back(ipv4InterfaceContainer);
        }
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

        CI_Monitors m;

        std::string pcapBaseFileName = MONITORS_DIRECTORY + "/" + ci_filePrefix + testName + "_r0_r1";
        t.pointToPointHelper_r0_r1.EnablePcap(pcapBaseFileName, t.netDeviceContainer_r0_r1.Get(0), true);

        m.flowMonitor = m.flowMonitorHelper.InstallAll();

        std::string animationFileName = ANIMATIONS_DIRECTORY + "/" + ci_filePrefix + testName + "_animation.xml";
        m.animationInterface = std::make_unique<AnimationInterface>(animationFileName);
        m.animationInterface->SetMaxPktsPerTraceFile(1000000);

        m.animationInterface->SetConstantPosition(t.nodeContainerRouters.Get(0), 50, 50);
        m.animationInterface->SetConstantPosition(t.nodeContainerRouters.Get(1), 150, 50);

        double sourceSpacing = 100.0 / (numSources + 1);
        for (uint32_t index = 0; index < numSources; ++index) {
            double yPos = sourceSpacing * (index + 1);
            m.animationInterface->SetConstantPosition(t.nodeContainerSources.Get(index), 0, yPos);
        }

        double destSpacing = 100.0 / (numDestinations + 1);
        for (uint32_t index = 0; index < numDestinations; ++index) {
            double yPos = destSpacing * (index + 1);
            m.animationInterface->SetConstantPosition(t.nodeContainerDestinations.Get(index), 200, yPos);
        }

        m.animationInterface->UpdateNodeColor(t.nodeContainerRouters.Get(0), 0, 255, 0);
        m.animationInterface->UpdateNodeColor(t.nodeContainerRouters.Get(1), 0, 255, 0);

        for (uint32_t index = 0; index < numSources; ++index) {
            m.animationInterface->UpdateNodeDescription(t.nodeContainerSources.Get(index), "Source " + std::to_string(index));
        }
        m.animationInterface->UpdateNodeDescription(t.nodeContainerRouters.Get(0), "Router 0");
        m.animationInterface->UpdateNodeDescription(t.nodeContainerRouters.Get(1), "Router 1");
        for (uint32_t index = 0; index < numDestinations; ++index) {
            m.animationInterface->UpdateNodeDescription(t.nodeContainerDestinations.Get(index), "Destination " + std::to_string(index));
        }
        std::string baselineLinkDescription = "Baseline link (Rate: 100Mbps, Delay: 1ms)";
        std::string bottleneckLinkDescription = "Bottleneck link (Rate: 10Mbps, Delay: 10ms, Queue size: 50p)";

        for (uint32_t index = 0; index < numSources; ++index) {
            m.animationInterface->UpdateLinkDescription(
                t.nodeContainerSources.Get(index),
                t.nodeContainerRouters.Get(0),
                baselineLinkDescription
            );
        }

        m.animationInterface->UpdateLinkDescription(
            t.nodeContainerRouters.Get(0),
            t.nodeContainerRouters.Get(1),
            bottleneckLinkDescription
        );

        for (uint32_t index = 0; index < numDestinations; ++index) {
            m.animationInterface->UpdateLinkDescription(
                t.nodeContainerRouters.Get(1),
                t.nodeContainerDestinations.Get(index),
                baselineLinkDescription
            );
        }

        configureMonitors(m, t);

        Simulator::Stop(Seconds(CI_SIMULATION_STOP_TIME));
        Simulator::Run();

        std::string flowFileName = MONITORS_DIRECTORY + "/" + ci_filePrefix + testName + "_flow.xml";
        m.flowMonitor->SerializeToXmlFile(flowFileName, true, true);
        Simulator::Destroy();
    }

    void runClassImbalanceSimulationDefault(std::string testName, uint32_t numTcpFlows, uint32_t numUdpFlows) {
        NS_LOG_UNCOND("Starting runClassImbalanceSimulation test " << testName 
                     << " with " << numTcpFlows << " TCP flows and " << numUdpFlows << " UDP flows");
        uint32_t totalFlows = numTcpFlows + numUdpFlows;

        Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
        Config::SetDefault("ns3::TcpSocket::SndBufSize", CI_SIZE_2_MB);
        Config::SetDefault("ns3::TcpSocket::RcvBufSize", CI_SIZE_2_MB);

        runClassImbalanceSimulationWithDefaultTopology(
            testName,
            totalFlows,
            totalFlows,
            [numTcpFlows, numUdpFlows](CI_Topology& t) {
                // Configure TCP flows
                for (uint32_t index = 0; index < numTcpFlows; ++index) {
                    BulkSendHelper bulkSendHelper(
                        "ns3::TcpSocketFactory",
                        InetSocketAddress(t.ipv4InterfaceContainers_r1_to_destinations[index].GetAddress(1), 10)
                    );
                    bulkSendHelper.SetAttribute("MaxBytes", UintegerValue(0)); // Unlimited
                    ApplicationContainer sourceContainer = bulkSendHelper.Install(t.nodeContainerSources.Get(index));
                    sourceContainer.Start(Seconds(CI_SIMULATION_START_TIME));
                    sourceContainer.Stop(Seconds(CI_SIMULATION_STOP_TIME));
                    PacketSinkHelper packetSinkHelper(
                        "ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 10)
                    );
                    ApplicationContainer destinationContainer = packetSinkHelper.Install(t.nodeContainerDestinations.Get(index));
                    destinationContainer.Start(Seconds(0.0));
                    destinationContainer.Stop(Seconds(CI_SIMULATION_STOP_TIME));
                }

                // Configure UDP flows
                for (uint32_t index = numTcpFlows; index < numUdpFlows + numTcpFlows; ++index) {
                    OnOffHelper onOffHelper(
                        "ns3::UdpSocketFactory",
                        InetSocketAddress(t.ipv4InterfaceContainers_r1_to_destinations[index].GetAddress(1), 9)
                    );
                    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=20]"));
                    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                    onOffHelper.SetAttribute("DataRate", StringValue("1.1Mbps"));
                    onOffHelper.SetAttribute("PacketSize", CI_SIZE_1_KB);
                    ApplicationContainer sourceContainer = onOffHelper.Install(t.nodeContainerSources.Get(index));
                    sourceContainer.Start(Seconds(CI_SIMULATION_START_TIME));
                    sourceContainer.Stop(Seconds(CI_SIMULATION_STOP_TIME));

                    PacketSinkHelper packetSinkHelper(
                        "ns3::UdpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), 9)
                    );
                    ApplicationContainer destinationContainer = packetSinkHelper.Install(t.nodeContainerDestinations.Get(index));
                    destinationContainer.Start(Seconds(0.0));
                    destinationContainer.Stop(Seconds(CI_SIMULATION_STOP_TIME));
                }
            },
            [numTcpFlows, numUdpFlows](CI_Monitors& m, CI_Topology& t) {
                for (uint32_t index = 0; index < numTcpFlows; ++index) {
                    m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(index), 0, 0, 255); // Blue
                    m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(index), 0, 0, 255); // Blue
                }
                for (uint32_t index = numTcpFlows; index < numUdpFlows + numTcpFlows; ++index) {
                    m.animationInterface->UpdateNodeColor(t.nodeContainerSources.Get(index), 255, 0, 0); // Red
                    m.animationInterface->UpdateNodeColor(t.nodeContainerDestinations.Get(index), 255, 0, 0); // Red
                }
            }
        );

        NS_LOG_UNCOND("Ended runClassImbalanceSimulation");
    }
}

void runClassImbalanceSimulation() {
    NS_LOG_COMPONENT_DEFINE("ClassImbalanceSimulation");

    std::cout << "Available Flow Ratios (TCP:UDP):" << std::endl;
    for (const auto& config : ci_configs) {
        std::cout << "  - " << config.first << " (" << config.second.tcpFlows << " TCP, "
                  << config.second.udpFlows << " UDP)" << std::endl;
    }

    std::string ratioName;
    std::cout << "\nEnter Flow Ratio: ";
    std::getline(std::cin, ratioName);

    if (ci_configs.find(ratioName) == ci_configs.end()) {
        std::cerr << "Invalid flow ratio." << std::endl;
        return;
    }

    CI_Config ratio = ci_configs[ratioName];

    runClassImbalanceSimulationDefault(ratioName, ratio.tcpFlows, ratio.udpFlows);
}
