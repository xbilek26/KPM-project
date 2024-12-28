/*
 * Authors:
 * Frantisek Bilek <xbilek26@vutbr.cz>
 * ...
 * ...
 * ...
 * 
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/lte-helper.h"
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
 *              |   UE 4 7.0.0.2   |
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

    // Position allocators for eNodeBs and UEs
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    enbPositionAlloc->Add(Vector(0, 0, 0));
    enbPositionAlloc->Add(Vector(distance, 0, 0));
    mobility.SetPositionAllocator(enbPositionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);

    Ptr<RandomRectanglePositionAllocator> uePositionAlloc = CreateObject<RandomRectanglePositionAllocator>();
    Ptr<UniformRandomVariable> xVar = CreateObject<UniformRandomVariable>();
    xVar->SetAttribute("Min", DoubleValue(0.0));
    xVar->SetAttribute("Max", DoubleValue(distance));
    uePositionAlloc->SetX(xVar);

    Ptr<UniformRandomVariable> yVar = CreateObject<UniformRandomVariable>();
    yVar->SetAttribute("Min", DoubleValue(0.0));
    yVar->SetAttribute("Max", DoubleValue(100.0));
    uePositionAlloc->SetY(yVar);

    mobility.SetPositionAllocator(uePositionAlloc);
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", StringValue("0|500|0|100"),
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=1.5]"),
                              "Distance", DoubleValue(10.0));
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
        // traffic sources, TCP clients
        OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(ueIpIfaces.GetAddress(1 - i), tcpPort));
        onOffHelper.SetConstantRate(DataRate("5kbps"));
        sourceApps.Add(onOffHelper.Install(ueNodes.Get(i)));

        // traffic sinks, TCP servers
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
        sinkApps.Add(sinkHelper.Install(ueNodes.Get(1 - i)));
    }

    //Video Streaming: nodes 2, 3 and 4 receive UDP traffic from remote host
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

    lteHelper->EnableTraces();

    // Run simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
