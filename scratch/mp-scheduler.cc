#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/error-model.h"
#include "ns3/nal-unit-header.h"
#include "ns3/rtp-protocol.h"
#include "ns3/nstime.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/delay-jitter-estimation.h"
#include "ns3/gnuplot.h"

#include <iostream>
#include <cstring>
#include <cassert>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("mp-scheduler");

typedef std::map<uint32_t,Time> TMap;
typedef std::pair<uint32_t,Time> TPair;
typedef std::map<uint32_t,Time>::iterator TIte;

class MpTcpTest
{

public:
	MpTcpTest ();
	//void Join(Ptr<BulkSendMpApplication> sender);
	void Join();
	void Trace(Ptr<TcpSocket> sock);
	static void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd);
	static void BufferChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd);
	void RxPacket (Ptr<const Packet> p,Ptr<TcpSocketBase> socket);
	static void RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt);
	static void BwChange (Ptr<OutputStreamWrapper> stream, double oldRtt, double newRtt);
	static void LandaChange (Ptr<OutputStreamWrapper> stream, double oldRtt, double newRtt);
	void TxPacket (Ptr<const Packet> p);
	void Drop (Ptr<const Packet> p);
	double Run(uint32_t forward_delay, uint32_t bandwidth, uint32_t sched);
	//void Plot (Gnuplot2dDataset dataset);
private:
	Ptr<BulkSendMpApplication> sender;
	uint32_t maxBytes;
	uint32_t buffer;
	std::string congestion;
	Ptr<OutputStreamWrapper> streamdelay;
	Ptr<OutputStreamWrapper> streamdelay1;
	Ptr<OutputStreamWrapper> streamdelay2;
	Ptr<OutputStreamWrapper> streamdelay3;
	Ptr<OutputStreamWrapper> streamdelay4;
	Ptr<OutputStreamWrapper> streamjitter;
	std::vector<Ptr<TcpSocketBase> > sockets;
	DelayJitterEstimation jitter;
	TMap tMap;
	std::vector<Ipv4InterfaceContainer> ipv4Ints;
	double packetdelay;
	uint32_t packetnum;
};

/* Simple FFMpeg wrapper for encoding/decoding purposes */
MpTcpTest::MpTcpTest ()
{
	maxBytes = 5000000;
	buffer = 375000;
	congestion="ns3::TcpWestwood";
	packetdelay=0;
	packetnum=0;
}
void
MpTcpTest::CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;

}
void
MpTcpTest::BufferChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newCwnd << std::endl;

}

void
MpTcpTest::RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newRtt.GetMilliSeconds() << std::endl;
}

void
MpTcpTest::BwChange (Ptr<OutputStreamWrapper> stream, double oldRtt, double newRtt)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newRtt*8 << std::endl;
}

void
MpTcpTest::LandaChange (Ptr<OutputStreamWrapper> stream, double oldRtt, double newRtt)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << newRtt*8 << std::endl;
}


void
MpTcpTest::TxPacket (Ptr<const Packet> p)
{
  Ptr<Packet> copy = p->Copy();
  RtpProtocol rtpHeader;
  copy->RemoveHeader(rtpHeader);
  NalUnitHeader nalHeader;
  copy->PeekHeader(nalHeader);

  jitter.PrepareTx(p);
  //NS_LOG_UNCOND ("Tx packet " << Simulator::Now ().GetSeconds () << "\t" << rtpHeader << "\t" << nalHeader);
  tMap.insert(TPair(rtpHeader.GetPacketId(),Simulator::Now()));

}

void
MpTcpTest::Drop (Ptr<const Packet> p)
{
  Ptr<Packet> copy = p->Copy();
  RtpProtocol rtpHeader;
  copy->RemoveHeader(rtpHeader);
  NalUnitHeader nalHeader;
  copy->PeekHeader(nalHeader);

  //NS_LOG_UNCOND ("Dropped packet " << Simulator::Now ().GetSeconds () << "\t" << rtpHeader << "\t" << nalHeader);
}


void
MpTcpTest::RxPacket (Ptr<const Packet> p,Ptr<TcpSocketBase> socket)
{
	Ptr<Packet> copy = p->Copy ();
    RtpProtocol rtpHeader;
    copy->RemoveHeader(rtpHeader);
    NalUnitHeader nalHeader;
    copy->RemoveHeader(nalHeader);

    jitter.RecordRx(p);

  //TIte iter = tMap.find(rtpHeader.GetPacketId());

  bool found=false;
  for(uint32_t i = 0;i< sockets.size();i++){
	  if(sockets[i]==socket)found=true;
  }
  if(!found)sockets.push_back(socket);

	  /*if((uint32_t)nalHeader.GetNri()==1){
		  *streamdelay1->GetStream () << Simulator::Now ().GetSeconds () << "\t" << Simulator::Now().GetSeconds()-(MilliSeconds(rtpHeader.GetPacketTimestamp()).GetSeconds()) << std::endl;
	  } else if ((uint32_t)nalHeader.GetNri()==2){
		  *streamdelay2->GetStream () << Simulator::Now ().GetSeconds () << "\t" << Simulator::Now().GetSeconds()-(MilliSeconds(rtpHeader.GetPacketTimestamp()).GetSeconds()) << std::endl;
	  }
	  std::cout << "sockets size " << sockets.size() <<  std::endl;
	  if(sockets.size()>0){
		  std::cout << "sockets size " << socket << " " << sockets[0] << " " << sockets[1] <<  std::endl;
		  if(socket==sockets[0]){
			  *streamdelay3->GetStream () << Simulator::Now ().GetSeconds () << "\t" << Simulator::Now().GetSeconds()-(MilliSeconds(rtpHeader.GetPacketTimestamp()).GetSeconds()) << std::endl;
		  }else if(socket==sockets[1]){
			  *streamdelay4->GetStream () << Simulator::Now ().GetSeconds () << "\t" << Simulator::Now().GetSeconds()-(MilliSeconds(rtpHeader.GetPacketTimestamp()).GetSeconds()) << std::endl;
		  }
	  }
	  *streamjitter->GetStream () << Simulator::Now ().GetSeconds () << "\t" << (double)jitter.GetLastJitter()/1000000000 << std::endl;
	  *streamdelay->GetStream () << Simulator::Now ().GetSeconds () << "\t" << jitter.GetLastDelta().GetSeconds() << std::endl;*/

	  //NS_LOG_UNCOND ("Process packet " << Simulator::Now ().GetSeconds () << " " << rtpHeader << " " << nalHeader << " " << Simulator::Now().GetSeconds()-(MilliSeconds(rtpHeader.GetPacketTimestamp()).GetSeconds()) << " " << socket << " " << jitter.GetLastJitter() << " " << jitter.GetLastDelay());
	  packetdelay+=Simulator::Now().GetSeconds()-(MilliSeconds(rtpHeader.GetPacketTimestamp()).GetSeconds());
	  packetnum++;
}



void
MpTcpTest::Join(void)
{
	  Ptr<MpTcpSocket> mpSocket = sender->GetSocket()->GetObject<MpTcpSocket>();
	  Simulator::ScheduleNow (&MpTcpSocket::Join, mpSocket,InetSocketAddress(ipv4Ints[1].GetAddress(1),50000),TcpWestwood::GetTypeId());
	  //Simulator::Schedule (Seconds(0.1),&MpTcpTest::Trace, sender->GetSocket()->GetObject<TcpSocket>());
      //mpSocket->TraceConnectWithoutContext ("Drop", MakeCallback (&MpTcpTest::Drop,this));

}

void
MpTcpTest::Trace(Ptr<TcpSocket> sock)
{

	std::vector<Ptr<TcpSocketBase> > m_sockets = sock->GetSocketsBase();
	int i = 0;
	for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin() ; it != m_sockets.end(); ++it)
	{
		AsciiTraceHelper asciiTraceHelper;
		std::ostringstream oss1;
		oss1 << "node" << i <<".cwnd";
		Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (oss1.str());
		(*it)->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&MpTcpTest::CwndChange, stream));

		std::ostringstream oss2;
		oss2 << "node" << i <<".rtt";
		Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (oss2.str());
		(*it)->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&MpTcpTest::RttChange, stream2));

		std::ostringstream oss3;
		oss3 << "node" << i <<".bwe";
		Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream (oss3.str());
		(*it)->TraceConnectWithoutContext ("EstimatedBW", MakeBoundCallback (&MpTcpTest::BwChange, stream3));

		std::ostringstream oss4;
		oss4 << "node" << i <<".landa";
		Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream (oss4.str());
		(*it)->TraceConnectWithoutContext ("LandaRate", MakeBoundCallback (&MpTcpTest::LandaChange, stream4));

		std::ostringstream oss5;
		oss5 << "node" << i <<".bf";
		Ptr<OutputStreamWrapper> stream5 = asciiTraceHelper.CreateFileStream (oss5.str());
		(*it)->TraceConnectWithoutContext ("Buffer", MakeBoundCallback (&MpTcpTest::BufferChange, stream5));
		i++;
	}

}

double
MpTcpTest::Run(uint32_t forward_delay, uint32_t bandwidth, uint32_t scheduler)
{
	  /* Some simulation variables */
	  //double packetLossRate = 0.001;
	  unsigned int mtu = 1000;
	  uint32_t sched = scheduler;
	  uint32_t fd=forward_delay;
	  uint32_t bw=bandwidth;
	  uint32_t buffer=375000;
	  Config::SetDefault ("ns3::TcpSocketBase::SegmentSize", UintegerValue (mtu));
	  Config::SetDefault ("ns3::BulkSendMpApplication::SendSize", UintegerValue (mtu-13));
	  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (1500));
	  Config::SetDefault ("ns3::MpScheduler::Mtu", UintegerValue (mtu));
	  Config::SetDefault ("ns3::MpTcpSocket::SndBufSize", UintegerValue (buffer));
	  Config::SetDefault ("ns3::TcpSocketBase::SndBufSize", UintegerValue (buffer));
	  Config::SetDefault ("ns3::MpTcpSocket::RcvBufSize", UintegerValue (buffer));
	  Config::SetDefault ("ns3::TcpSocketBase::RcvBufSize", UintegerValue (buffer));
		jitter = DelayJitterEstimation();

		/*AsciiTraceHelper asciiTraceHelperdelay;
		std::ostringstream ossjitter;
		ossjitter << "jitter.tr";
		streamjitter = asciiTraceHelperdelay.CreateFileStream (ossjitter.str());

		std::ostringstream ossdelay;
		ossdelay << "jitterdelay.tr";
		streamdelay = asciiTraceHelperdelay.CreateFileStream (ossdelay.str());

		std::ostringstream ossdelay1;
		ossdelay1 << "delay01.tr";
		streamdelay1 = asciiTraceHelperdelay.CreateFileStream (ossdelay1.str());


		std::ostringstream ossdelay2;
		ossdelay2 << "delay02.tr";
		streamdelay2 = asciiTraceHelperdelay.CreateFileStream (ossdelay2.str());

		std::ostringstream ossdelay3;
		ossdelay3 << "delay11.tr";
		streamdelay3 = asciiTraceHelperdelay.CreateFileStream (ossdelay3.str());


		std::ostringstream ossdelay4;
		ossdelay4 << "delay12.tr";
		streamdelay4 = asciiTraceHelperdelay.CreateFileStream (ossdelay4.str());*/

	  uint32_t sf = 2;


	  switch(sched)
	  {
	  case 0:
		  Config::SetDefault ("ns3::TcpL4Protocol::Scheduler", TypeIdValue (MpRoundRobin::GetTypeId ()));
		  break;
	  case 1:
		  Config::SetDefault ("ns3::TcpL4Protocol::Scheduler", TypeIdValue (MpWeightedRoundRobin::GetTypeId ()));
		  Config::SetDefault ("ns3::MpWeightedRoundRobin::w1", UintegerValue (4));
		  Config::SetDefault ("ns3::MpWeightedRoundRobin::w2", UintegerValue (1));
		  break;
	  case 2:
		  Config::SetDefault ("ns3::TcpL4Protocol::Scheduler", TypeIdValue (MpKernelScheduler::GetTypeId ()));
		  break;
	  case 3:
		  Config::SetDefault ("ns3::TcpL4Protocol::Scheduler", TypeIdValue (MpWaterfilling2::GetTypeId ()));
		  break;
	  default:
		  Config::SetDefault ("ns3::TcpL4Protocol::Scheduler", TypeIdValue (MpRoundRobin::GetTypeId ()));
		  break;
	  }
	  std::string simulationDuration("200s");
	  std::string transmitterStartTime("2s");
	  std::string transmitterStopTime("100s");
	  std::string receiverStartTime("1s");
	  std::string receiverStopTime("101s");


	  std::cout << "scheduler " << sched << std::endl;
	  /* Network setup */
	  NodeContainer nodes;
	  nodes.Create(2);


	  InternetStackHelper stack;
	  stack.Install(nodes);


	  for(uint32_t i=0; i < sf; i++)
	  {
	      // Creation of the point to point link between hots
		     // Creation of the point to point link between hots
		   PointToPointHelper p2plink;
		   if(i==0){
			   //p2plink.SetDeviceAttribute ("DataRate",  StringValue ("462Kbps"));
			   p2plink.SetDeviceAttribute ("DataRate",  StringValue ("10Mbps"));
			   p2plink.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(1)));

		   }
		   else if(i==1){
			   //p2plink.SetDeviceAttribute ("DataRate",  StringValue ("462Kbps"));
			   p2plink.SetDeviceAttribute ("DataRate",  DataRateValue (DataRate (bw)));
			   p2plink.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(fd)));
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
	      p2plink.EnablePcap ("qoe",nodes);

	     // std::cout << "ipaddress " << interface.GetAddress(0) << " " << interface.GetAddress(1) << std::endl;
	        /*AsciiTraceHelper ascii;
			std::ostringstream oss1;
			oss1 << "qoe" << i << ".tr";
			p2plink.EnableAsciiAll (ascii.CreateFileStream (oss1.str()));*/

	  }

	   uint16_t port = 50000;  // well-known echo port number


	   BulkMpSendHelper source ("ns3::TcpSocketFactory",
	                          InetSocketAddress (ipv4Ints[0].GetAddress (1), port));
	   // Set the amount of data to send in bytes.  Zero is unlimited.
	   source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
	   source.SetAttribute ("SocketType",StringValue("ns3::MpTcpSocket"));
	   source.SetAttribute ("Congestion",StringValue(congestion));
	   ApplicationContainer sourceApps = source.Install (nodes.Get(0));
	   sourceApps.Start (Time (transmitterStartTime));
	   sourceApps.Stop (Time (transmitterStopTime));


	   sender = DynamicCast<BulkSendMpApplication> (sourceApps.Get (0));
	   sender->SetCallback(MakeCallback(&MpTcpTest::Join,this));
	   sender->TraceConnectWithoutContext ("Tx", MakeCallback (&MpTcpTest::TxPacket,this));

	   PacketMpSinkHelper sink ("ns3::TcpSocketFactory",
	                          InetSocketAddress (Ipv4Address::GetAny (), port));
	   sink.SetAttribute ("SocketType",StringValue("ns3::MpTcpSocket"));
	   sink.SetAttribute ("Congestion",StringValue(congestion));
	   ApplicationContainer sinkApps = sink.Install (nodes.Get(1));

	   sinkApps.Start (Time (receiverStartTime));
	   sinkApps.Stop (Time (receiverStopTime));


	   Ptr<PacketMpSink> sink1 = DynamicCast<PacketMpSink> (sinkApps.Get (0));
	   sink1->TraceConnectWithoutContext ("Tx", MakeCallback (&MpTcpTest::RxPacket,this));
	   //sink1->SetCallback(MakeCallback(&Trace2));


	  Simulator::Stop (Time(simulationDuration));
	  Simulator::Run();


	  Simulator::Destroy();
	  NS_LOG_UNCOND (packetdelay << " " << packetnum);
	  return packetdelay/packetnum;

}


int main(int argc, char *argv[])
{
	 std::string fileNameWithNoExtension = "plot-2d";
	  std::string graphicsFileName        = fileNameWithNoExtension + ".png";
	  std::string plotFileName            = fileNameWithNoExtension + ".plt";
	  std::string plotTitle               = "Bulk transfer MPTCP scheduling";
	  //std::string dataTitle               = "2-D Data";

	  // Instantiate the plot and set its title.
	  Gnuplot plot (graphicsFileName);
	  plot.SetTitle (plotTitle);

	  // Make the graphics file, which the plot file will create when it
	  // is used with Gnuplot, be a PNG file.
	  plot.SetTerminal ("png");

	  // Set the labels for each axis.
	  plot.SetLegend ("Forward propagation delay", "Average end-to-end delay");

	  // Set the range for the x axis.
	  plot.AppendExtra ("set yrange [0:+1]");

	  // Instantiate the dataset, set its title, and make the points be
	  // plotted along with connecting lines.

    Gnuplot2dDataset dataset;
    Gnuplot2dDataset dataset2;
    Gnuplot2dDataset dataset3;
    Gnuplot2dDataset dataset4;


	/*for(int i = 1; i<100;i+=10){
		NS_LOG_UNCOND ("Simulation "<< i);
		MpTcpTest test;
		double result = test.Run(i,2500000,0);
		dataset.SetTitle ("Round Robin");
		dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);
		dataset.Add(i,result);
	}
	for(int i = 1; i<100;i+=10){
		NS_LOG_UNCOND ("Simulation "<< i);
		MpTcpTest test;
		double result = test.Run(i,2500000,1);
		dataset2.SetTitle ("Weighted Round Robin");
		dataset2.SetStyle (Gnuplot2dDataset::LINES_POINTS);
		dataset2.Add(i,result);
	}
	for(int i = 1; i<100;i+=10){
		NS_LOG_UNCOND ("Simulation "<< i);
		MpTcpTest test;
		double result = test.Run(i,2500000,2);
		NS_LOG_UNCOND ("Simulation2 "<< i);
		dataset3.SetTitle ("Linux Kernel");
		dataset3.SetStyle (Gnuplot2dDataset::LINES_POINTS);
		dataset3.Add(i,result);
	}
	for(int i = 1; i<100;i+=10){
		NS_LOG_UNCOND ("Simulation "<< i);
		MpTcpTest test;
		double result = test.Run(i,2500000,3);
		dataset4.SetTitle ("Waterfilling");
		dataset4.SetStyle (Gnuplot2dDataset::LINES_POINTS);
		dataset4.Add(i,result);
	}*/
	MpTcpTest test;
	test.Run(10,2500000,3);

	  // Add the dataset to the plot.
	  plot.AddDataset (dataset);
	  plot.AddDataset (dataset2);
	  plot.AddDataset (dataset3);
	  plot.AddDataset (dataset4);

	  // Open the plot file.
	  std::ofstream plotFile (plotFileName.c_str());

	  // Write the plot file.
	  plot.GenerateOutput (plotFile);

	  // Close the plot file.
	  plotFile.close ();
	return 0;
}


