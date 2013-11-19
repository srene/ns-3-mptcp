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
#include "tcp-newreno.h"
//#include "ns3/sha-digest.h"

NS_LOG_COMPONENT_DEFINE ("TcpSocket");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpSocket);


TypeId
TcpSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocket")
    .SetParent<Socket> ()
    .AddConstructor<TcpSocket> ()
    /*.AddAttribute ("SocketType",
                   "Socket type of TCP objects.",
                   TypeIdValue (TcpNewReno::GetTypeId ()),
                   MakeTypeIdAccessor (&TcpSocket::m_socketTypeId),
                   MakeTypeIdChecker ())
    .AddAttribute ("SndBufSize",
                    "TcpSocket maximum transmit buffer size (bytes)",
                    UintegerValue (131072), // 128k
                    MakeUintegerAccessor (&TcpSocketBase::GetSndBufSize,
                                          &TcpSocketBase::SetSndBufSize),
                    MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("RcvBufSize",
				   "TcpSocket maximum receive buffer size (bytes)",
				   UintegerValue (131072),
				   MakeUintegerAccessor (&TcpSocketBase::GetRcvBufSize,
										 &TcpSocketBase::SetRcvBufSize),
                   MakeUintegerChecker<uint32_t> ())*/

  ;
  return tid;
}

TcpSocket::TcpSocket ()
 //: m_tcpSocketBase(0)
// : m_txBuffer (0),
//   m_rxBuffer (0),
 : m_errno (Socket::ERROR_NOTERROR),
   m_shutdownSend(false),
   m_shutdownRecv(false)
{
	//CreateSocket(TcpNewReno::GetTypeId ());
}

TcpSocket::TcpSocket (TypeId socketTypeId)
 //: m_tcpSocketBase(0)
// : m_txBuffer (0),
//   m_rxBuffer (0),
 : m_succeed(false),
   m_errno (Socket::ERROR_NOTERROR),
   m_shutdownSend(false),
   m_shutdownRecv(false)
{
	CreateSocket(socketTypeId);
}

TcpSocket::~TcpSocket ()
{
  NS_LOG_FUNCTION_NOARGS ();
}
/** Associate a node with this TCP socket */

void
TcpSocket::CreateSocket(TypeId socketTypeId)
{
	  NS_LOG_FUNCTION_NOARGS ();
	  ObjectFactory socketBaseFactory;
	  socketBaseFactory.SetTypeId (socketTypeId);
	  Ptr<TcpSocketBase> socketBase = socketBaseFactory.Create<TcpSocketBase> ();
	  m_sockets.push_back(socketBase);
	  //m_tcpSocketBase = Create<TcpNewReno>();
	  m_sockets[0]->SetSocket(this);
}
void
TcpSocket::SetNode (Ptr<Node> node)
{
	m_node = node;
	m_sockets[0]->SetNode(node);
}

/** Associate the L4 protocol (e.g. mux/demux) with this socket */
void
TcpSocket::SetTcp (Ptr<TcpL4Protocol> tcp)
{
	m_sockets[0]->SetTcp(tcp);
	m_tcp = tcp;
}

int
TcpSocket::GetPending(void)
{
	return  m_sockets[0]->GetPending();
}

bool
TcpSocket::GetSucceed()
{
	return m_succeed;
}

/** Set an RTT estimator with this socket */
void
TcpSocket::SetRtt (Ptr<RttEstimator> rtt)
{
	m_sockets[0]->SetRtt(rtt);
}

/** Set an RTT estimator with this socket */
void
TcpSocket::SetSocketBase (Ptr<TcpSocketBase> socketBase)
{
	m_sockets.push_back(socketBase);
	m_sockets[0]->SetSocket(this);
}

void
TcpSocket::SetSndBufSize (uint32_t size)
{
	m_sockets[0]->SetSndBufSize(size);
}

uint32_t
TcpSocket::GetSndBufSize (void) const
{
	return m_sockets[0]->GetRcvBufSize();
}

void
TcpSocket::SetRcvBufSize (uint32_t size)
{
	m_sockets[0]->SetRcvBufSize(size);
}

uint32_t
TcpSocket::GetRcvBufSize (void) const
{
	return m_sockets[0]->GetRcvBufSize();
}

void
TcpSocket::SetSegSize (uint32_t size)
{
	m_sockets[0]->SetSegSize(size);
}

uint32_t
TcpSocket::GetSegSize (void) const
{
	return m_sockets[0]->GetSegSize();
}
void
TcpSocket::SetSSThresh (uint32_t threshold)
{
	m_sockets[0]->SetSSThresh(threshold);
}

uint32_t
TcpSocket::GetSSThresh (void) const
{
	return m_sockets[0]->GetSSThresh();
}

void
TcpSocket::SetInitialCwnd (uint32_t count)
{
	m_sockets[0]->SetInitialCwnd(count);
}

uint32_t
TcpSocket::GetInitialCwnd (void) const
{
	return m_sockets[0]->GetInitialCwnd();
}

void
TcpSocket::SetConnTimeout (Time timeout)
{
	m_sockets[0]->SetConnTimeout(timeout);
}

Time
TcpSocket::GetConnTimeout (void) const
{
	return m_sockets[0]->GetConnTimeout();
}

void
TcpSocket::SetConnCount (uint32_t count)
{
	m_sockets[0]->SetConnCount(count);
}

uint32_t
TcpSocket::GetConnCount (void) const
{
	return m_sockets[0]->GetConnCount();
}

void
TcpSocket::SetDelAckTimeout (Time timeout)
{
	m_sockets[0]->SetDelAckTimeout(timeout);
}

Time
TcpSocket::GetDelAckTimeout (void) const
{
	return m_sockets[0]->GetDelAckTimeout();
}

void
TcpSocket::SetDelAckMaxCount (uint32_t count)
{
	m_sockets[0]->SetDelAckMaxCount(count);

}

uint32_t
TcpSocket::GetDelAckMaxCount (void) const
{
	return m_sockets[0]->GetDelAckMaxCount();
}

void
TcpSocket::SetTcpNoDelay (bool noDelay)
{
	m_sockets[0]->SetTcpNoDelay(noDelay);
}

bool
TcpSocket::GetTcpNoDelay (void) const
{
	return m_sockets[0]->GetTcpNoDelay();
}

void
TcpSocket::SetPersistTimeout (Time timeout)
{
	m_sockets[0]->SetPersistTimeout(timeout);
}

Time
TcpSocket::GetPersistTimeout (void) const
{
	return m_sockets[0]->GetPersistTimeout();
}


/** Inherit from Socket class: Returns error code */
enum Socket::SocketErrno
TcpSocket::GetErrno (void) const
{
  return m_sockets[0]->GetErrno();
}

bool
TcpSocket::GetTcpMpCapable(void)
{
	NS_LOG_FUNCTION (this);
	return false;
}

/** Inherit from Socket class: Returns socket type, NS3_SOCK_STREAM */
enum Socket::SocketType
TcpSocket::GetSocketType (void) const
{
  return NS3_SOCK_STREAM;
}

/** Inherit from Socket class: Returns associated node */
Ptr<Node>
TcpSocket::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_sockets[0]->GetNode();
}

/** Inherit from Socket class: Bind socket to an end-point in TcpL4Protocol */
int
TcpSocket::Bind (void)
{
  NS_LOG_FUNCTION (this);
  int value = m_sockets[0]->Allocate();
 if (value ==-1)
	  m_errno = ERROR_ADDRNOTAVAIL;
  return value;
}

int
TcpSocket::Bind6 (void)
{
  NS_LOG_FUNCTION (this);
  int value = m_sockets[0]->Allocate6();
  if (value ==-1)
	  m_errno = ERROR_ADDRNOTAVAIL;
  return value;

}

/** Inherit from Socket class: Bind socket (with specific address) to an end-point in TcpL4Protocol */
int
TcpSocket::Bind (const Address &address)
{
  NS_LOG_FUNCTION (this << address);
  int value;

  if (InetSocketAddress::IsMatchingType (address))
    {
      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      Ipv4Address ipv4 = transport.GetIpv4 ();
      uint16_t port = transport.GetPort ();
      if (ipv4 == Ipv4Address::GetAny () && port == 0)
        {
    	  value = m_sockets[0]->Allocate ();
        }
      else if (ipv4 == Ipv4Address::GetAny () && port != 0)
        {
          value = m_sockets[0]->Allocate (port);
        }
      else if (ipv4 != Ipv4Address::GetAny () && port == 0)
        {
          value = m_sockets[0]->Allocate (ipv4);
        }
      else if (ipv4 != Ipv4Address::GetAny () && port != 0)
        {
    	  value = m_sockets[0]->Allocate (ipv4, port);
        }
      if (value == -1)
        {
          m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
          return -1;
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (address))
    {
      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address ipv6 = transport.GetIpv6 ();
      uint16_t port = transport.GetPort ();
      if (ipv6 == Ipv4Address::GetAny () && port == 0)
          {
      	  value = m_sockets[0]->Allocate6 ();
          }
        else if (ipv6 == Ipv4Address::GetAny () && port != 0)
          {
            value = m_sockets[0]->Allocate6 (port);
          }
        else if (ipv6 != Ipv4Address::GetAny () && port == 0)
          {
            value = m_sockets[0]->Allocate6 (ipv6);
          }
        else if (ipv6 != Ipv4Address::GetAny () && port != 0)
          {
      	  value = m_sockets[0]->Allocate6 (ipv6, port);
          }
        if (value == -1)
          {
            m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
            return -1;
          }
    }
  else
    {
      m_errno = ERROR_INVAL;
      return -1;
    }

  return value;
}

/** Inherit from Socket class: Initiate connection to a remote address:port */
int
TcpSocket::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << InetSocketAddress::ConvertFrom (address).GetIpv4());
  int result =  m_sockets[0]->Connect(address);
  if(result==-1)
		m_errno = ERROR_INVAL;
  return result;

}

bool
TcpSocket::GetAllowBroadcast (void) const
{
  return false;
}

bool
TcpSocket::SetAllowBroadcast (bool allowBroadcast)
{
  // Broadcast is not implemented. Return true only if allowBroadcast==false
  return (!allowBroadcast);
}

/** Inherit from Socket class: Listen on the endpoint for an incoming connection */
int
TcpSocket::Listen (void)
{
  NS_LOG_FUNCTION (this);
  // Linux quits EINVAL if we're not in CLOSED state, so match what they do
  if (m_sockets[0]->GetState() != CLOSED)
    {
      m_errno = ERROR_INVAL;
      return -1;
    }
  // In other cases, set the state to LISTEN and done
  NS_LOG_INFO ("CLOSED -> LISTEN");
  m_sockets[0]->SetState(LISTEN);
  return 0;
}

/** Inherit from Socket class: Kill this socket and signal the peer (if any) */
int
TcpSocket::Close (void)
{
	NS_LOG_FUNCTION (this);
	//return m_sockets[0]->Close();
	  int i = -1;
	  for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin (); it != m_sockets.end (); it++)
	    {
		  i = (Ptr<TcpSocketBase>(*it))->Close();
	    }
	  NS_LOG_FUNCTION (this << i);
/*
	  for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_rxSockets.begin (); it != m_rxSockets.end (); it++)
	    {
		  i = (Ptr<TcpSocketBase>(*it))->Close();
	    }*/
	  return i;
}

/** Inherit from Socket class: Signal a termination of send */
int
TcpSocket::ShutdownSend (void)
{
  NS_LOG_FUNCTION (this);
  m_sockets[0]->SetShutdownSend(true);
  return 0;
}

/** Inherit from Socket class: Signal a termination of receive */
int
TcpSocket::ShutdownRecv (void)
{
  NS_LOG_FUNCTION (this);
  m_sockets[0]->SetShutdownRecv(true);
  return 0;
}

/** Inherit from Socket class: Send a packet. Parameter flags is not used.
    Packet has no TCP header. Invoked by upper-layer application */
int
TcpSocket::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p);
  //return m_sockets[0]->Send(p,flags);
  int i = -1;
  for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_sockets.begin (); it != m_sockets.end (); it++)
    {
	  i = (Ptr<TcpSocketBase>(*it))->Send(p,flags);
    }

  return i;
}

/** Inherit from Socket class: In TcpSocket, it is same as Send() call */
int
TcpSocket::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
{
  return Send (p, flags); // SendTo() and Send() are the same
}

void
TcpSocket::ResetCallback(void)
{
	  Callback<void, Ptr< Socket > > vPS = MakeNullCallback<void, Ptr<Socket> > ();
	  Callback<void, Ptr<Socket>, const Address &> vPSA = MakeNullCallback<void, Ptr<Socket>, const Address &> ();
	  Callback<void, Ptr<Socket>, uint32_t> vPSUI = MakeNullCallback<void, Ptr<Socket>, uint32_t> ();
	  SetConnectCallback (vPS, vPS);
	  SetDataSentCallback (vPSUI);
	  SetSendCallback (vPSUI);
	  SetRecvCallback (vPS);
}
/** Inherit from Socket class: Return data to upper-layer application. Parameter flags
    is not used. Data is returned as a packet of size no larger than maxSize */
Ptr<Packet>
TcpSocket::Recv (uint32_t maxSize, uint32_t flags)
{
  NS_LOG_FUNCTION (this);
  for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_rxSockets.begin (); it != m_rxSockets.end (); it++)
    {
	  Ptr<Packet> packet = (Ptr<TcpSocketBase>(*it))->Recv(maxSize,flags);
      if(packet != 0 && packet->GetSize () != 0)
      {
    	  return packet;
      }
    }
  return NULL;

}

void
TcpSocket::SetRxSocket(Ptr<TcpSocketBase> socketBase)
{
	NS_LOG_FUNCTION (this << socketBase);
	m_rxSockets.push_back(socketBase);

	for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_rxSockets.begin() ; it != m_rxSockets.end(); ++it)
	{
		(*it)->SetSucceed();
	}
}
/** Inherit from Socket class: Recv and return the remote's address */
Ptr<Packet>
TcpSocket::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
{
  NS_LOG_FUNCTION (this << maxSize << flags << fromAddress);

  for (std::vector<Ptr<TcpSocketBase> >::iterator it = m_rxSockets.begin (); it != m_rxSockets.end (); it++)
    {
	  Ptr<Packet> packet = (Ptr<TcpSocketBase>(*it))->RecvFrom(maxSize,flags,fromAddress);
      if(packet != 0 && packet->GetSize () != 0)
      {
    	  return packet;
      }
    }
  return NULL;

}

void
TcpSocket::NotifyDataReceived(void)
{
	NotifyDataRecv();
}

/** Inherit from Socket class: Get the max number of bytes an app can send */
uint32_t
TcpSocket::GetTxAvailable (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sockets[0]->GetTxAvailable();
}

/** Inherit from Socket class: Get the max number of bytes an app can read */
uint32_t
TcpSocket::GetRxAvailable (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sockets[0]->GetRxAvailable();
}

/** Inherit from Socket class: Return local address:port */
int
TcpSocket::GetSockName (Address &address) const
{
	return m_sockets[0]->GetSocketName(address);
}

/** Inherit from Socket class: Bind this socket to the specified NetDevice */
void
TcpSocket::BindToNetDevice (Ptr<NetDevice> netdevice)
{
  NS_LOG_FUNCTION (netdevice);
  Socket::BindToNetDevice (netdevice); // Includes sanity check

  return m_sockets[0]->BindToNetDevice(netdevice);
}

void
TcpSocket::SetTcpMp (bool multipath)
{
}

void
TcpSocket::Join(void)
{
}

void
TcpSocket::SetTcpMpCapable(void)
{
}

bool
TcpSocket::GetTcpMp (void) const
{
  return false;
}

void
TcpSocket::SetLocalKey (uint64_t key)
{
}
void
TcpSocket::SetRemoteKey (uint64_t key)
{
}
uint64_t
TcpSocket::GetLocalKey (void)
{
  return 0;
}
uint64_t
TcpSocket::GetRemoteKey (void)
{
  return 0;
}

void
TcpSocket::SetMpcapSent(bool sent)
{

}

bool
TcpSocket::GetMpcapSent(void)
{
	return false;
}
int
TcpSocket::NotifySubflowConnectionSucceeded(Ptr<TcpSocketBase> socket)
{
	m_succeed = true;
	return -1;
}

std::vector<Ptr<TcpSocketBase> >
TcpSocket::GetSocketsBase(void)
{
	return m_sockets;
}

} // namespace ns3
