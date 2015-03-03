/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "bulk-send-mp-application2.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/mp-tcp-socket.h"
#include "ns3/tcp-mptcp.h"
#include "ns3/rtp-protocol.h"
#include "ns3/nal-unit-header.h"
#include "ns3/tcp-westwood.h"


NS_LOG_COMPONENT_DEFINE ("BulkSendMpApplication2");

using namespace std;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BulkSendMpApplication2);

TypeId
BulkSendMpApplication2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BulkSendMpApplication2")
    .SetParent<Application> ()
    .AddConstructor<BulkSendMpApplication2> ()
    .AddAttribute ("DataRate", "The data rate in on state.",
                   DataRateValue (DataRate ("500kb/s")),
                   MakeDataRateAccessor (&BulkSendMpApplication2::m_cbrRate),
                   MakeDataRateChecker ())
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (512),
                   MakeUintegerAccessor (&BulkSendMpApplication2::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&BulkSendMpApplication2::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("OnTime", "A RandomVariableStream used to pick the duration of the 'On' state.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&BulkSendMpApplication2::m_onTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("OffTime", "A RandomVariableStream used to pick the duration of the 'Off' state.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                   MakePointerAccessor (&BulkSendMpApplication2::m_offTime),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BulkSendMpApplication2::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&BulkSendMpApplication2::m_tid),
                   MakeTypeIdChecker ())
   .AddAttribute ("Congestion", "The type of protocol to use.",
			   TypeIdValue (TcpWestwood::GetTypeId ()),
			   MakeTypeIdAccessor (&BulkSendMpApplication2::m_congestion),
			   MakeTypeIdChecker ())
   .AddAttribute ("SocketType", "The type of protocol to use.",
			   TypeIdValue (MpTcpSocket::GetTypeId ()),
			   MakeTypeIdAccessor (&BulkSendMpApplication2::m_socketType),
			   MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&BulkSendMpApplication2::m_txTrace))
  ;
  return tid;
}


BulkSendMpApplication2::BulkSendMpApplication2 ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
  m_connected = false;
  m_residualBits = 0;
  m_lastStartTime = Seconds (0);
  m_totBytes = 0;
  m_succeed = false;
  m_packetId = 0;
}

BulkSendMpApplication2::~BulkSendMpApplication2()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
BulkSendMpApplication2::SetMaxBytes (uint32_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
BulkSendMpApplication2::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

int64_t
BulkSendMpApplication2::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_onTime->SetStream (stream);
  m_offTime->SetStream (stream + 1);
  return 2;
}

void
BulkSendMpApplication2::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void
BulkSendMpApplication2::SetCallback (Callback<void> callback){
	  m_callback = callback;
}
// Application Methods
void BulkSendMpApplication2::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();

  // Create the socket if not already
  if (!m_socket)
    {
	  if(m_tid!=UdpSocketFactory::GetTypeId ()){
	      m_socket = Socket::CreateSocket (GetNode (), m_tid, m_socketType, m_congestion);
	      m_socket->Bind ();
	      m_socket->Connect (m_peer);
	      m_socket->SetAllowBroadcast (true);
	      m_socket->ShutdownRecv ();
	      m_socket->SetConnectCallback (
	        MakeCallback (&BulkSendMpApplication2::ConnectionSucceeded, this),
	        MakeCallback (&BulkSendMpApplication2::ConnectionFailed, this));
	      if(m_socketType==MpTcpSocket::GetTypeId ())m_socket->GetObject<MpTcpSocket>()->SetSuccedCallback (
	      	    	MakeCallback (&BulkSendMpApplication2::Succeeded, this));
	     // m_socket->SetSendCallback (
	     //   MakeCallback (&BulkSendMpApplication2::DataSend, this));

	      CancelEvents ();

	  } else {
	      m_socket = Socket::CreateSocket (GetNode (), m_tid);
	      m_socket->Bind ();
	      m_socket->Connect (m_peer);
	      m_socket->SetAllowBroadcast (true);
	      m_socket->ShutdownRecv ();
	      CancelEvents ();

	      ScheduleStartEvent ();
	  }

    }
  // Insure no pending event
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;

}

void BulkSendMpApplication2::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS ();

  CancelEvents ();
  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("BulkSendMpApplication2 found null socket to close in StopApplication");
    }
}

void BulkSendMpApplication2::CancelEvents ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_sendEvent.IsRunning ())
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta (Simulator::Now () - m_lastStartTime);
      int64x64_t bits = delta.To (Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}

// Event handlers
void BulkSendMpApplication2::StartSending ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_lastStartTime = Simulator::Now ();
  //SendPacket();
  //ScheduleNextTx ();  // Schedule the send packet event
  ScheduleStopEvent ();
}

void BulkSendMpApplication2::StopSending ()
{
  NS_LOG_FUNCTION_NOARGS ();
  CancelEvents ();

  ScheduleStartEvent ();
}

// Private helpers
void BulkSendMpApplication2::ScheduleNextTx ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_FUNCTION(m_cbrRate.GetBitRate ());
  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      uint32_t bits = m_pktSize * 8 - m_residualBits;
      NS_LOG_LOGIC ("bits = " << bits);
      Time nextTime (Seconds (bits /
                              static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = Simulator::Schedule (nextTime,
                                         &BulkSendMpApplication2::SendPacket, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void BulkSendMpApplication2::SetDataRate(DataRate rate)
{
	NS_LOG_FUNCTION(rate.GetBitRate ());
	m_cbrRate = rate;
	buff.clear();
}

void BulkSendMpApplication2::ScheduleStartEvent ()
{  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION_NOARGS ();

  Time offInterval = Seconds (m_offTime->GetValue ());
  NS_LOG_LOGIC ("start at " << offInterval);
  m_startStopEvent = Simulator::Schedule (offInterval, &BulkSendMpApplication2::StartSending, this);
}

void BulkSendMpApplication2::ScheduleStopEvent ()
{  // Schedules the event to stop sending data (switch to "Off" state)
  NS_LOG_FUNCTION_NOARGS ();

  Time onInterval = Seconds (m_onTime->GetValue ());
  NS_LOG_LOGIC ("stop at " << onInterval);
  m_startStopEvent = Simulator::Schedule (onInterval, &BulkSendMpApplication2::StopSending, this);
}

void BulkSendMpApplication2::DataSend (Ptr<Socket> socket, uint32_t f)
{
	  NS_LOG_FUNCTION(socket << f);

	  int result = 0;
	  int packets = 0;
	  if(buff.size()>0)
	  {
		do{
			NS_LOG_LOGIC("Buff size " << buff.size());
			Ptr<Packet> packet = buff.front();
			result = m_socket->Send (packet);
			if(result>0)
			{
				buff.erase(buff.begin());
				packets++;
			}
		}while(result>=0&&buff.size()>0);
	  }
	  NS_LOG_LOGIC("Send packets " << packets);

}

void BulkSendMpApplication2::SendPacket ()
{
  NS_LOG_FUNCTION_NOARGS ();

  //NS_ASSERT (m_sendEvent.IsExpired ());
  //Ptr<Packet> packet = Create<Packet> (m_pktSize);
  Ptr<Packet> packet = Create<Packet> (m_pktSize-13);
    NalUnitHeader nalHeader;
    if(m_packetId%2==0)nalHeader = NalUnitHeader(1, NalUnitHeader::FU_A);
    else nalHeader = NalUnitHeader(2, NalUnitHeader::FU_A);
    packet->AddHeader(nalHeader);
    RtpProtocol header;
    NS_LOG_LOGIC("Send packet1 " << m_pktSize << " " << m_packetId);
    header = RtpProtocol(RtpProtocol::UNSPECIFIED, m_packetId, Simulator::Now().GetMilliSeconds(), 0);
    packet->AddHeader(header);
  m_txTrace (packet);
  m_socket->Send (packet);
  buff.push_back(packet);
  m_packetId++;
  m_totBytes += m_pktSize;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s on-off application sent "
                   <<  packet->GetSize () << " bytes to "
                   << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                   << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s on-off application sent "
                   <<  packet->GetSize () << " bytes to "
                   << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                   << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  m_lastStartTime = Simulator::Now ();
  m_residualBits = 0;
  ScheduleNextTx ();
}

void BulkSendMpApplication2::ConnectionSucceeded (Ptr<Socket> sock)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_connected = true;
  if(!m_callback.IsNull())m_callback();
  //if(m_succeed)DataSend (sock,0);

  //ScheduleStartEvent ();
}

void BulkSendMpApplication2::Succeeded(void)
{
	NS_LOG_FUNCTION (this);
	m_succeed = true;
	ScheduleStartEvent();
	SendPacket();
}

void BulkSendMpApplication2::ConnectionFailed (Ptr<Socket>)
{
  NS_LOG_FUNCTION_NOARGS ();
  cout << "BulkSendMpApplication2, Connection Failed" << endl;
}

} // Namespace ns3
