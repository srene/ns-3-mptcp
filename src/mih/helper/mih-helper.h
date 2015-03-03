/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Sergi Reñé
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Sergi Reñé <sergi.rene@entel.upc.edu>
 */

#ifndef MIH_HELPER_H
#define MIH_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/mih-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/core-module.h"
//#include "ns3/common-module.h"
//#include "ns3/node-module.h"
//#include "ns3/helper-module.h"
#include "ns3/address.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mih-module.h"
#include "ns3/mac48-address.h"
#include "ns3/global-route-manager.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/ptr.h"

namespace ns3 {
namespace mih{
/**
 * \ingroup MIH
 * \brief Helper class that adds MIH routing to nodes.
 */
class MihHelper: public Object
{
public:
  MihHelper ();
  ~MihHelper ();
  /**
   * \internal
   * \returns pointer to clone of this MIHHelper
   *
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  MihHelper* Copy (void) const;


  void AddDestinationEntry(MihfId mihfId, InetSocketAddress address);
  MihfId getMihfId();
  //InetSocketAddress getAddress();
  Ptr<SimpleMihUser> getUser();
  Ptr<WifiMihLinkSap> getSap();
  Ptr<MihProtocol> getProtocol();
  LinkIdentifier getLinkId();
  void SetUser(Ptr<SimpleMihUser> mihUser);
  void wifiInstall (std::string aName, Ptr<Node> b, Ptr<NetDevice> staDevice, Ptr<NetDevice> apDevice, Ipv4Address address) ;

  /**
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set.
   *
   * This method controls the attributes of ns3::dsdv::RoutingProtocol
   */
  void Set (std::string name, const AttributeValue &value);

private:
  ObjectFactory m_mihProtocolFactory;
  MihfId m_mihfId;
  LinkIdentifier m_linkId;
  Ptr<MihFunction> m_mihf;
  Ptr<SimpleMihUser> m_mihUser;
  Ptr<WifiMihLinkSap> m_mihLinkSap;
  Ptr<MihProtocol> m_mihProtocol;
  Ptr<Node> m_node;
  //InetSocketAddress m_addrSta;
};

}
}
#endif /* MIH_HELPER_H */
