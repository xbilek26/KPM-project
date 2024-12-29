/*
 * Authors:
 * Frantisek Bilek <xbilek26@vutbr.cz>
 * ...
 * ...
 * ...
 * 
 * TODO: Simulation Run
 * - Execute the simulation with different sets of parameters and collect data on network performance.
 * - Run the simulation for a sufficient duration to observe network behavior.
 * - Capture and analyze key metrics, such as throughput, latency, and packet loss.
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-epc-helper.h"

/* ---------------------------- SIMULATION SCENARIO ----------------------------
 *
 *                           +----------------------+
 *                           |      Remote Host     |
 *                           +----------------------+
 *                               1.0.0.1 |
 *                                       | Point-to-Point
 *                                       |
 *                               1.0.0.2 v
 *                           +----------------------+
 *                           |         P-GW         |
 *                           +----------------------+
 *                              14.0.0.5 |
 *                                       | S5 interface
 *                              14.0.0.6 |
 *                           +----------------------+ 13.0.0.6     13.0.0.5 +----------------------+
 *                           |         S-GW         |-----------------------|         MME          |
 *                           +----------------------+     S11 interface     +----------------------+
 *                                       |
 *                                       | S1 interface
 *                                       |
 *                       +---------------+----------------+
 *                       |                                |
 *                       v                                v
 *           +-----------------------+        +-----------------------+
 *           |   eNodeB 0 10.0.0.5   |        |   eNodeB 1 10.0.0.9   |
 *           +-----------------------+        +-----------------------+
 *                       |                                |
 *                       | LTE                            | LTE
 *                       v                                v
 *              +------------------+             +------------------+
 *              |   UE 0 7.0.0.2   |             |   UE 1 7.0.0.3   |
 *              +------------------+             +------------------+
 *              +------------------+             +------------------+
 *              |   UE 2 7.0.0.4   |             |   UE 3 7.0.0.5   |
 *              +------------------+             +------------------+
 *              +------------------+
 *              |   UE 4 7.0.0.6   |
 *              +------------------+
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("KPMProjectScript");

int main(int argc, char *argv[])
{
    LogComponentEnable("KPMProjectScript", LOG_LEVEL_INFO);
    //LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    ns3::PacketMetadata::Enable();

    // Number of UEs (used multiple times in the code)
    uint32_t numUes = 5;

    // Simulation parameters
    double simTime = 10.0;

    // Create LTE Helper and EPC Helper
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    // Create nodes for eNodeBs and UEs
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(2);
    ueNodes.Create(numUes);

    // Create a remote host
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Set up point-to-point link between P-GW and remote host
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NetDeviceContainer internetDevices = p2p.Install(remoteHost, pgw);

    // Get S-GW and MME nodes (just for NetAnim configuration)
    Ptr<Node> sgw = epcHelper->GetSgwNode();
    Ptr<Node> mme = NodeList::GetNode(2); // No getter method for MME

    // Enable pcap tracing on point-to-point link
    p2p.EnablePcapAll("p2p");

    // Assign IP addresses to point-to-point link (internet)
    Ipv4AddressHelper ipv4hInternet;
    ipv4hInternet.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4hInternet.Assign(internetDevices);
    NS_LOG_INFO("Remote host address: " << internetIpIfaces.GetAddress(0));
    NS_LOG_INFO("P-GW address: " << internetIpIfaces.GetAddress(1));

    // Create static routing
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // Install LTE Internet Stack
    InternetStackHelper ueInternet;
    ueInternet.Install(ueNodes);

    // Mobility
    MobilityHelper mobilityUes;
    MobilityHelper mobilityEnbs;

    mobilityEnbs.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(25.0),
        "MinY", DoubleValue(50.0),
        "DeltaX", DoubleValue(50.0),
        "DeltaY", DoubleValue(0.0)
    );

    mobilityEnbs.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobilityUes.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=100]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=100]")
    );
    
    mobilityUes.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue(Rectangle(0, 100, 0, 100)),
    "Speed", StringValue("ns3::ConstantRandomVariable[Constant=10.0]")
    );

    mobilityUes.Install(ueNodes);
    mobilityEnbs.Install(enbNodes);

    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes); // Add eNB nodes to the container
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes); // Add UE nodes to the container

    // Assign IP addresses to UEs
    Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    for (uint32_t i = 0; i < numUes; i++)
    {
      Ptr<Node> ueNode = ueNodes.Get(i);
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
      ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    for (uint32_t i = 0; i < numUes; i++)
    {
        NS_LOG_INFO("UE " << i << " IP Address: " << ueIpIfaces.GetAddress(i));
    }

    // Attach UEs 0, 2 and 4 to eNodeB 0 and UEs 1 and 3 to eNodeB 1
    for (uint32_t i = 0; i < numUes; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i % 2));
        NS_LOG_INFO("Attached UE " << i << " to eNodeB " << i % 2);
    }

    // Definitions of ports to send traffic to
    uint16_t tcpPort = 4000;
    uint16_t udpPort = 5000;

    // Application containers
    ApplicationContainer sourceApps, sinkApps;

    // File Transfer: nodes 0 and 1 send and receive TCP traffic between each other
    for (uint32_t i = 0; i < 2; i++)
    {
        // Traffic sources, TCP clients
        OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(ueIpIfaces.GetAddress(1 - i), tcpPort));
        onOffHelper.SetConstantRate(DataRate("100kbps"));
        sourceApps.Add(onOffHelper.Install(ueNodes.Get(i)));

        // Traffic sinks, TCP servers
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
        sinkApps.Add(sinkHelper.Install(ueNodes.Get(1 - i)));
    }

    // Video Streaming: nodes 2, 3 and 4 receive UDP traffic from remote host
    for (uint32_t i = 2; i < 5; i++)
    {
        // Traffic source (Remote Host), UDP client
        OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(ueIpIfaces.GetAddress(i), udpPort));
        onOffHelper.SetConstantRate(DataRate("100kbps"));
        sourceApps.Add(onOffHelper.Install(remoteHost));

        // Traffic sinks, UDP servers
        PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), udpPort));
        sinkApps.Add(sinkHelper.Install(ueNodes.Get(i)));
    }

    sourceApps.Start(Seconds(1.0));
    sinkApps.Start(Seconds(2.0));
    sourceApps.Stop(Seconds(10.0));
    sinkApps.Stop(Seconds(9.0));

    // Flow monitor
    Ptr <FlowMonitor> monitor;
    FlowMonitorHelper flowMonHelper;
    monitor = flowMonHelper.InstallAll();
    monitor->Start(Seconds(1.0));

    // NetAnim configuration
    AnimationInterface::SetConstantPosition(remoteHost, 50.0, 5.0);
    AnimationInterface::SetConstantPosition(pgw, 50.0, 20.0);
    AnimationInterface::SetConstantPosition(sgw, 50.0, 35.0);
    AnimationInterface::SetConstantPosition(mme, 75.0, 35.0);
    AnimationInterface anim("lte-simulation.xml");
    anim.EnablePacketMetadata(true);
    anim.SetMobilityPollInterval(Seconds(0.05));

    unsigned long long testValue = 0xFFFFFFFFFFFFFFFF;
    anim.SetMaxPktsPerTraceFile(testValue);

    anim.UpdateNodeDescription(remoteHost, "Remote Host");
    anim.UpdateNodeDescription(pgw, "P-GW");
    anim.UpdateNodeDescription(sgw, "S-GW");
    anim.UpdateNodeDescription(mme, "MME");
    anim.UpdateNodeDescription(enbNodes.Get(0), "eNodeB 0");
    anim.UpdateNodeDescription(enbNodes.Get(1), "eNodeB 1");
    anim.UpdateNodeDescription(ueNodes.Get(0), "UE 0");
    anim.UpdateNodeDescription(ueNodes.Get(1), "UE 1");
    anim.UpdateNodeDescription(ueNodes.Get(2), "UE 2");
    anim.UpdateNodeDescription(ueNodes.Get(3), "UE 3");
    anim.UpdateNodeDescription(ueNodes.Get(4), "UE 4");

    anim.UpdateNodeColor(remoteHost, 255, 0, 0);
    anim.UpdateNodeColor(pgw, 255, 255, 0);
    anim.UpdateNodeColor(sgw, 0, 255, 255);
    anim.UpdateNodeColor(mme, 255, 0, 255);
    anim.UpdateNodeColor(enbNodes.Get(0), 0, 255, 0);
    anim.UpdateNodeColor(enbNodes.Get(1), 0, 255, 0);
    anim.UpdateNodeColor(ueNodes.Get(0), 0, 0, 255);
    anim.UpdateNodeColor(ueNodes.Get(1), 0, 0, 255);
    anim.UpdateNodeColor(ueNodes.Get(2), 0, 0, 255);
    anim.UpdateNodeColor(ueNodes.Get(3), 0, 0, 255);
    anim.UpdateNodeColor(ueNodes.Get(4), 0, 0, 255);

    anim.UpdateNodeSize(remoteHost->GetId(), 5.0, 5.0);
    anim.UpdateNodeSize(pgw->GetId(), 5.0, 5.0);
    anim.UpdateNodeSize(sgw->GetId(), 5.0, 5.0);
    anim.UpdateNodeSize(mme->GetId(), 5.0, 5.0);
    anim.UpdateNodeSize(enbNodes.Get(0)->GetId(), 5.0, 5.0);
    anim.UpdateNodeSize(enbNodes.Get(1)->GetId(), 5.0, 5.0);
    anim.UpdateNodeSize(ueNodes.Get(0)->GetId(), 2.0, 2.0);
    anim.UpdateNodeSize(ueNodes.Get(1)->GetId(), 2.0, 2.0);
    anim.UpdateNodeSize(ueNodes.Get(2)->GetId(), 2.0, 2.0);
    anim.UpdateNodeSize(ueNodes.Get(3)->GetId(), 2.0, 2.0);
    anim.UpdateNodeSize(ueNodes.Get(4)->GetId(), 2.0, 2.0);

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr <Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonHelper.GetClassifier());
    std::map <FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::cout << std::endl << "************* FLOW MONITOR STATISTICS *************" << std::endl;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); i++) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow ID: " << i->first << std::endl;
        std::cout << "Src address: " << t.sourceAddress << " -> Dst address: " << t.destinationAddress << std::endl;
        std::cout << "Src port: " << t.sourcePort << " -> Dst port: " << t.destinationPort << std::endl;
        std::cout << "Tx Packets/Bytes: " << i->second.txPackets << "/" << i->second.txBytes << std::endl;
        std::cout << "Rx Packets/Bytes: " << i->second.rxPackets << "/" << i->second.rxBytes << std::endl;
        std::cout << "Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1024 << " kb/s" << std::endl;
        std::cout << "Delay sum: " << i->second.delaySum.GetMilliSeconds() << " ms" << std::endl;
        std::cout << "Mean delay: " << (i->second.delaySum.GetSeconds() / i->second.rxPackets) * 1000 << " ms" << std::endl;
        std::cout << "Jitter sum: " << i->second.jitterSum.GetMilliSeconds() << " ms" << std::endl;
        std::cout << "Mean jitter: " << (i->second.jitterSum.GetSeconds() / (i->second.rxPackets - 1)) * 1000 << " ms" << std::endl;
        std::cout << "Lost Packets: " << i->second.txPackets - i->second.rxPackets << std::endl;
        std::cout << "Packet loss: " << (((i->second.txPackets - i->second.rxPackets) * 1.0) / i->second.txPackets) * 100 << " %" << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
    }

    Simulator::Destroy();

    return 0;
}