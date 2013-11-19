/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

//
// Network topology
//
//           10Mb/s, 10ms       10Mb/s, 10ms
//       n0-----------------n1-----------------n2
//
//
// - Tracing of queues and packet receptions to file
//   "tcp-large-transfer.tr"
// - pcap traces also generated in the following files
//   "tcp-large-transfer-$n-$i.pcap" where n and i represent node and interface
// numbers respectively
//  Usage (e.g.): ./waf --run tcp-large-transfer


#include <ctype.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <string>
#include <vector>


#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkP2p");
std::string congestion;
uint32_t recv;
Time prevTime;
double currTime;
std::vector<Ipv4InterfaceContainer> ipv4Ints;

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  //*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;

}



void
Throughput (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &address)
{
	//NS_LOG_FUNCTION(socket);
	NS_LOG_INFO("Received packet at " << Simulator::Now().GetSeconds() << " " << packet->GetSize() << " " << address << " " << currTime);

	recv += packet->GetSize();
	currTime += (Simulator::Now().GetSeconds() - prevTime.GetSeconds());

	if (currTime >= 0.1) {
		*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << (recv*8)/currTime << std::endl;
		recv = 0;
		currTime = 0;
	}
	prevTime=Simulator::Now ();
}

void
TotalRx(Ptr<PacketSink> sink, std::string protocol)
{
	std::cout << "Total RX " << sink->GetTotalRx () << " " << Simulator::Now().GetSeconds() << std::endl;
	std::cout << "Total Throughput " << protocol << " : " << ((double)sink->GetTotalRx ()*8)/(Simulator::Now().GetSeconds()*1000000) << " Mbps" << std::endl;
}

void Join(ApplicationContainer sourceApps);
void Trace(ApplicationContainer sourceApps, Ptr<PacketSink> sink);


int main (int argc, char *argv[])
{

	   Config::SetDefault ("ns3::TcpSocketBase::SegmentSize", UintegerValue (1400));
	   Config::SetDefault ("ns3::BulkSendApplication::SendSize", UintegerValue (1400));

  CommandLine cmd;
  cmd.Parse (argc, argv);
	currTime=0;
	recv=0;
	prevTime=Simulator::Now ();
  double stop = 50.0;
  int sf = 2; // number of subflows

  uint32_t maxBytes = 200000;
  congestion="ns3::TcpMpTcp";
  //congestion="ns3::TcpNewReno";

  // Here, we will explicitly create three nodes.  The first container contains
  // nodes 0 and 1 from the diagram above, and the second one contains nodes
  // 1 and 2.  This reflects the channel connectivity, and will be used to
  // install the network interfaces and connect them with a channel.

  Ptr<Node> client;
  Ptr<Node> server;
   NodeContainer nodes;
   nodes.Create(2);
   client = nodes.Get(0);
   server = nodes.Get(1);


   // Now add ip/tcp stack to all nodes.
   InternetStackHelper internet;
   internet.InstallAll ();

   for(int i=0; i < sf; i++)
   {
       // Creation of the point to point link between hots
       PointToPointHelper p2plink;
       if(i==0){
           p2plink.SetDeviceAttribute ("DataRate",  StringValue ("1Mbps"));
    	   p2plink.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(1)));

       }
       else if(i==1){
           p2plink.SetDeviceAttribute ("DataRate",  StringValue ("500Kbps"));
    	   p2plink.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(10)));
       }



       NetDeviceContainer netDevices;
       netDevices = p2plink.Install(nodes);

       netDevices.Get (0)->SetMtu (1500);
       netDevices.Get (1)->SetMtu (1500);
       // Attribution of the IP addresses
       std::stringstream netAddr;
       netAddr << "10.1." << (i+1) << ".0";
       std::string str = netAddr.str();

       Ipv4AddressHelper ipv4addr;
       ipv4addr.SetBase(str.c_str(), "255.255.255.0");
       Ipv4InterfaceContainer interface = ipv4addr.Assign(netDevices);
       ipv4Ints.insert(ipv4Ints.end(), interface);
       std::stringstream pcap;
       pcap << "mptcp" << i;

       p2plink.EnablePcap ("mptcp",nodes);



   }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  NS_LOG_INFO ("Create Applications.");

//
// Create a BulkSendApplication and install it on node 0
//
  uint16_t port = 50000;  // well-known echo port number


  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (ipv4Ints[0].GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  source.SetAttribute ("SocketType",StringValue("ns3::MpTcpSocket"));
  source.SetAttribute ("Congestion",StringValue(congestion));
  ApplicationContainer sourceApps = source.Install (client);
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (stop));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  sink.SetAttribute ("SocketType",StringValue("ns3::MpTcpSocket"));
  sink.SetAttribute ("Congestion",StringValue(congestion));
  ApplicationContainer sinkApps = sink.Install (server);
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (stop));


  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;

  Simulator::Schedule (Seconds(0.05),&Join, sourceApps);

  Simulator::Schedule (Seconds(0.5),&Trace, sourceApps,sink1);
  Simulator::Schedule (Seconds(stop),&TotalRx, sink1, "TCP");

//
// Set up tracing if enabled
  Simulator::Stop (Seconds (stop+5));
  Simulator::Run ();
  Simulator::Destroy ();

  // Trace changes to the congestion window
  //Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));


}

void Join(ApplicationContainer sourceApps)
{
	  Ptr<BulkSendApplication> bulk = DynamicCast<BulkSendApplication> (sourceApps.Get (0));
	  Ptr<MpTcpSocket> mpSocket = bulk->GetSocket()->GetObject<MpTcpSocket>();
	  Simulator::ScheduleNow (&MpTcpSocket::Join, mpSocket,InetSocketAddress(ipv4Ints[1].GetAddress(1),50000),TcpWestwood::GetTypeId());

}


void Trace(ApplicationContainer sourceApps, Ptr<PacketSink> sink)
{
	Ptr<BulkSendApplication> bulk = DynamicCast<BulkSendApplication> (sourceApps.Get (0));
	Ptr<TcpSocket> localSocket = bulk->GetSocket()->GetObject<TcpSocket>();
	std::vector<Ptr<TcpSocketBase> > m_sockets = localSocket->GetSocketsBase();
	int i = 0;

	for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin() ; it != m_sockets.end(); ++it)
	{
		AsciiTraceHelper asciiTraceHelper;

		std::ostringstream oss1;
		oss1 << "node" << i <<".cwnd";
		Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (oss1.str());
		(*it)->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

	}

}

