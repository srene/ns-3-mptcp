#ifndef MP_INTERNET_STACK_HELPER_H
#define MP_INTERNET_STACK_HELPER_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "internet-trace-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"

namespace ns3 {

class Node;
class Ipv4RoutingHelper;
class Ipv6RoutingHelper;

/**
 * \brief aggregate IP/TCP/UDP functionality to existing Nodes.
 */
class MpInternetStackHelper
{
public:
  MpInternetStackHelper(void);
  ~MpInternetStackHelper(void);
  void CreateAndAggregateObjectFromTypeId (Ptr<Node> node, const std::string typeId);
  void Install(NodeContainer nodes);

  /**
   * \param routing a new routing helper
   *
   * Set the routing helper to use during Install. The routing
   * helper is really an object factory which is used to create
   * an object of type ns3::Ipv4RoutingProtocol per node. This routing
   * object is then associated to a single ns3::Ipv4 object through its
   * ns3::Ipv4::SetRoutingProtocol.
   */
  void SetRoutingHelper (const Ipv4RoutingHelper &routing);

private:
    Ipv4RoutingHelper *m_routing;

};

} // namespace ns3

#endif /* MP_INTERNET_STACK_HELPER_H */
