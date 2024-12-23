#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-helper.h"
#include "ns3/point-to-point-epc-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("kpm_project");

int main(int argc, char *argv[]) {
    
    // At least two eNodeBs.
    uint16_t numENodeBs = 2;

    // A minimum of five User Equipment (UE) devices.
    uint16_t numUEs = 5;

    // At least one remote host (server) on the Internet.
    uint16_t numRemoteHosts = 1;

    NodeContainer ueNodes;
    NodeContainer eNodeBNodes;
    NodeContainer remoteHostContainer;

    eNodeBNodes.Create(numENodeBs);
    ueNodes.Create(numUEs);
    remoteHostContainer.Create(numRemoteHosts);

    return 0;
}