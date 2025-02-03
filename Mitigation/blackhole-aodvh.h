#ifndef BLACKHOLE_AODV_H
#define BLACKHOLE_AODV_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4.h"
#include <map>
#include <set>
#include <cstdint>

namespace ns3 {

/**
 * BlackholeAodv simulates a blackhole attack in a network and provides mechanisms
 * to mitigate its effects using trust scores and blacklisting.
 */
class BlackholeAodv : public Ipv4RoutingProtocol {
public:
    static TypeId GetTypeId(void);

    BlackholeAodv();
    virtual ~BlackholeAodv();

    // Public Methods

    /**
     * @brief Set the probability of dropping packets.
     * @param probability A double value between 0.0 and 1.0.
     */
    void SetDropProbability(double probability);

    /**
     * @brief Initialize trust scores for all nodes in the network.
     * @param totalNodes The total number of nodes in the network.
     */
    void InitializeTrustScores(uint32_t totalNodes);

    /**
     * @brief Update the trust score of a node based on its behavior.
     * @param nodeId The ID of the node to update.
     * @param dropped True if the node dropped a packet, false otherwise.
     */
    void UpdateTrustScore(uint32_t nodeId, bool dropped);

    /**
     * @brief Get the trust score of a node.
     * @param nodeId The ID of the node.
     * @return The trust score of the node.
     */
    double GetTrustScore(uint32_t nodeId) const;

    /**
     * @brief Get the list of blacklisted nodes.
     * @return A constant reference to the set of blacklisted nodes.
     */
    const std::set<uint32_t>& GetBlacklistedNodes() const;
    const std::map<uint32_t, double>& GetTrustScores() const;

    /**
     * @brief Log the trust scores and blacklisted nodes for debugging purposes.
     */
    void LogTrustScores() const;

    // Overridden Methods from Ipv4RoutingProtocol

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

private:
    Ptr<Ipv4> m_ipv4; ///< Pointer to the associated IPv4 object.
    Ptr<UniformRandomVariable> m_randomVar; ///< Random variable for simulating packet drops.

    std::map<uint32_t, double> trustScores; ///< Trust scores for all nodes.
    std::set<uint32_t> blacklistedNodes; ///< Set of blacklisted nodes.

    uint32_t totalDroppedPackets; ///< Total number of packets dropped.
    uint32_t totalForwardedPackets; ///< Total number of packets forwarded.
    double dropProbability; ///< Probability of dropping packets.
};

} // namespace ns3

#endif // BLACKHOLE_AODV_H