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

#include "ns3/core-module.h"
#include "ns3/common-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/mobility-module.h"
#include "ns3/contrib-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mih-module.h"
#include "ns3/mac48-address.h"
#include "ns3/global-route-manager.h"

using namespace ns3;
using namespace ns3::mih;

int 
main (int argc, char *argv[])
{
   Packet::EnablePrinting ();
   //  enable rts cts all the time.
   Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
   //  disable fragmentation
   Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));

  WifiHelper wifi;
  MobilityHelper mobility;
  NodeContainer stas; // stations
  NodeContainer ap; // access point
  NetDeviceContainer staDevs;
  NetDeviceContainer apDevs;

  stas.Create (2);
  ap.Create (1);

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  Ssid ssid = Ssid ("wifi-default");
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
  // setup stas.
  wifi.SetMac ("ns3::NqstaWifiMac", 
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));
  staDevs = wifi.Install (wifiPhy, stas);
  // setup ap.
  wifi.SetMac ("ns3::NqapWifiMac", "Ssid", SsidValue (ssid),
               "BeaconGeneration", BooleanValue (true),
               "BeaconInterval", TimeValue (Seconds (2.5)));
  apDevs = wifi.Install (wifiPhy, ap);

  // mobility.
  mobility.Install (stas);
  mobility.Install (ap);

  // Add an ip stack on all nodes;
  InternetStackHelper ipStack;
  ipStack.Install (stas);

  // assign ip addresses
  Ipv4AddressHelper ip;
  ip.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer addresses = ip.Assign (staDevs);

  InetSocketAddress addrSta0 = InetSocketAddress (addresses.GetAddress (0), 8282);
  InetSocketAddress addrSta1 = InetSocketAddress (addresses.GetAddress (1), 8282);

  // std::cout << addrSta0 << " & " << addrSta1 << std::endl;

  // MihProtocol factory;
  ObjectFactory mihProtocolFactory = ObjectFactory ();
  mihProtocolFactory.SetTypeId ("ns3::mih::MihProtocol");
  mihProtocolFactory.Set ("Protocol", TypeIdValue (UdpSocketFactory::GetTypeId ()));

  // STA-0

  MihfId mihfId0 = MihfId ("802.21-sta-0@ns3");
  MihfId mihfId1 = MihfId ("802.21-sta-1@ns3");


  // Add MihFunction to stas(0).
  Ptr<MihFunction> mihf0 = CreateObject<MihFunction> ("MihfId", MihfIdValue (mihfId0));
  stas.Get (0)->AggregateObject (mihf0);

  // Add Mih User to stas(0).
  Ptr<SimpleMihUser> mihUser0 = CreateObject<SimpleMihUser> ();
  stas.Get (0)->AggregateObject (mihUser0);

  // Add Mih Link Sap to stas(0).
  Ptr<SimpleMihLinkSap> mihLinkSap0 = CreateObject<SimpleMihLinkSap> ("MihfId", MihfIdValue (mihfId0));
  LinkIdentifier linkId0 = LinkIdentifier (LinkType (LinkType::WIRELESS_802_11),
                                           staDevs.Get (0)->GetAddress (),
                                           apDevs.Get (0)->GetAddress ());
  mihLinkSap0->SetLinkIdentifier (linkId0);
  //  mihLinkSap0->SetAttribute ("PowerUp", BooleanValue (false));
  mihf0->Register (mihLinkSap0);
  staDevs.Get (0)->AggregateObject (mihLinkSap0);

  mihProtocolFactory.Set ("Local", AddressValue (addrSta0));
  mihProtocolFactory.Set ("Node", PointerValue (stas.Get (0)));
  Ptr<MihProtocol> mihProtocol0 = mihProtocolFactory.Create<MihProtocol> ();
  mihProtocol0->Init (); // Create server socket;
  // Resolve MihfId <==> Address
  mihProtocol0->AddDestinationEntry (mihfId0, addrSta0);
  mihProtocol0->AddDestinationEntry (mihfId1, addrSta1);
  stas.Get (0)->AggregateObject (mihProtocol0);
  
  // STA-1


  // Add MihFunction to stas(1).
  Ptr<MihFunction> mihf1 = CreateObject<MihFunction> ("MihfId", MihfIdValue (mihfId1));
  stas.Get (1)->AggregateObject (mihf1);

  // Add Mih User to stas(1).
  Ptr<SimpleMihUser> mihUser1 = CreateObject<SimpleMihUser> ();
  stas.Get (1)->AggregateObject (mihUser1);

  // Add Mih Link Sap to stas(1).
  Ptr<SimpleMihLinkSap> mihLinkSap1 = CreateObject<SimpleMihLinkSap> ("MihfId", MihfIdValue (mihfId1));
  LinkIdentifier linkId1 = LinkIdentifier (LinkType (LinkType::WIRELESS_802_11),
                                           staDevs.Get (1)->GetAddress (),
                                           apDevs.Get (0)->GetAddress ());
  mihLinkSap1->SetLinkIdentifier (linkId1);
  mihf1->Register (mihLinkSap1);
  staDevs.Get (1)->AggregateObject (mihLinkSap1); 

  mihProtocolFactory.Set ("Local", AddressValue (addrSta1));
  mihProtocolFactory.Set ("Node", PointerValue (stas.Get (1)));
  Ptr<MihProtocol> mihProtocol1 = mihProtocolFactory.Create<MihProtocol> ();
  mihProtocol1->Init (); // Create server socket;
  // Resolve MihfId <==> Address
  mihProtocol1->AddDestinationEntry (mihfId0, addrSta0);
  mihProtocol1->AddDestinationEntry (mihfId1, addrSta1);
  stas.Get (1)->AggregateObject (mihProtocol1);

  YansWifiPhyHelper::EnablePcap ("remote-mih-framework", stas.Get (0));
  YansWifiPhyHelper::EnablePcap ("remote-mih-framework", stas.Get (1));
  
  Simulator::Schedule (Seconds (0.5), &SimpleMihUser::Start, mihUser0);

  Simulator::Schedule (Seconds (0.5), &SimpleMihUser::Start, mihUser1);

  // Simulator::Schedule (Seconds (1.0), &SimpleMihLinkSap::Run, mihLinkSap0);
  Simulator::Schedule (Seconds (4.0), &SimpleMihLinkSap::Run, mihLinkSap1);

  Simulator::Schedule (Seconds (9.0), &SimpleMihLinkSap::Stop, mihLinkSap0);
  Simulator::Schedule (Seconds (9.0), &SimpleMihLinkSap::Stop, mihLinkSap1);
  
  Simulator::Schedule (Seconds (1.0), &SimpleMihUser::Register, mihUser0, mihfId1);

  // Simulator::Schedule (Seconds (1.0), &SimpleMihUser::Register, mihUser1, mihfId0);

  // Simulator::Schedule (Seconds (2.0), &SimpleMihUser::LinkGetParameters, mihUser0, mihfId1, linkId1);
  //  Simulator::Schedule (Seconds (5 + 1.0), &SimpleMihUser::LinkGetParameters, mihUser1, mihfId1, linkId1);
  
  // Simulator::Schedule (Seconds (3.0), &SimpleMihUser::LinkConfigureThresholds, mihUser0, mihfId1, linkId1);
  //  Simulator::Schedule (Seconds (4.5 + 1.5), &SimpleMihUser::LinkConfigureThresholds, mihUser1, mihfId1, linkId1);
  
  // Simulator::Schedule (Seconds (4.0), &SimpleMihUser::EventSubscribe, mihUser0, mihfId0, LinkIdentifier ());
  // Simulator::Schedule (Seconds (2 + 1.7), &SimpleMihUser::EventSubscribe, mihUser1, mihfId1, LinkIdentifier ());
  
  //  Simulator::Schedule (Seconds (5 + 1.7), &SimpleMihUser::EventSubscribe, mihUser1, mihfId0, linkId0);
  Simulator::Schedule (Seconds (5.0), &SimpleMihUser::EventSubscribe, mihUser0, mihfId1, linkId1);

  // Simulator::Schedule (Seconds (4.0), &SimpleMihUser::LinkActions, mihUser0, mihfId0, linkId0);

  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;                                                                              
}
