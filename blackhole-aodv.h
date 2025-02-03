#ifndef BLACKHOLE_AODV_H
#define BLACKHOLE_AODV_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include <cstdint>

namespace ns3 {

class BlackholeAodv : public Ipv4RoutingProtocol {
public:
    static TypeId GetTypeId(void);

    BlackholeAodv();
    virtual ~BlackholeAodv();

    // Inherited from Ipv4RoutingProtocol
    virtual Ptr<Ipv4Route> RouteOutput(Ptr<Packet> packet,
                                       const Ipv4Header &header,
                                       Ptr<NetDevice> oif,
                                       Socket::SocketErrno &sockerr) override;

    virtual bool RouteInput(Ptr<const Packet> packet,
                            const Ipv4Header &header,
                            Ptr<const NetDevice> idev,
                            const UnicastForwardCallback &ucb,
                            const MulticastForwardCallback &mcb,
                            const LocalDeliverCallback &lcb,
                            const ErrorCallback &ecb) override;

    virtual void NotifyInterfaceUp(uint32_t interface) override;
    virtual void NotifyInterfaceDown(uint32_t interface) override;
    virtual void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    virtual void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    virtual void SetIpv4(Ptr<Ipv4> ipv4) override;
    virtual void PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const override;

    void SetDropProbability(double probability);
    double GetDropProbability() const;

    uint32_t GetTotalDroppedPackets() const;

    // Declaration of GetTotalForwardedPackets
    uint32_t GetTotalForwardedPackets() const;

private:
    Ptr<Ipv4> m_ipv4;
    Ptr<UniformRandomVariable> m_randomVar;
    uint32_t totalDroppedPackets; // Tracks the total number of dropped packets
    uint32_t totalForwardedPackets; // Tracks the total number of forwarded packets
    double dropProbability; // Probability of dropping a packet
};

} // namespace ns3

#endif // BLACKHOLE_AODV_H
