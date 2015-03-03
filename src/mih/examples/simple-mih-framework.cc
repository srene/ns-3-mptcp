/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */            
/*
 * Copyright (c) 2008 IT-SUDPARIS
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
 * Author: Providence SALUMU M. <Providence.Salumu_Munga@it-sudparis.eu>
 */

// If you want to see what is processed by this script,
// execute the following command from your CLI:
// NS_LOG="SimpleMihUser:SimpleMihLinkSap:MihFunction"
// ./waf --run simple-mih-framework

#include "ns3/core-module.h"
#include "ns3/common-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/mobility-module.h"
#include "ns3/contrib-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mih-module.h"
#include "ns3/mac48-address.h"

using namespace ns3;
using namespace ns3::mih;

int 
main (int argc, char *argv[])
{
  NodeContainer stas; // stations

  stas.Create (1);

  // Add MihFunction to stas(0).
  MihfId mihfId = MihfId ("802.21-local@ns3");

  Ptr<MihFunction> mihf = CreateObject<MihFunction> ("MihfId", MihfIdValue (mihfId));
  stas.Get (0)->AggregateObject (mihf);

  // Add Mih User to stas(0).
  Ptr<SimpleMihUser> mihUser = CreateObject<SimpleMihUser> ();
  stas.Get (0)->AggregateObject (mihUser);

  // Add Mih Link Sap to stas(0).
  Ptr<SimpleMihLinkSap> mihLinkSap = CreateObject<SimpleMihLinkSap> ("MihfId", MihfIdValue (mihfId));
  LinkIdentifier linkId = LinkIdentifier (LinkType (LinkType::WIRELESS_802_11),
                                          Mac48Address ("00:00:00:00:00:82"),
                                          Mac48Address ("00:00:00:00:00:92"));
  mihLinkSap->SetLinkIdentifier (linkId);
  mihf->Register (mihLinkSap);
  
  Simulator::Schedule (Seconds (1.0), &SimpleMihLinkSap::Run, mihLinkSap);

  Simulator::Schedule (Seconds (9.0), &SimpleMihLinkSap::Stop, mihLinkSap);

  Simulator::Schedule (Seconds (5 + 1.0), &SimpleMihUser::LinkGetParameters, mihUser, mihfId, linkId);

  Simulator::Schedule (Seconds (5 + 1.5), &SimpleMihUser::LinkConfigureThresholds, mihUser, mihfId, linkId);

  Simulator::Schedule (Seconds (5 + 1.7), &SimpleMihUser::EventSubscribe, mihUser, mihfId, LinkIdentifier ());

  Simulator::Schedule (Seconds (5 + 1.7), &SimpleMihUser::EventSubscribe, mihUser, mihfId, linkId);

  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;                                                                              
}
