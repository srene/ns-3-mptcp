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

#include "mih-helper.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"


namespace ns3 {
namespace mih {
MihHelper::~MihHelper ()
{
}

MihHelper::MihHelper ()
{
	  m_mihProtocolFactory = ObjectFactory ();
	  m_mihProtocolFactory.SetTypeId ("ns3::mih::MihProtocol");
	  m_mihProtocolFactory.Set ("Protocol", TypeIdValue (UdpSocketFactory::GetTypeId ()));
}

MihHelper*
MihHelper::Copy (void) const
{
  return new MihHelper (*this);
}

void
MihHelper::AddDestinationEntry(MihfId mihfId, InetSocketAddress address)
{
	m_mihProtocol->AddDestinationEntry (mihfId, address);
}

MihfId
MihHelper::getMihfId()
{
	return m_mihfId;
}

/*InetSocketAddress
MihHelper::getAddress()
{
	return m_addrSta;
}*/

Ptr<SimpleMihUser> MihHelper::getUser()
{
	return m_mihUser;
}

Ptr<WifiMihLinkSap> MihHelper::getSap()
{
	return m_mihLinkSap;
}

Ptr<MihProtocol> MihHelper::getProtocol()
{
	return m_mihProtocol;
}
LinkIdentifier MihHelper::getLinkId()
{
	return m_linkId;
}
void MihHelper::SetUser(Ptr<SimpleMihUser> mihUser)
{
	m_mihUser = mihUser;
    m_node->AggregateObject (mihUser);
}

void
MihHelper::wifiInstall (std::string aName, Ptr<Node> b, Ptr<NetDevice> staDevice, Ptr<NetDevice> apDevice, Ipv4Address address)
{
	  //m_mihProtocolFactory = ObjectFactory ();
	  m_mihfId = MihfId (aName);
      m_node = b;
	  // Add MihFunction
	  m_mihf = CreateObject<MihFunction> ();
	  m_mihf->SetMihfId(m_mihfId);
	  m_node->AggregateObject (m_mihf);

	  // Add Mih User to stas(0).
	  //m_mihUser = CreateObject<SimpleMihUser> ();
	  //m_node->AggregateObject (m_mihUser);

	  m_mihLinkSap = CreateObject<WifiMihLinkSap> ();
	  m_mihLinkSap->SetAttribute("MihfId", MihfIdValue (m_mihfId));
	  m_mihLinkSap->SetWifiNetDevice(staDevice);
	  m_mihLinkSap->SetNode(b);
	  m_linkId = LinkIdentifier (LinkType (LinkType::WIRELESS_802_11),
											   staDevice->GetAddress (),
											   apDevice->GetAddress ());
	  m_mihLinkSap->SetLinkIdentifier (m_linkId);
	  //  mihLinkSap0->SetAttribute ("PowerUp", BooleanValue (false));
	  m_mihf->Register (m_mihLinkSap);
	  staDevice->AggregateObject (m_mihLinkSap);

	  InetSocketAddress m_addrSta = InetSocketAddress (address, 8282);


	  m_mihProtocolFactory.Set ("Local", AddressValue (m_addrSta));
	  m_mihProtocolFactory.Set ("Node", PointerValue (m_node));
	  m_mihProtocol = m_mihProtocolFactory.Create<MihProtocol> ();
	  m_mihProtocol->Init (); // Create server socket;

	  m_node->AggregateObject (m_mihProtocol);
}

void
MihHelper::Set (std::string name, const AttributeValue &value)
{
	m_mihProtocolFactory.Set (name, value);

}

}
}
