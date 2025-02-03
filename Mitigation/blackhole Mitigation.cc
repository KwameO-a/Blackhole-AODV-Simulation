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
std::map<uint32_t, double> globalTrustScores;

// Callback for receiving packets
void ReceivePacket(Ptr<Socket> socket) {
    while (Ptr<Packet> packet = socket->Recv()) {
        totalReceivedPackets++;
        
        // Extract timestamp for delay calculation
        TimestampTag timestamp;
        if (packet->PeekPacketTag(timestamp)) {
            Time delay = Simulator::Now() - timestamp.GetTimestamp();
            totalDelay += delay;
        }
        
        // Log sender address
        Address senderAddress;
        if (socket->GetPeerName(senderAddress) == 0) {
            if (InetSocketAddress::IsMatchingType(senderAddress)) {
                Ipv4Address senderIp = InetSocketAddress::ConvertFrom(senderAddress).GetIpv4();
                NS_LOG_INFO("Packet received from IP: " << senderIp);
            }
        }
    }
}



// Update global trust scores
void UpdateGlobalTrustScores(const NodeContainer &nodeContainer) {
    NS_LOG_INFO("Updating Global Trust Scores");
    
    for (uint32_t i = 0; i < nodeContainer.GetN(); ++i) {
        Ptr<BlackholeAodv> blackholeRouting = nodeContainer.Get(i)->GetObject<BlackholeAodv>();
        
        if (blackholeRouting) {
            // Retrieve local trust scores
            const std::map<uint32_t, double> &localScores = blackholeRouting->GetTrustScores();

            for (const auto &entry : localScores) {
                uint32_t nodeId = entry.first;
                double localTrustScore = entry.second;

                // Update global trust scores and log any changes
                auto it = globalTrustScores.find(nodeId);
                if (it != globalTrustScores.end()) {
                    double previousScore = it->second;
                    globalTrustScores[nodeId] = localTrustScore;

                    if (localTrustScore != previousScore) {
                        NS_LOG_INFO("Node " << nodeId 
                                    << " Trust Score updated from " 
                                    << previousScore << " to " << localTrustScore);
                    }
                } else {
                    // Add new trust score entry
                    globalTrustScores[nodeId] = localTrustScore;
                    NS_LOG_INFO("Node " << nodeId 
                                << " added with initial Trust Score = " 
                                << localTrustScore);
                }
            }
        }
    }
}


// Periodically log trust scores and evaluate behavior
void PeriodicTrustLogging(const NodeContainer &nodeContainer, Time interval) {
    NS_LOG_INFO("Logging Trust Scores...");
    for (uint32_t i = 0; i < nodeContainer.GetN(); ++i) {
        Ptr<BlackholeAodv> blackholeRouting = nodeContainer.Get(i)->GetObject<BlackholeAodv>();
        if (blackholeRouting) {
            blackholeRouting->LogTrustScores();
        }
    }
    Simulator::Schedule(interval, &PeriodicTrustLogging, nodeContainer, interval);
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

    std::cout << "-------- Global Trust Scores --------" << std::endl;
    for (const auto &entry : globalTrustScores) {
        std::cout << "Node " << entry.first << ": Trust Score = " << entry.second << std::endl;
    }
}

int main() {
    uint32_t nodes = 10;
    double simTime = 50.0;
    uint32_t trafficRate = 128; // Reduced traffic rate for realism
    std::vector<uint32_t> blackholeNodes = {10};

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

    // Internet setup
    AodvHelper aodvHelper;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodvHelper);
    internet.Install(nodeContainer);

    // Blackhole nodes setup
for (uint32_t blackholeNode : blackholeNodes) {
    if (blackholeNode >= nodeContainer.GetN()) {
        NS_LOG_ERROR("Blackhole node index out of range: " << blackholeNode);
        continue;
    }

    Ptr<Node> node = nodeContainer.Get(blackholeNode);
    Ptr<BlackholeAodv> blackholeRouting = CreateObject<BlackholeAodv>();
    blackholeRouting->InitializeTrustScores(nodes);
    blackholeRouting->SetDropProbability(0.9); // Drop 90% of packets
    node->AggregateObject(blackholeRouting);

    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    if (!ipv4) {
        NS_LOG_ERROR("IPv4 object not set for Node " << blackholeNode);
        continue;
    }
    blackholeRouting->SetIpv4(ipv4); // Ensure the protocol is linked to IPv4
}



    // Assign IPs
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // Traffic setup
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sourceSocket = Socket::CreateSocket(nodeContainer.Get(1), tid);
    InetSocketAddress dest = InetSocketAddress(interfaces.GetAddress(nodes - 1), 9);
    sourceSocket->Connect(dest);

    Ptr<Socket> recvSocket = Socket::CreateSocket(nodeContainer.Get(nodes - 1), tid);
    recvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 9));
    recvSocket->SetRecvCallback(MakeCallback(&ReceivePacket));

    double interval = 1.0 / trafficRate;
    for (uint32_t i = 0; i < trafficRate * simTime; ++i) {
        Simulator::Schedule(Seconds(i * interval), [sourceSocket]() {
            Ptr<Packet> packet = Create<Packet>(1024);
            TimestampTag timestamp;
            timestamp.SetTimestamp(Simulator::Now());
            packet->AddPacketTag(timestamp);
            sourceSocket->Send(packet);
            totalSentPackets++;
        });
    }

	// Schedule PeriodicTrustLogging
	Simulator::Schedule(Seconds(5.0), &PeriodicTrustLogging, nodeContainer, Seconds(5.0));

	// Install FlowMonitor
	FlowMonitorHelper flowmonHelper;
	Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();
	if (!monitor) {
	    NS_LOG_ERROR("Failed to install FlowMonitor.");
	} else {
	    NS_LOG_INFO("FlowMonitor successfully installed.");
	}

	// Stop simulation after simTime
	Simulator::Stop(Seconds(simTime));

	// Run simulation
	NS_LOG_INFO("Starting simulation...");
	Simulator::Run();
	NS_LOG_INFO("Simulation finished.");

	// Log simulation statistics
	NS_LOG_INFO("Logging simulation statistics...");
	LogStatistics(nodes, simTime);

	// Serialize FlowMonitor results to XML
	NS_LOG_INFO("Serializing FlowMonitor results...");
	try {
	    monitor->SerializeToXmlFile("flowmon-results.xml", true, true);
	    NS_LOG_INFO("FlowMonitor results successfully serialized to flowmon-results.xml.");
	} catch (const std::exception &e) {
	    NS_LOG_ERROR("Failed to serialize FlowMonitor results: " << e.what());
	}

	// Destroy simulation
	Simulator::Destroy();
	NS_LOG_INFO("Simulation destroyed successfully.");
	return 0;
}
