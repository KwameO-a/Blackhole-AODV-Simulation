#include "ns3/node.h"           // For Node class
#include "ns3/double.h"         // For DoubleValue
#include "ns3/attribute.h"      // For MakeDoubleAccessor and MakeDoubleChecker
#include "ns3/log.h"            // For logging macros
#include "ns3/simulator.h"      // For simulation utilities
#include "ns3/ipv4-routing-protocol.h" // For INTERFACE_NOT_FOUND
#include "blackhole-aodv.h"     // The header file for this implementation
#include <fstream>               // For file operations
#include "ns3/node-container.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BlackholeAodv");

// Trust score thresholds
const double TRUST_THRESHOLD = 0.3;  // Blacklist threshold
const double RECOVERY_THRESHOLD = 0.6;  // Recovery threshold (adjusted for robustness)
const double ADAPTIVE_DROP_RATE = 0.05;  // Drop rate adaptation factor

const std::set<uint32_t>& BlackholeAodv::GetBlacklistedNodes() const {
    return blacklistedNodes;
}

const std::map<uint32_t, double>& BlackholeAodv::GetTrustScores() const {
    return trustScores;
}

// Constructor
BlackholeAodv::BlackholeAodv()
    : m_randomVar(CreateObject<UniformRandomVariable>()),
      totalDroppedPackets(0),
      totalForwardedPackets(0),
      dropProbability(0.05) { // Start with an adaptive drop probability
    m_randomVar->SetAttribute("Min", DoubleValue(0.0));
    m_randomVar->SetAttribute("Max", DoubleValue(1.0));
    NS_LOG_INFO("BlackholeAodv initialized with default drop probability = " << dropProbability);
}

// Destructor
BlackholeAodv::~BlackholeAodv() {}

TypeId BlackholeAodv::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::BlackholeAodv")
        .SetParent<Ipv4RoutingProtocol>()
        .AddConstructor<BlackholeAodv>()
        .AddAttribute("DropProbability",
                      "Probability of dropping packets (0.0 to 1.0)",
                      DoubleValue(0.05),
                      MakeDoubleAccessor(&BlackholeAodv::dropProbability),
                      MakeDoubleChecker<double>(0.0, 1.0));
    return tid;
}

void BlackholeAodv::SetDropProbability(double probability) {
    if (probability < 0.0 || probability > 1.0) {
        NS_LOG_WARN("Invalid drop probability. Retaining current value = " << dropProbability);
    } else {
        dropProbability = probability;
        NS_LOG_INFO("Drop probability updated to " << dropProbability);
    }
}

void BlackholeAodv::InitializeTrustScores(uint32_t totalNodes) {
    for (uint32_t i = 0; i < totalNodes; ++i) {
        trustScores[i] = 1.0;  // Start with maximum trust
    }
    NS_LOG_INFO("Initialized trust scores for all nodes.");
}

Ptr<Ipv4Route> BlackholeAodv::RouteOutput(Ptr<Packet> packet,
                                          const Ipv4Header &header,
                                          Ptr<NetDevice> oif,
                                          Socket::SocketErrno &sockerr) {
    NS_LOG_WARN("BlackholeAodv: RouteOutput called but not supported.");
    sockerr = Socket::ERROR_NOROUTETOHOST;
    return nullptr;
}

bool BlackholeAodv::RouteInput(Ptr<const Packet> packet,
                               const Ipv4Header &header,
                               Ptr<const NetDevice> device,
                               const UnicastForwardCallback &ucb,
                               const MulticastForwardCallback &mcb,
                               const LocalDeliverCallback &lcb,
                               const ErrorCallback &ecb) {
    // Extract destination node ID
    uint32_t destNodeId = header.GetDestination().Get();

    // Validate IPv4 object before proceeding
    if (!m_ipv4) {
        NS_LOG_ERROR("IPv4 object not set in BlackholeAodv! Dropping packet.");
        return false; // Drop packet due to invalid state
    }

    // Get the interface index for the incoming device
    int32_t interfaceIndex = m_ipv4->GetInterfaceForDevice(device);
    if (interfaceIndex == -1) {
        NS_LOG_ERROR("Invalid interface index for the incoming device! Dropping packet.");
        return false;
    }

    // Check if the destination node is blacklisted
    if (blacklistedNodes.find(destNodeId) != blacklistedNodes.end()) {
        if (m_randomVar->GetValue() < dropProbability) {
            totalDroppedPackets++;
            UpdateTrustScore(destNodeId, true); // Penalize trust score
            NS_LOG_WARN("Packet dropped by Blacklisted Node: Node " << destNodeId);
            return false; // Drop packet
        } else {
            NS_LOG_INFO("Blacklisted Node " << destNodeId << " forwarded packet (adaptive behavior).");
        }
    }

    // If this is the final destination, deliver locally
    if (m_ipv4->IsDestinationAddress(header.GetDestination(), static_cast<uint32_t>(interfaceIndex))) {
        if (!lcb.IsNull()) {
            lcb(packet, header, static_cast<uint32_t>(interfaceIndex));
            NS_LOG_INFO("Packet delivered locally to Node " << destNodeId);
            return true; // Successfully delivered locally
        } else {
            NS_LOG_ERROR("LocalDeliverCallback not set! Packet cannot be delivered.");
            return false;
        }
    }

    // Update metrics and trust score for forwarding
    totalForwardedPackets++;
    UpdateTrustScore(destNodeId, false); // Reward trust score

    // Attempt to forward the packet
    if (!ucb.IsNull()) {
        Ptr<Ipv4Route> route = Create<Ipv4Route>();
        route->SetDestination(header.GetDestination());
        route->SetSource(m_ipv4->GetAddress(static_cast<uint32_t>(interfaceIndex), 0).GetLocal());
        route->SetOutputDevice(m_ipv4->GetNetDevice(static_cast<uint32_t>(interfaceIndex)));
        ucb(route, packet, header); // Correctly invoke UnicastForwardCallback
        NS_LOG_INFO("Packet forwarded successfully to Node " << destNodeId);
        return true; // Successfully forwarded
    } else {
        NS_LOG_ERROR("UnicastForwardCallback not set! Packet cannot be forwarded.");
        return false; // Unable to forward
    }
}

void BlackholeAodv::UpdateTrustScore(uint32_t nodeId, bool dropped) {
    if (trustScores.find(nodeId) == trustScores.end()) {
        trustScores[nodeId] = 1.0; // Initialize trust score
    }

    double change = (dropped) ? -0.2 : +0.1; // Penalize drops more than reward
    trustScores[nodeId] += change;
    trustScores[nodeId] = std::clamp(trustScores[nodeId], 0.0, 1.0);

    NS_LOG_INFO("Node " << nodeId << (dropped ? " penalized" : " rewarded")
                << ". Trust Score = " << trustScores[nodeId]);

    if (trustScores[nodeId] < TRUST_THRESHOLD) {
        if (blacklistedNodes.insert(nodeId).second) {
            NS_LOG_INFO("Node " << nodeId << " added to blacklist.");
        }
    } else if (trustScores[nodeId] >= RECOVERY_THRESHOLD) {
        if (blacklistedNodes.erase(nodeId)) {
            NS_LOG_INFO("Node " << nodeId << " removed from blacklist.");
        }
    }
}


void BlackholeAodv::LogTrustScores() const {
    NS_LOG_INFO("Executing LogTrustScores...");
    std::ofstream trustLog("/home/uwe/ns3/ns-allinone-3.43/ns-3.43/trust_scores.csv", std::ios_base::app);
    if (!trustLog.is_open()) {
        NS_LOG_ERROR("Failed to open trust_scores.csv for writing!");
        return;
    }
    static bool headerWritten = false;
    if (!headerWritten) {
        trustLog << "Time,NodeID,TrustScore\n";
        headerWritten = true;
    }

    for (const auto &entry : trustScores) {
        NS_LOG_INFO("Node " << entry.first << ": Trust Score = " << entry.second);
        trustLog << Simulator::Now().GetSeconds() << "," << entry.first << "," << entry.second << "\n";
    }
    for (uint32_t nodeId : blacklistedNodes) {
        NS_LOG_INFO("Blacklisted Node: " << nodeId);
        trustLog << Simulator::Now().GetSeconds() << ",BlacklistedNode," << nodeId << "\n";
    }
    trustLog.close();
    NS_LOG_INFO("LogTrustScores completed.");
}

//NS_LOG_COMPONENT_DEFINE("PeriodicTrustLogging");
void PeriodicTrustLogging(const ns3::NodeContainer &nodeContainer, ns3::Time interval) {
    // Attempt to open the log file
    std::ofstream testLog("/home/uwe/ns3/ns-allinone-3.43/ns-3.43/test_log.txt", std::ios_base::app);
    if (testLog.is_open()) {
        // Write a log entry with the current simulation time
        testLog << "Testing write in PeriodicTrustLogging at " << Simulator::Now().GetSeconds() << " seconds\n";
        testLog.close();
        NS_LOG_INFO("Test write to test_log.txt successful.");
    } else {
        // Log an error message if the file cannot be opened
        NS_LOG_ERROR("Failed to open test_log.txt for writing in PeriodicTrustLogging!");
    }

    NS_LOG_INFO("PeriodicTrustLogging executed at " << Simulator::Now().GetSeconds() << " seconds");
    
    // Iterate through the nodes and log trust scores
    for (uint32_t i = 0; i < nodeContainer.GetN(); ++i) {
        Ptr<BlackholeAodv> blackholeRouting = nodeContainer.Get(i)->GetObject<BlackholeAodv>();
        if (blackholeRouting) {
            blackholeRouting->LogTrustScores();
            NS_LOG_INFO("Trust scores logged for Node " << i);
        } else {
            NS_LOG_WARN("BlackholeAodv object not found for Node " << i);
        }
    }

    // Schedule the next execution of PeriodicTrustLogging
    ns3::Simulator::Schedule(interval, &PeriodicTrustLogging, nodeContainer, interval);
}





double BlackholeAodv::GetTrustScore(uint32_t nodeId) const {
    auto it = trustScores.find(nodeId);
    return (it != trustScores.end()) ? it->second : 1.0;  // Default trust score
}

void BlackholeAodv::NotifyInterfaceUp(uint32_t interface) {
    NS_LOG_INFO("Interface " << interface << " is up.");
}

void BlackholeAodv::NotifyInterfaceDown(uint32_t interface) {
    NS_LOG_INFO("Interface " << interface << " is down.");
}

void BlackholeAodv::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) {
    NS_LOG_INFO("Address added to interface " << interface << ": " << address);
}

void BlackholeAodv::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) {
    NS_LOG_INFO("Address removed from interface " << interface << ": " << address);
}

void BlackholeAodv::SetIpv4(Ptr<Ipv4> ipv4) {
    m_ipv4 = ipv4;
    NS_LOG_INFO("IPv4 set for BlackholeAodv.");
}

void BlackholeAodv::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {
    *stream->GetStream() << "Routing table not maintained by BlackholeAodv.\n";
}

} // namespace ns3