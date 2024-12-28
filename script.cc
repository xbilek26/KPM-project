/*
 * Authors:
 * Frantisek Bilek <xbilek26@vutbr.cz>
 * ...
 * ...
 * ...
 * 
 * 
 * TODO: Mobility (line 119 - 149)
 * Use two different position allocators for the eNodeBs and the UEs.
 * Choose a suitable mobility model for the UEs to mimic the movement of pedestrians.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/lte-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"

/* ---------------------------- SIMULATION SCENARIO ----------------------------
 *
 *                           +----------------------+
 *                           | Remote Host 10.0.0.1 |
 *                           +----------------------+
 *                                       |
 *                                       | Point-to-Point (Internet)
 *                                       |
 *                                       v
 *                           +----------------------+
 *                           |     PGW 10.0.0.2     |
 *                           +----------------------+
 *                                       |
 *                                       | (S1 interface)
 *                                       |
 *                       +---------------+----------------+
 *                       |                                |
 *                       v                                v
 *           +------------------------+       +------------------------+
 *           |        eNodeB 0        |       |        eNodeB 1        |
 *           +------------------------+       +------------------------+
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
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    // Number of UEs (used multiple times in the code)
    uint32_t numUes = 5;

    // Simulation parameters
    double distance = 500.0; // Distance between eNodeBs
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

    // Set up point-to-point link between PGW and remote host
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NetDeviceContainer internetDevices = p2p.Install(remoteHost, pgw);

    // Enable pcap tracing on point-to-point link
    p2p.EnablePcapAll("p2p");

    // Assign IP addresses to point-to-point link (internet)
    Ipv4AddressHelper ipv4hInternet;
    ipv4hInternet.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4hInternet.Assign(internetDevices);
    NS_LOG_INFO("Remote host address: " << internetIpIfaces.GetAddress(0));
    NS_LOG_INFO("PGW address: " << internetIpIfaces.GetAddress(1));

    // Create static routing
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // Install LTE Internet Stack
    InternetStackHelper ueInternet;
    ueInternet.Install(ueNodes);

    // TODO: Mobility (I'm not sure if this is correct)

    MobilityHelper mobility;

    // eNodeB Position Allocator (Static positions)
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    enbPositionAlloc->Add(Vector(0, 0, 0));  // eNodeB 0 at (0, 0, 0)
    enbPositionAlloc->Add(Vector(distance, 0, 0));  // eNodeB 1 at (distance, 0, 0)
    mobility.SetPositionAllocator(enbPositionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // Static positions for eNodeBs
    mobility.Install(enbNodes);

    // UE Position Allocator (Dynamic positions using Random Walk)
    Ptr<RandomRectanglePositionAllocator> uePositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    Ptr<UniformRandomVariable> xVar = CreateObject<UniformRandomVariable>();
    xVar->SetAttribute("Min", DoubleValue(0.0));
    xVar->SetAttribute("Max", DoubleValue(distance));
    uePositionAlloc->SetX(xVar);

    Ptr<UniformRandomVariable> yVar = CreateObject<UniformRandomVariable>();
    yVar->SetAttribute("Min", DoubleValue(0.0));
    yVar->SetAttribute("Max", DoubleValue(100.0));  // Limit to 100 units along y-axis
    uePositionAlloc->SetY(yVar);

    // Set the mobility model for UEs to mimic pedestrian movement (Random Walk)
    mobility.SetPositionAllocator(uePositionAlloc);
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", StringValue("0|500|0|100"),  // Boundaries for UEs' movement area
                            "Speed", StringValue("ns3::ConstantRandomVariable[Constant=1.5]"),  // Speed of 1.5 m/s (approx walking speed)
                            "Distance", DoubleValue(10.0));  // Distance moved before changing direction
    mobility.Install(ueNodes);

    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes); // Add eNB nodes to the container
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes); // Add UE nodes to the container

    // Assign IP addresses to UEs
    Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

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
    uint16_t tcpPort = 1100;
    uint16_t udpPort = 2000;

    // Application containers
    ApplicationContainer sourceApps, sinkApps;

    // File Transfer: nodes 0 and 1 send and receive TCP traffic between each other
    for (uint32_t i = 0; i < 2; i++)
    {
        // Traffic sources, TCP clients
        OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(ueIpIfaces.GetAddress(1 - i), tcpPort));
        onOffHelper.SetConstantRate(DataRate("5kbps"));
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
        onOffHelper.SetConstantRate(DataRate("5kbps"));
        sourceApps.Add(onOffHelper.Install(remoteHost));

        // Traffic sinks, UDP servers
        PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), udpPort));
        sinkApps.Add(sinkHelper.Install(ueNodes.Get(i)));
    }

    sourceApps.Start(Seconds(1.0));
    sinkApps.Start(Seconds(2.0));

    sourceApps.Stop(Seconds(10.0));
    sinkApps.Stop(Seconds(9.0));

    // Run simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    /*

    AnimationInterface anim("lte-simulation.xml");

    anim.SetConstantPosition(remoteHost, 0.0, 50.0);
    anim.SetConstantPosition(enbNodes.Get(0), 50.0, 50.0);
    anim.SetConstantPosition(enbNodes.Get(1), 150.0, 50.0);

    anim.UpdateNodeDescription(remoteHost, "RemoteHost");
    anim.UpdateNodeDescription(enbNodes.Get(0), "eNodeB_0");
    anim.UpdateNodeDescription(enbNodes.Get(1), "eNodeB_1");
    anim.UpdateNodeDescription(ueNodes.Get(0), "UE_0");
    anim.UpdateNodeDescription(ueNodes.Get(1), "UE_1");
    anim.UpdateNodeDescription(ueNodes.Get(2), "UE_2");
    anim.UpdateNodeDescription(ueNodes.Get(3), "UE_3");
    anim.UpdateNodeDescription(ueNodes.Get(4), "UE_4");

    anim.UpdateNodeColor(remoteHost, 255, 0, 0);
    anim.UpdateNodeColor(enbNodes.Get(0), 0, 255, 0);
    anim.UpdateNodeColor(enbNodes.Get(1), 0, 255, 0);
    anim.UpdateNodeColor(ueNodes.Get(0), 0, 0, 255);
    anim.UpdateNodeColor(ueNodes.Get(1), 0, 0, 255);
    anim.UpdateNodeColor(ueNodes.Get(2), 0, 0, 255);
    anim.UpdateNodeColor(ueNodes.Get(3), 0, 0, 255);
    anim.UpdateNodeColor(ueNodes.Get(4), 0, 0, 255);

    */

    Simulator::Destroy();

    return 0;
}
