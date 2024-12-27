#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/lte-helper.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("KPMProjectScript");

int main(int argc, char *argv[])
{
    // Povolení logování
    LogComponentEnable("KPMProjectScript", LOG_LEVEL_INFO);
    LogComponentEnable("LteHelper", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    // Set simulation parameters
    double simTime = 10.0; // Simulation duration in seconds
    double distance = 500.0; // Distance between eNodeBs
    uint16_t numUes = 5; // Number of UEs

    // Create LTE Helper and EPC Helper
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    // Install LTE devices
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create nodes for eNodeBs and UEs
    NodeContainer enbNodes;
    enbNodes.Create(2); // Two eNodeBs

    NodeContainer ueNodes;
    ueNodes.Create(numUes); // Five UEs

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
    NetDeviceContainer internetDevices = p2p.Install(pgw, remoteHost);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("1.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
    // Print remote host address
    NS_LOG_INFO("Remote host address: " << remoteHostAddr);

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

    // Install LTE Devices to eNodeBs and UEs
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Assign IP addresses to UEs
    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    NS_LOG_INFO("Assigned UE IP Addresses:");
    for (uint32_t i = 0; i < numUes; i++)
    {
        NS_LOG_INFO("UE " << i << " IP Address: " << ueIpIface.GetAddress(i));
    }

    // Attach UEs to eNodeBs
    for (uint32_t i = 0; i < numUes; i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i % 2));
        NS_LOG_INFO("Attached UE " << i << " to eNodeB " << (i % 2));
    }

    // Applications
    uint16_t dlPort = 1100;
    uint16_t ulPort = 2000;
    ApplicationContainer serverApps, clientApps;

    // File Transfer: BulkSendApplication
    for (uint32_t i = 0; i < 2; i++)
    {
        OnOffHelper onOffHelper("ns3::TcpSocketFactory", InetSocketAddress(ueIpIface.GetAddress(1 - i), dlPort));

        onOffHelper.SetConstantRate(DataRate("50Mbps"));
        clientApps.Add(onOffHelper.Install(ueNodes.Get(i)));

        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        serverApps.Add(sinkHelper.Install(ueNodes.Get(1 - i)));
    }

    // Video Streaming
    for (uint32_t i = 2; i < 5; i++)
    {
        // ERROR HERE
        OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(remoteHostAddr, ulPort));
        
        onOffHelper.SetConstantRate(DataRate("1Mbps"));
        clientApps.Add(onOffHelper.Install(ueNodes.Get(i)));
        
        //PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        //serverApps.Add(sinkHelper.Install(remoteHost));
        
    }

    serverApps.Start(Seconds(1.0));
    clientApps.Start(Seconds(2.0));

    serverApps.Stop(Seconds(10.0));
    clientApps.Stop(Seconds(9.0));

    // Enable tracing
    lteHelper->EnableTraces();

    // Run simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
