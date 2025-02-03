#include "blackhole-aodv.h"
#include "ns3/log.h"
#include "ns3/ipv4-header.h"
#include "ns3/double.h"
#include "ns3/attribute.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BlackholeAodv");

TypeId BlackholeAodv::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::BlackholeAodv")
        .SetParent<Ipv4RoutingProtocol>()
        .AddConstructor<BlackholeAodv>()
        .AddAttribute("DropProbability",
                      "Probability of dropping packets",
                      DoubleValue(1.0), // Default to 90% drop rate
                      MakeDoubleAccessor(&BlackholeAodv::dropProbability),
                      MakeDoubleChecker<double>(0.0, 1.0));
    return tid;
}

BlackholeAodv::BlackholeAodv()
    : totalDroppedPackets(0),
      totalForwardedPackets(0),
      dropProbability(1.0) {
    // Initialize random variable for drop decisions
    m_randomVar = CreateObject<UniformRandomVariable>();
    m_randomVar->SetAttribute("Min", DoubleValue(0.0));
    m_randomVar->SetAttribute("Max", DoubleValue(1.0));
    NS_LOG_INFO("BlackholeAodv: Initialized with drop probability = " << dropProbability);
}

BlackholeAodv::~BlackholeAodv() {}

Ptr<Ipv4Route> BlackholeAodv::RouteOutput(Ptr<Packet>, const Ipv4Header &, Ptr<NetDevice>, Socket::SocketErrno &sockerr) {
    NS_LOG_WARN("BlackholeAodv: RouteOutput called but not supported (returning nullptr).");
    sockerr = Socket::ERROR_NOROUTETOHOST;
    return nullptr; // Blackhole does not provide routes
}

bool BlackholeAodv::RouteInput(Ptr<const Packet> packet,
                               const Ipv4Header &header,
                               Ptr<const NetDevice>,
                               const UnicastForwardCallback &,
                               const MulticastForwardCallback &,
                               const LocalDeliverCallback &,
                               const ErrorCallback &) {
    NS_LOG_INFO("BlackholeAodv: Packet from " << header.GetSource() 
                << " to " << header.GetDestination());

    double randomValue = m_randomVar->GetValue();
    NS_LOG_INFO("Random drop value: " << randomValue << " (Drop Probability: " << dropProbability << ")");

    if (randomValue <= dropProbability) {
        totalDroppedPackets++;
        NS_LOG_WARN("BlackholeAodv: Dropped packet from " << header.GetSource() 
                    << " to " << header.GetDestination());
        return false; // Drop the packet
    }

    totalForwardedPackets++;
    NS_LOG_INFO("BlackholeAodv: Forwarded packet from " << header.GetSource() 
                << " to " << header.GetDestination());
    return true; // Forward the packet
}


void BlackholeAodv::SetDropProbability(double probability) {
    if (probability < 0.0 || probability > 1.0) {
        NS_LOG_WARN("BlackholeAodv: Invalid drop probability. Retaining previous value = " << dropProbability);
    } else {
        dropProbability = probability;
        NS_LOG_INFO("BlackholeAodv: Drop probability updated to " << dropProbability);
    }
}

double BlackholeAodv::GetDropProbability() const {
    return dropProbability;
}

uint32_t BlackholeAodv::GetTotalDroppedPackets() const {
    return totalDroppedPackets;
}

uint32_t BlackholeAodv::GetTotalForwardedPackets() const {
    return totalForwardedPackets;
}

void BlackholeAodv::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {
    *stream->GetStream() << "BlackholeAodv: Routing table not maintained.\n";
}

void BlackholeAodv::NotifyInterfaceUp(uint32_t interface) {
    NS_LOG_INFO("BlackholeAodv: Interface " << interface << " is up.");
}

void BlackholeAodv::NotifyInterfaceDown(uint32_t interface) {
    NS_LOG_INFO("BlackholeAodv: Interface " << interface << " is down.");
}

void BlackholeAodv::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) {
    NS_LOG_INFO("BlackholeAodv: Address added to interface " << interface 
                << ": " << address);
}

void BlackholeAodv::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) {
    NS_LOG_INFO("BlackholeAodv: Address removed from interface " << interface 
                << ": " << address);
}

void BlackholeAodv::SetIpv4(Ptr<Ipv4> ipv4) {
    m_ipv4 = ipv4;
    NS_LOG_INFO("BlackholeAodv: IPv4 set for this protocol.");
}

} // namespace ns3
