/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/mp-tcp-socket.h"
#include "ns3/tcp-mptcp.h"
#include "ns3/tcp-newreno.h"
#include "ns3/rtp-protocol.h"
#include "ns3/nal-unit-header.h"


#include "bulk-send-mp-application.h"

NS_LOG_COMPONENT_DEFINE ("BulkSendMpApplication");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BulkSendMpApplication);

TypeId
BulkSendMpApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BulkSendMpApplication")
    .SetParent<Application> ()
    .AddConstructor<BulkSendMpApplication> ()
    .AddAttribute ("SendSize", "The amount of data to send each time.",
                   UintegerValue (1460),
                   MakeUintegerAccessor (&BulkSendMpApplication::m_sendSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&BulkSendMpApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. "
                   "Once these bytes are sent, "
                   "no data  is sent again. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BulkSendMpApplication::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&BulkSendMpApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("SocketType", "The type of protocol to use.",
				   TypeIdValue (MpTcpSocket::GetTypeId ()),
				   MakeTypeIdAccessor (&BulkSendMpApplication::m_socketType),
				   MakeTypeIdChecker ())
    .AddAttribute ("Congestion", "The type of protocol to use.",
	   		 	   TypeIdValue (TcpMpTcp::GetTypeId ()),
				   MakeTypeIdAccessor (&BulkSendMpApplication::m_congestion),
				   MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&BulkSendMpApplication::m_txTrace))
  ;
  return tid;
}


BulkSendMpApplication::BulkSendMpApplication ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0),
    m_packetId (0),
    m_succeed(false)
{
  NS_LOG_FUNCTION (this);
}

BulkSendMpApplication::~BulkSendMpApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
BulkSendMpApplication::SetMaxBytes (uint32_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
BulkSendMpApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
BulkSendMpApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void
BulkSendMpApplication::SetCallback (Callback<void> callback){
	  m_callback = callback;
}

// Application Methods
void
BulkSendMpApplication::StartApplication (void) // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid, m_socketType, m_congestion);

      // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
      if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
          m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
          NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                          "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                          "In other words, use TCP instead of UDP.");
        }

      m_socket->Bind ();
      m_socket->Connect (m_peer);
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
        MakeCallback (&BulkSendMpApplication::ConnectionSucceeded, this),
        MakeCallback (&BulkSendMpApplication::ConnectionFailed, this));
      m_socket->SetSendCallback (
        MakeCallback (&BulkSendMpApplication::DataSend, this));
      m_socket->GetObject<MpTcpSocket>()->SetSuccedCallback (
    	MakeCallback (&BulkSendMpApplication::Succeed, this));

    }
  if (m_connected)
    {
      SendData ();
    }
}

void BulkSendMpApplication::StopApplication (void) // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_connected = false;
    }
  else
    {
      NS_LOG_WARN ("BulkSendMpApplication found null socket to close in StopApplication");
    }
}


// Private helpers

void BulkSendMpApplication::SendData (void)
{
  NS_LOG_FUNCTION (this);

  while (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    { // Time to send more
      uint32_t toSend = m_sendSize;
      // Make sure we don't send too many
      if (m_maxBytes > 0)
        {
          toSend = std::min (m_sendSize, m_maxBytes - m_totBytes);
        }
      //NS_LOG_LOGIC ("sending packet at " << Simulator::Now () << " ");
      Ptr<Packet> packet = Create<Packet> (toSend);
      NalUnitHeader nalHeader;
      if(m_packetId%2==0)nalHeader = NalUnitHeader(1, NalUnitHeader::FU_A);
      else nalHeader = NalUnitHeader(2, NalUnitHeader::FU_A);
      packet->AddHeader(nalHeader);
      RtpProtocol header;
      NS_LOG_LOGIC("Send packet1 " << toSend+13 << " " << m_packetId);
      header = RtpProtocol(RtpProtocol::UNSPECIFIED, m_packetId, Simulator::Now().GetMilliSeconds(), 0);
      packet->AddHeader(header);
      m_txTrace (packet);

      NS_LOG_LOGIC ("sending packet at " << Simulator::Now () << " " << packet->GetSize());

      int actual = m_socket->Send (packet);
      if (actual > 0)
        {
    	  m_packetId++;
          m_totBytes += actual;
        }
      NS_LOG_LOGIC("Send packet2 " << toSend+13 << " " << actual << " " << m_packetId);
      // We exit this loop when actual < toSend as the send side
      // buffer is full. The "DataSent" callback will pop when
      // some buffer space has freed ip.
      if ((unsigned)actual != toSend+13)
        {
          break;
        }
    }
  // Check if time to close (all sent)
  if (m_totBytes == m_maxBytes && m_connected)
    {
      m_socket->Close ();
      m_connected = false;
    }
}

void BulkSendMpApplication::Succeed(void)
{
	NS_LOG_FUNCTION (this);
	m_succeed = true;
}
void BulkSendMpApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("BulkSendMpApplication Connection succeeded");
  m_connected = true;
  if(!m_callback.IsNull())m_callback();
  if(m_succeed)SendData ();
}

void BulkSendMpApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("BulkSendMpApplication, Connection Failed");
}

void BulkSendMpApplication::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this << m_connected << m_succeed);

  if (m_connected&&m_succeed)
    { // Only send new data if the connection has completed
      Simulator::ScheduleNow (&BulkSendMpApplication::SendData, this);
    }
}



} // Namespace ns3
