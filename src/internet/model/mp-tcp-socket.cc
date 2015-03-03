/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/nstime.h"
#include "tcp-socket.h"
#include "tcp-socket-base.h"
#include "ns3/node.h"
#include "tcp-l4-protocol.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "tcp-mptcp.h"
//#include "ns3/sha-digest.h"
#include "tcp-westwood.h"
#include "mp-tcp-socket.h"
#include "ipv4-end-point.h"
#include "mp-waterfilling.h"
#include "mp-waterfilling2.h"
//#include "mp-wardrop.h"
//#include "mp-wardrop2.h"
//#include "mp-wardrop3.h"
#include "mp-kernelsched.h"
#include "mp-roundrobin.h"
#include "ns3/rtp-protocol.h"
#include "ns3/nal-unit-header.h"
#include "ns3/delay-jitter-estimation.h"
#include "ns3/tag.h"

NS_LOG_COMPONENT_DEFINE ("MpTcpSocket");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpTcpSocket);

//const char* const MpTcpSocket::MpTcpStateName[LAST_STATE] = { "MP_NONE", "MP_MPC", "MP_ADDR", "MP_JOIN"};

TypeId
MpTcpSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpTcpSocket")
    .SetParent<TcpSocket> ()
    .AddConstructor<MpTcpSocket> ()
	.AddAttribute ("Scheduler", "The amount of data to send each time.",
				   UintegerValue (1),
				   MakeUintegerAccessor (&MpTcpSocket::m_sched),
				   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TcpMp", "Set to true to enable SACK options",
      			   BooleanValue (true),
   			   MakeBooleanAccessor (&MpTcpSocket::GetTcpMp,
   									&MpTcpSocket::SetTcpMp),
   			   MakeBooleanChecker ())
	.AddAttribute ("MpSocketType",
				   "Socket type of TCP objects.",
				   TypeIdValue (TcpMpTcp::GetTypeId ()),
   	               MakeTypeIdAccessor (&MpTcpSocket::m_socketTypeId),
				   MakeTypeIdChecker ())
	 /*.AddAttribute ("SocketList", "The list of sockets associated to this protocol.",
				   ObjectVectorValue (),
				   MakeObjectVectorAccessor (&MpTcpSocket::m_sockets),
				   MakeObjectVectorChecker<TcpSocketBase> ())*/
	.AddAttribute ("SndBufSize",
   	               "TcpSocket maximum transmit buffer size (bytes)",
   	               UintegerValue (131072), // 256k
   	               MakeUintegerAccessor (&MpTcpSocket::GetSndBufSize,
   	                                     &MpTcpSocket::SetSndBufSize),
   	               MakeUintegerChecker<uint32_t> ())
   	.AddAttribute ("RcvBufSize",
   				   "TcpSocket maximum receive buffer size (bytes)",
   				   UintegerValue (131072),
   				   MakeUintegerAccessor (&MpTcpSocket::GetRcvBufSize,
   										 &MpTcpSocket::SetRcvBufSize),
   	               MakeUintegerChecker<uint32_t> ())
	.AddTraceSource ("Throughput", "traced throughput.",
				   MakeTraceSourceAccessor (&MpTcpSocket::m_packetRxTrace))
    .AddTraceSource ("Joined", "Joined subflow.",
                   MakeTraceSourceAccessor (&MpTcpSocket::m_joined))
    .AddTraceSource ("TotalCwnd",
 					"Next sequence number to send (SND.NXT)",
 					MakeTraceSourceAccessor (&MpTcpSocket::m_cWndTotal))
    .AddTraceSource ("Alpha",
	 				"Highest sequence number ever sent in socket's life time",
	 				MakeTraceSourceAccessor (&MpTcpSocket::m_alpha))
	.AddTraceSource ("Drop", "A new packet is created and is sent",
					 MakeTraceSourceAccessor (&MpTcpSocket::m_drop))
	 .AddTraceSource ("Buffer",
					  "Remote side's flow control window",
					  MakeTraceSourceAccessor (&MpTcpSocket::m_buffsize))
	/*.AddTraceSource ("Succeed", "All subflows connected",
					 MakeTraceSourceAccessor (&MpTcpSocket::m_connected))*/
	//.AddAttribute ("MaxWindowSize", "Max size of advertised window",
	//			   UintegerValue (65535),
	//			   MakeUintegerAccessor (&MpTcpSocket::m_maxWinSize),
	//			   MakeUintegerChecker<uint16_t> ())

  ;
  return tid;
}

MpTcpSocket::MpTcpSocket ()
//:TcpSocket(TcpMpTcp::GetTypeId ()),
//:TcpSocket(m_socketTypeId),
:
 //m_txBuffer (0),
 m_rxBuffer (0),
 m_multipath(true),
 m_capable(true),
 m_nextTxSequence (1),
 m_sched(0),
 m_ack(0),
 //m_distribAlgo(Round_Robin),
 //m_distribAlgo(Water_Filling),
 //m_distribAlgo(Wardrop),
 //m_distribAlgo(Kernel),
 //m_lastUsedsFlowIdx (0),
 m_landaRate(0),
 m_lastSampleLanda(0),
 m_lastLanda(0),
 data(0),
 succeed(false),
 m_maxBuffsize(0),
 m_buffsize(0)
  {
  NS_LOG_FUNCTION (this);
  m_rxBuffer.SetNextRxSequence(m_nextTxSequence);

}

MpTcpSocket::~MpTcpSocket ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
MpTcpSocket::SetTcpMp (bool multipath)
{
	NS_LOG_FUNCTION (this);
	m_multipath = multipath;

}


void
MpTcpSocket::Join2(TypeId congestion)
{
  NS_LOG_FUNCTION (this);
  ObjectFactory socketBaseFactory;
  ObjectFactory rttFactory;
  rttFactory.SetTypeId (RttMeanDeviation::GetTypeId ());
  Ptr<RttEstimator> rtt = rttFactory.Create<RttEstimator> ();
  //socketBaseFactory.SetTypeId (TcpMpTcp::GetTypeId ());
  socketBaseFactory.SetTypeId (congestion);
  Ptr<TcpSocketBase> socketBase = socketBaseFactory.Create<TcpSocketBase> ();
  socketBase->SetTcp(Create<TcpL4Protocol>());
  socketBase->SetNode(m_node);
  socketBase->SetRtt(rtt);
  socketBase->SetTcp (m_tcp);
  socketBase->SetSocket(this);
  socketBase->Allocate();
  socketBase->Connect(InetSocketAddress (Ipv4Address("10.1.2.2"), 50000));
}

void
MpTcpSocket::Join(InetSocketAddress socket, TypeId congestion)
{
  NS_LOG_FUNCTION (this);
  ObjectFactory socketBaseFactory;
  ObjectFactory rttFactory;
  rttFactory.SetTypeId (RttMeanDeviation::GetTypeId ());
  Ptr<RttEstimator> rtt = rttFactory.Create<RttEstimator> ();
  //socketBaseFactory.SetTypeId (TcpMpTcp::GetTypeId ());
  socketBaseFactory.SetTypeId (congestion);
  Ptr<TcpSocketBase> socketBase = socketBaseFactory.Create<TcpSocketBase> ();
  socketBase->SetTcp(Create<TcpL4Protocol>());
  socketBase->SetNode(m_node);
  socketBase->SetRtt(rtt);
  socketBase->SetTcp (m_tcp);
  socketBase->SetSocket(this);
  //socketBase->BindToNetDevice(netDev);
  socketBase->Allocate();
  socketBase->Connect(socket);
  //m_sockets.push_back(socketBase);
}

void
MpTcpSocket::SetTcpMpCapable(void)
{
	NS_LOG_FUNCTION (this);
	m_capable = true;
}

bool
MpTcpSocket::GetTcpMpCapable(void)
{
	//NS_LOG_FUNCTION (this);
	//NS_LOG_FUNCTION (this<<m_capable);
	return m_capable;
}

bool
MpTcpSocket::GetTcpMp (void) const
{
  return m_multipath;
}

void
MpTcpSocket::SetLocalKey (uint64_t key)
{
  m_localKey = key;
}
void
MpTcpSocket::SetRemoteKey (uint64_t key)
{
  m_remoteKey = key;
}
uint64_t
MpTcpSocket::GetLocalKey (void)
{
  return m_localKey;
}
uint64_t
MpTcpSocket::GetRemoteKey (void)
{
  return m_remoteKey;
}

void
MpTcpSocket::SetRcvBufSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_rxBuffer.SetMaxBufferSize (size);
}

uint32_t
MpTcpSocket::GetRcvBufSize (void) const
{
  return m_rxBuffer.MaxBufferSize ();
}

/** Inherit from Socket class: Recv and return the remote's address */
Ptr<Packet>
MpTcpSocket::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
{
  NS_LOG_FUNCTION (this << maxSize << flags << fromAddress);

  return Recv (maxSize, flags);

}

void
MpTcpSocket::ReceivedData(Ptr<TcpSocketBase> socket, SequenceNumber64 seq, Ptr<Packet> packet)
{
	NS_LOG_FUNCTION(socket << seq << packet->GetSize() << m_rxBuffer.NextRxSequence () << m_rxBuffer.Size() << m_rxBuffer.MaxBufferSize());
	Address fromAddress;
	SequenceNumber64 expectedSeq = m_rxBuffer.NextRxSequence ();

	if(!m_socketRecv.IsNull())m_socketRecv(packet,seq,socket);
    if(packet)
	NS_LOG_LOGIC("ExpectedSeq " << expectedSeq << " seq " << seq << " nextrxseq " << m_rxBuffer.NextRxSequence () << " packet length " << packet->GetSize());
     if(packet != 0 && packet->GetSize () != 0)
     {
  	  if (!m_rxBuffer.Add (packet,seq))
  		{ // Insert failed: No data or RX buffer full
  		  NS_LOG_LOGIC("Error buffer64 add packet");
  		  return;
  		}
     }

     NS_LOG_LOGIC("NextRxSequence " << m_rxBuffer.NextRxSequence () << " expectedseq " << expectedSeq);

  	 if (expectedSeq < m_rxBuffer.NextRxSequence ())
  	 {
  		 NS_LOG_LOGIC("RecvCallback");
  	  	 socket->SetDataAck(m_rxBuffer.NextRxSequence ());
  		 NotifyDataRecv();
     }

   	NS_LOG_LOGIC("Buffer " << m_rxBuffer.MaxBufferSize ()  << " " << m_rxBuffer.Size ());


}

uint32_t
MpTcpSocket::GetCwndTotal()
{
	NS_LOG_FUNCTION(this);

	uint32_t cwnd_total=0;
	for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin() ; it != m_sockets.end(); ++it)
	{
		if(SocketCanSend(*it))cwnd_total+= (*it)->GetCongestionWindow();
	}
	m_cWndTotal = cwnd_total;
	return m_cWndTotal;
}


double
MpTcpSocket::GetAlpha()
{
	NS_LOG_FUNCTION(this);

	double max_numerator=0;
	double sum=0;
	for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin() ; it != m_sockets.end(); ++it)
	{
		if(SocketCanSend(*it))
		{
			double rtt = ((*it)->GetRtt()->GetCurrentEstimate ().GetMicroSeconds())*((*it)->GetRtt()->GetCurrentEstimate ().GetMicroSeconds());
			double tmp = (*it)->GetCongestionWindow() / rtt ;

			NS_LOG_INFO("Max Rtt^2 " << rtt << " cwnd " <<(*it)->GetCongestionWindow() );
			if(tmp>=max_numerator)max_numerator=tmp;
		}
	}
	for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin() ; it != m_sockets.end(); ++it)
	{
		if(SocketCanSend(*it))
		{
			sum+= (*it)->GetCongestionWindow() / ((*it)->GetRtt()->GetCurrentEstimate ().GetMicroSeconds());
			NS_LOG_INFO("Sum cwnd " << (*it)->GetCongestionWindow() << " rtt " <<((*it)->GetRtt()->GetCurrentEstimate ().GetMicroSeconds()) );

		}
	}

	NS_LOG_INFO("cwnd total " << GetCwndTotal() << " max " <<max_numerator << " sum " << sum << " sum^2 " << sum*sum );

	m_alpha =  GetCwndTotal()*(max_numerator/(sum*sum));
	return m_alpha;
}

void
MpTcpSocket::ReceivedAck(Ptr<TcpSocketBase> socket, SequenceNumber64 ack)
{
	NS_LOG_FUNCTION(this << socket << ack << m_ack);//0 << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size());
	Ptr<Packet> packet;
	Address fromAddress;
	if(succeed)m_ack++;
	if(m_scheduler->Discard(ack))
	       Discard (ack);
	m_buffsize = m_scheduler->GetBufSize();

	NS_LOG_FUNCTION("Buffer size " << m_buffsize << " available " << m_scheduler->Available());

}

uint32_t
MpTcpSocket::AdvertisedWindowSize ()
{
	 // NS_LOG_FUNCTION(this << m_rxBuffer.MaxBufferSize () << m_rxBuffer.Size ());
	  return m_rxBuffer.MaxBufferSize () - m_rxBuffer.Size ();

}

bool
MpTcpSocket::SocketCanSend(Ptr<TcpSocketBase> socket){

	return socket->GetState()==ESTABLISHED||socket->GetState()==CLOSE_WAIT;
}
Ptr<Packet>
MpTcpSocket::Recv (uint32_t maxSize, uint32_t flags)
{
	NS_LOG_FUNCTION (this <<  maxSize << flags << m_rxBuffer.Size());
	//NS_ABORT_MSG_IF (flags, "use of flags is not supported in TcpSocket::Recv()");
	if (m_rxBuffer.Size () == 0 && m_state == CLOSE_WAIT)
	  {
		NS_LOG_FUNCTION("empty packet");
		return Create<Packet> (); // Send EOF on connection close
	  }
	Ptr<Packet> outPacket = m_rxBuffer.Extract (maxSize);

	NS_LOG_FUNCTION (this <<  maxSize << flags << m_rxBuffer.Size());

	return outPacket;
}

int
MpTcpSocket::Discard (SequenceNumber64 ack)
{
  NS_LOG_FUNCTION (this << ack );

	  for(std::map<SequenceNumber64,uint8_t>::iterator it = seqTx.begin(); it != seqTx.find(ack); it++)
	  {
		  seqTx.erase(it);
	  	}

	  return 1;

}

int
MpTcpSocket::Send (Ptr<Packet> p, uint32_t flags)
{

  int i = -1;
  //NS_LOG_FUNCTION (this << m_landaRate << p->GetSize() << m_sched);
  NS_LOG_FUNCTION (this << m_landaRate << p->GetSize() << succeed);

  //If new packet is sent
  if(p->GetSize()>0){

	  //Add packet to the buffer/scheduler
	  if(!m_socketSubflow.IsNull())m_socketSubflow(p);
	  //NS_LOG_LOGIC("Frame type " << (uint32_t)m_subflow);
	  if (m_scheduler->Add (p,m_subflow)==-1)
	  { // TxBuffer overflow, send failed
		 NS_LOG_FUNCTION("Buffer full");
		 m_errno = Socket::ERROR_MSGSIZE;
		 i = -1;
		 //m_segmentSize = 0;
	  } else {
		  NS_LOG_FUNCTION("Buffer not full");
		  i = p->GetSize();
		  //m_segmentSize = p->GetSize();
	  }
		m_buffsize = m_scheduler->GetBufSize();


	  //Calculate Throughput
	  t = Simulator::Now()-t;
	  //NS_LOG_FUNCTION(Simulator::Now().GetSeconds() << t.GetSeconds());
	  data+= p->GetSize()*8;
	  if(t.GetSeconds()>0){
		  m_landaRate = data / t.GetSeconds();
		  data = 0;
		  double alpha = 0.6;
		  double sample_landa = m_landaRate;
		  m_landaRate = (alpha * m_lastLanda) + ((1 - alpha) * ((sample_landa + m_lastSampleLanda) / 2));
		  m_lastSampleLanda = sample_landa;
		  m_lastLanda = sample_landa;
	  }
	  t = Simulator::Now();
	  //Assign packet size to segment size

  		// while(m_ack>0)
	  //if(succeed){

	  //}

  }
  //m_state = m_sockets[0]->GetState();

  //if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
  //{
  int sent=0;
  while(sent>=0){
      sent = SendPacket(flags);
	  NS_LOG_FUNCTION(this<< succeed << m_ack << sent);
  }
  //}
  return i;

}

int
MpTcpSocket::SendPacket(uint32_t flags){
	  //If connection established continue
	  NS_LOG_FUNCTION (this);

	  int i = -1;

	  //Get state
	  m_state = m_sockets[0]->GetState();

	  //Get packet to transmit from scheduler
	  bool result = m_scheduler->GetPacket(m_sockets);

	  if(result){
	  //If connection established send packet
	  if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
	  { // Try to send the data out
		  NS_LOG_LOGIC("Socket ESTABLISHED or CLOSE_WAIT");

		  //Get path number to transmit packet
		  int lastUsedsFlowIdx = m_scheduler->GetSubflowToUse();
		  NS_LOG_LOGIC("FLowid " << lastUsedsFlowIdx << " size " << m_sockets.size() << " m_nextTxSequence " <<  m_nextTxSequence);

		  //If available path -> send packet
		  if(lastUsedsFlowIdx!=-1){
			  NS_LOG_LOGIC("Send packet");
			  Ptr<Packet> packet = Create<Packet>();
			  i = m_sockets[lastUsedsFlowIdx]->Send(packet,flags);
			  //Send duplicate packets over another paths without traffic
			  for(SocketPacketIte it = sPacketMap.begin(); it != sPacketMap.end(); ++it)
			  {
				  NS_LOG_LOGIC("Socket going to send " << it->first << " " << it->second);
				  if(it->second==10)
				  {
					 NS_LOG_LOGIC("Backup sent");
					 //it->first->Send(packet,m_nextTxSequence,flags);
					 m_scheduler->SendBackup(it->first);
					 it->first->Send(packet,flags);
				  }
			  }
		  }

		  //If packet sent succesfully
		  if(i>=0){
			  NS_LOG_LOGIC("Tcpsocketbase Packet sent " << i);
			  //Control which paths are sending traffic and which are
			  //if(m_distribAlgo!=Round_Robin&&m_distribAlgo!=Kernel){
			 // NS_LOG_LOGIC("Backup sent2 " << m_sched << " " << MpWaterfilling::GetTypeId() << " " << MpWaterfilling2::GetTypeId());
			  //if(m_scheduler->GetTypeId()==MpWaterfilling::GetTypeId()||m_scheduler->GetTypeId()==MpWaterfilling2::GetTypeId()){
			  if(m_sched==3||m_sched==4){
				//  NS_LOG_LOGIC("Backup sent3");
				  SocketPacketIte iter = sPacketMap.find(m_sockets[lastUsedsFlowIdx]);
				  if( iter != sPacketMap.end() )
				  {
					  sPacketMap.erase(iter);
					  sPacketMap.insert(SocketPacketPair(m_sockets[lastUsedsFlowIdx],0));
					  //NS_LOG_LOGIC("Socket packet count " << m_sockets[lastUsedsFlowIdx] << " " << 0);
				  }
				  for(SocketPacketIte it = sPacketMap.begin(); it != sPacketMap.end(); ++it)
				  {
					  if(iter!=it){
						  sPacketMap.erase(it);
						  if(it->second==10){
							  sPacketMap.insert(SocketPacketPair(it->first,0));
						  } else {
							  sPacketMap.insert(SocketPacketPair(it->first,++it->second));
						  }
					  }
					  //NS_LOG_LOGIC("Socket packet count " << it->first << " " << it->second);

				  }
			  }
			  seqTx.insert(SeqPair(m_nextTxSequence,lastUsedsFlowIdx));
			  NS_LOG_LOGIC("Seq insert " << m_nextTxSequence << " " << lastUsedsFlowIdx << " " << false);
			  //m_nextTxSequence += m_scheduler->GetSize(m_nextTxSequence);
			  //m_scheduler->SetNextSequence(m_nextTxSequence);
			  //m_segmentSize = packet->GetSize();
		  } else {
			  NS_LOG_LOGIC("Tcpsocketbase Packet not sent " << i);
		  }
	  }
	  else
	  {
		  NS_LOG_LOGIC("ERROR Socket NOT ESTABLISHED");
	  }
	  } else {
		  NS_LOG_LOGIC("No packet to send");

		  return -1;
	  }
	  return i;
}

void
MpTcpSocket::SetSubflowCallback (Callback<void, Ptr<Packet> > socketSubflow){
	m_socketSubflow = socketSubflow;
}

void
MpTcpSocket::SetTraceCallback(Callback<void> traceCallback)
{
	m_trace = traceCallback;
}
void
MpTcpSocket::SetRecvCallback (Callback<void, Ptr<Packet>, SequenceNumber64, Ptr<TcpSocketBase> > socketRecv){
	m_socketRecv = socketRecv;
}

void
MpTcpSocket::SetSuccedCallback (Callback<void> succeedCallback)
{
	m_connected = succeedCallback;
}
void
MpTcpSocket::SetSubflow (uint8_t subflow,uint32_t size){
	NS_LOG_FUNCTION(this<<(uint32_t)subflow << size);
	m_subflow = subflow;
}

void
MpTcpSocket::CloseSubflow(InetSocketAddress socket)
{

  NS_LOG_FUNCTION (this << m_sockets.size() << socket.GetIpv4() << socket.GetPort() );

  m_state = m_sockets[0]->GetState();

  if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
  { // Try to send the data out

	  for(std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin(); it != m_sockets.end(); it++)
	  {

		  if(socket.GetIpv4()==(*it)->GetEndPoint()->GetPeerAddress()&&socket.GetPort()==(*it)->GetEndPoint()->GetPeerPort())
		  {

			  NS_LOG_LOGIC("Delete socket " << (*it)->GetEndPoint()->GetPeerAddress());
			  m_sockets.erase (it);
			  break;

		  }

	  }
  }

}

SequenceNumber64
MpTcpSocket::GetNextSequence (Ptr<TcpSocketBase> tcpSocketBase, const SequenceNumber32& seq)
{
	return m_scheduler->GetNextSequence(tcpSocketBase,seq);
}


SequenceNumber64
MpTcpSocket::GetRxSequence()
{
	return m_rxBuffer.NextRxSequence ();
}
void
MpTcpSocket::SetMpcapSent(bool sent)
{
	m_mpcapSent = sent;
}

bool
MpTcpSocket::GetMpcapSent(void)
{
	return m_mpcapSent;
}

int
MpTcpSocket::NotifySubflowConnectionSucceeded(Ptr<TcpSocketBase> socket)
{
	NS_LOG_FUNCTION(this << socket );
	 t = Simulator::Now();
	m_joined(socket);
	m_succeed = true;
	socket->SetSucceed();

	//if(m_distribAlgo!=Kernel&&m_distribAlgo!=Round_Robin)
	//if(m_scheduler->GetTypeId()==MpWaterfilling::GetTypeId()||m_scheduler->GetTypeId()==MpWaterfilling2::GetTypeId())
	if(m_sched==3||m_sched==4)
		sPacketMap.insert(SocketPacketPair(socket,0));

	for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_rxSockets.begin() ; it != m_rxSockets.end(); ++it)
	{
		NS_LOG_FUNCTION(this << " rxsocket succeed "<< (*it));
		(*it)->SetSucceed();
	}
	NS_LOG_FUNCTION(this << m_sockets.size() );

	for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin() ; it != m_sockets.end(); ++it)
	{
		if(*it == socket) return -1;
	}
	m_sockets.push_back(socket);

	if(m_sockets.size()==2){
		m_connected();
		if(!m_trace.IsNull())m_trace();
		succeed = true;
	}
	NS_LOG_FUNCTION(this << m_sockets.size() );

	return 0;

}

Ptr<MpScheduler>
MpTcpSocket::GetScheduler()
{
	return m_scheduler;
}

void
MpTcpSocket::SetScheduler(Ptr<MpScheduler> mp)
{
	NS_LOG_FUNCTION(this << mp->GetTypeId());
	m_scheduler = mp;
	m_scheduler->SetSndBufSize(m_maxBuffsize);
}

void
MpTcpSocket::SetAck(void)
{
	m_ack++;
	NS_LOG_FUNCTION(this << succeed << m_ack);


}
void
MpTcpSocket::SetSndBufSize (uint32_t size)
{
	NS_LOG_FUNCTION(this<<size);
	m_maxBuffsize = size;
}

uint32_t
MpTcpSocket::GetSndBufSize (void) const
{
	NS_LOG_FUNCTION(this<<m_maxBuffsize);
	return m_maxBuffsize;
}
/*uint32_t
MpTcpSocket::Size(Ptr<TcpSocketBase> tcpSocketBase)
{
	//NS_LOG_FUNCTION(this<<m_scheduler->Size(tcpSocketBase));
	return m_scheduler->Size(tcpSocketBase);
}*/


} // namespace ns3
