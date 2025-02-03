#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "/home/uwe/ns3/ns-allinone-3.43/ns-3.43/src/aodv/model/blackhole-aodv.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EnhancedBlackholeSimulation");

// Global metrics
uint32_t totalSentPackets = 0;
uint32_t totalReceivedPackets = 0;
Time totalDelay = Seconds(0);

// Callback for receiving packets
void ReceivePacket(Ptr<Socket> socket) {
    while (Ptr<Packet> packet = socket->Recv()) {
        totalReceivedPackets++;
        TimestampTag timestamp;
        if (packet->PeekPacketTag(timestamp)) {
            Time delay = Simulator::Now() - timestamp.GetTimestamp();
            totalDelay += delay;
        }
    }
}

// Log simulation statistics
void LogStatistics(uint32_t totalNodes, double totalTime) {
    uint32_t totalLostPackets = totalSentPackets - totalReceivedPackets;
    double packetLossRatio = ((double)totalLostPackets / totalSentPackets) * 100.0;
    double packetDeliveryRatio = ((double)totalReceivedPackets / totalSentPackets) * 100.0;
    double averageThroughput = (totalReceivedPackets * 1024 * 8) / (totalTime * 1000.0);
    double averageDelay = (totalReceivedPackets > 0) ? (totalDelay.GetSeconds() / totalReceivedPackets) : -1.0;

    std::cout << "\n-------- Simulation Results --------" << std::endl;
    std::cout << "Total Nodes: " << totalNodes << std::endl;
    std::cout << "Simulation Time: " << totalTime << " seconds" << std::endl;
    std::cout << "Sent Packets: " << totalSentPackets << std::endl;
    std::cout << "Received Packets: " << totalReceivedPackets << std::endl;
    std::cout << "Lost Packets: " << totalLostPackets << std::endl;
    std::cout << "Packet Loss Ratio: " << packetLossRatio << "%" << std::endl;
    std::cout << "Packet Delivery Ratio: " << packetDeliveryRatio << "%" << std::endl;
    std::cout << "Average Throughput: " << averageThroughput << " Kbps" << std::endl;
    std::cout << "Average End-to-End Delay: " << ((averageDelay >= 0) ? averageDelay : -1) << " seconds" << std::endl;
}

int main() {
    // Simulation parameters
    uint32_t nodes = 200;
    double simTime = 10.0;
    uint32_t trafficRate = 1024;  // Packets per second
    std::vector<uint32_t> blackholeNodes = {10, 15, 25, 35, 40, 55};

    // Create nodes
    NodeContainer nodeContainer;
    nodeContainer.Create(nodes);

    // Mobility setup
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(50.0),
                                  "DeltaY", DoubleValue(50.0),
                                  "GridWidth", UintegerValue(10),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodeContainer);

    // WiFi setup
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiHelper wifiHelper;
    wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate6Mbps"));
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifiHelper.Install(wifiPhy, wifiMac, nodeContainer);

    // Install Internet stack
    AodvHelper aodvHelper;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodvHelper);
    internet.Install(nodeContainer);

    // Configure blackhole nodes
    for (uint32_t nodeIndex : blackholeNodes) {
        Ptr<Node> blackholeNode = nodeContainer.Get(nodeIndex);
        Ptr<BlackholeAodv> blackholeRouting = CreateObject<BlackholeAodv>();
        //blackholeRouting->InitializeTrustScores(nodes);
        blackholeNode->AggregateObject(blackholeRouting);
    }

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // UDP traffic setup
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sourceSocket = Socket::CreateSocket(nodeContainer.Get(1), tid);
    InetSocketAddress remote = InetSocketAddress(interfaces.GetAddress(nodes - 1), 9);
    sourceSocket->Connect(remote);

    Ptr<Socket> recvSocket = Socket::CreateSocket(nodeContainer.Get(nodes - 1), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 9);
    recvSocket->Bind(local);
    recvSocket->SetRecvCallback(MakeCallback(&ReceivePacket));

    double packetInterval = 1.0 / trafficRate;

    for (uint32_t i = 0; i < trafficRate * simTime; ++i) {
        Simulator::Schedule(Seconds(i * packetInterval), [sourceSocket]() {
            Ptr<Packet> packet = Create<Packet>(1024); // Fixed size for debugging
            TimestampTag timestamp;
            timestamp.SetTimestamp(Simulator::Now());
            packet->AddPacketTag(timestamp);
            sourceSocket->Send(packet);
            totalSentPackets++;
        });
    }

    // Flow monitor setup
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    // Run simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // Log results
    LogStatistics(nodes, simTime);

    // Serialize flow monitor results
    monitor->SerializeToXmlFile("flowmon-results.xml", true, true);

    Simulator::Destroy();
    return 0;
}

