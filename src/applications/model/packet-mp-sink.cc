/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "packet-mp-sink.h"
#include "ns3/tcp-mptcp.h"
#include "ns3/tcp-newreno.h"
#include "ns3/tcp-socket-base.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PacketMpSink");
NS_OBJECT_ENSURE_REGISTERED (PacketMpSink);

TypeId 
PacketMpSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketMpSink")
    .SetParent<Application> ()
    .AddConstructor<PacketMpSink> ()
    .AddAttribute ("Local", "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&PacketMpSink::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol", "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&PacketMpSink::m_tid),
                   MakeTypeIdChecker ())
   .AddAttribute ("SocketType", "The type of protocol to use.",
			   TypeIdValue (MpTcpSocket::GetTypeId ()),
			   MakeTypeIdAccessor (&PacketMpSink::m_socketType),
			   MakeTypeIdChecker ())
   .AddAttribute ("Congestion", "The type of protocol to use.",
			   TypeIdValue (TcpMpTcp::GetTypeId ()),
			   MakeTypeIdAccessor (&PacketMpSink::m_congestion),
			   MakeTypeIdChecker ())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&PacketMpSink::m_rxTrace))
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&PacketMpSink::m_txTrace))
  ;
  return tid;
}

PacketMpSink::PacketMpSink ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
}

PacketMpSink::~PacketMpSink()
{
  NS_LOG_FUNCTION (this);
}

uint32_t PacketMpSink::GetTotalRx () const
{
  return m_totalRx;
}

Ptr<Socket>
PacketMpSink::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
PacketMpSink::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void PacketMpSink::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}

// Application Methods
void PacketMpSink::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid,m_socketType,m_congestion);
      m_socket->Bind (m_local);
      m_socket->Listen ();
      m_socket->ShutdownSend ();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }
  NS_LOG_FUNCTION(this<<m_socket);
  m_socket->SetRecvCallback (MakeCallback (&PacketMpSink::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&PacketMpSink::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&PacketMpSink::HandlePeerClose, this),
    MakeCallback (&PacketMpSink::HandlePeerError, this));
}

void PacketMpSink::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void PacketMpSink::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      NS_LOG_LOGIC("Total rx " << m_totalRx);
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      m_rxTrace (packet, from);
    }
}


void PacketMpSink::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 
void PacketMpSink::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}
 

void PacketMpSink::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  //s->SetRecvCallback (MakeCallback (&PacketMpSink::HandleRead, this));
  //m_socketList.push_back (s);
  NS_LOG_FUNCTION(this << " Handle accept");
//s->SetRecvCallback (MakeCallback (&MpMultimediaApplicationReceiver::OnReceive, this));
  s->GetObject<MpTcpSocket>()->SetRecvCallback(MakeCallback (&PacketMpSink::GetPacket, this));

}

void
PacketMpSink::GetPacket (Ptr<Packet> p, SequenceNumber64 seq,Ptr<TcpSocketBase> socket)
{
	  NS_LOG_FUNCTION(this << p << seq << socket << p->GetSize());
	  m_txTrace(p,socket);

}

} // Namespace ns3
