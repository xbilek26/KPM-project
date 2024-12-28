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
 *                                       | S1 interface
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
 *              | UE 0 192.168.0.1 |             | UE 1 192.168.0.2 |
 *              +------------------+             +------------------+
 *              +------------------+             +------------------+
 *              | UE 2 192.168.0.3 |             | UE 3 192.168.0.4 |
 *              +------------------+             +------------------+
 *              +------------------+
 *              | UE 4 192.168.0.5 |
 *              +------------------+
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("KPMProject");

int main(int argc, char *argv[])
{
    LogComponentEnable("KPMProject", LOG_LEVEL_INFO);

    // Number of UEs (used multiple times in the code)
    uint32_t numUes = 5;

    // Simulation parameters
    double simTime = 10.0; // Simulation duration in seconds
    double distance = 500.0; // Distance between eNodeBs

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
    Ipv4AddressHelper ipv4hUEs;
    ipv4hUEs.SetBase("192.168.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ueIpIfaces = ipv4hUEs.Assign(NetDeviceContainer(ueLteDevs));

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

    // Applications
    uint16_t dlPort = 1100;
    uint16_t ulPort = 2000;
    ApplicationContainer serverApps, clientApps;

    // File Transfer: TCP
    for (uint32_t i = 0; i < 2; i++)
    {
        // clients
        OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(ueIpIfaces.GetAddress(1 - i), dlPort));
        onOffHelper.SetConstantRate(DataRate("5kbps"));
        clientApps.Add(onOffHelper.Install(ueNodes.Get(i)));

        // servers
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        serverApps.Add(sinkHelper.Install(ueNodes.Get(1 - i)));
    }

    // Video Streaming: UDP
    for (uint32_t i = 2; i < 5; i++)
    {
        // client (source) on remoteHost
        OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(ueIpIfaces.GetAddress(i), ulPort));
        onOffHelper.SetConstantRate(DataRate("5Mbps")); // Set data rate for UDP stream
        clientApps.Add(onOffHelper.Install(remoteHost)); // Install client on remoteHost

        NS_LOG_INFO("UDP Client installed on remoteHost to send traffic to UE " << i
                    << " with IP " << ueIpIfaces.GetAddress(i));

        // server (sink) on UE nodes
        PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        serverApps.Add(sinkHelper.Install(ueNodes.Get(i))); // Install server on each UE node

        NS_LOG_INFO("UDP Sink installed on UE " << i
                    << " to receive traffic on port " << ulPort);
    }

    serverApps.Start(Seconds(1.0));
    clientApps.Start(Seconds(2.0));

    serverApps.Stop(Seconds(10.0));
    clientApps.Stop(Seconds(9.0));

    // Run simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
