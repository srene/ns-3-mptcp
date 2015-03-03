/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 * Copyright (c) 2010 Adrian Sai-wah Tam
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
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */


  #define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }


#include "ns3/abort.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/simulation-singleton.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "tcp-socket-base.h"
#include "tcp-l4-protocol.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include "ipv6-l3-protocol.h"
#include "tcp-header.h"
#include "rtt-estimator.h"
#include "tcp-option.h"
#include "tcp-option-mptcp.h"
#include "tcp-option-mp_cap.h"
#include "tcp-option-mp_addr.h"
#include "tcp-option-sack-permitted.h"
#include "tcp-option-mp_dss.h"
#include "mp-tcp-socket.h"
#include "ns3/node.h"
#include "tcp-socket.h"
#include "tcp-newreno.h"
#include "tcp-suboption.h"
#include "tcp-option-winscale.h"
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("TcpSocketBase");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpSocketBase);

const char* const TcpSocketBase::TcpStateName[LAST_STATE] = { "CLOSED", "LISTEN", "SYN_SENT", "SYN_RCVD", "ESTABLISHED", "CLOSE_WAIT", "LAST_ACK", "FIN_WAIT_1", "FIN_WAIT_2", "CLOSING", "TIME_WAIT" };

TypeId
TcpSocketBase::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketBase")
    .SetParent<Object> ()
//    .AddAttribute ("TcpState", "State in TCP state machine",
//                   TypeId::ATTR_GET,
//                   EnumValue (CLOSED),
//                   MakeEnumAccessor (&TcpSocketBase::m_state),
//                   MakeEnumChecker (CLOSED, "Closed"))
    //.AddConstructor<TcpNewReno> ()
    .AddAttribute ("SndBufSize",
                    "TcpSocket maximum transmit buffer size (bytes)",
                    UintegerValue (131072), // 2MB
                    MakeUintegerAccessor (&TcpSocketBase::GetSndBufSize,
                                          &TcpSocketBase::SetSndBufSize),
                    MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("RcvBufSize",
                    "TcpSocket maximum receive buffer size (bytes)",
                    UintegerValue (131072), // 2MB
                    MakeUintegerAccessor (&TcpSocketBase::GetRcvBufSize,
                                          &TcpSocketBase::SetRcvBufSize),
                    MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("SegmentSize",
                    "TCP maximum segment size in bytes (may be adjusted based on MTU discovery)",
                    UintegerValue (536),
                    MakeUintegerAccessor (&TcpSocketBase::GetSegSize,
                                          &TcpSocketBase::SetSegSize),
                    MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("SlowStartThreshold",
                    "TCP slow start threshold (bytes)",
                    UintegerValue (0xffff),
                    MakeUintegerAccessor (&TcpSocketBase::GetSSThresh,
                                          &TcpSocketBase::SetSSThresh),
                    MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("InitialCwnd",
                    "TCP initial congestion window size (segments)",
                    UintegerValue (1),
                    MakeUintegerAccessor (&TcpSocketBase::GetInitialCwnd,
                                          &TcpSocketBase::SetInitialCwnd),
                    MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("ConnTimeout",
                    "TCP retransmission timeout when opening connection (seconds)",
                    TimeValue (Seconds (3)),
                    MakeTimeAccessor (&TcpSocketBase::GetConnTimeout,
                                      &TcpSocketBase::SetConnTimeout),
                    MakeTimeChecker ())
     .AddAttribute ("ConnCount",
                    "Number of connection attempts (SYN retransmissions) before returning failure",
                    UintegerValue (6),
                    MakeUintegerAccessor (&TcpSocketBase::GetConnCount,
                                          &TcpSocketBase::SetConnCount),
                    MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("DelAckTimeout",
                    "Timeout value for TCP delayed acks, in seconds",
                    TimeValue (Seconds (0.2)),
                    MakeTimeAccessor (&TcpSocketBase::GetDelAckTimeout,
                                      &TcpSocketBase::SetDelAckTimeout),
                    MakeTimeChecker ())
     .AddAttribute ("DelAckCount",
                    "Number of packets to wait before sending a TCP ack",
                    UintegerValue (1),
                    MakeUintegerAccessor (&TcpSocketBase::GetDelAckMaxCount,
                                          &TcpSocketBase::SetDelAckMaxCount),
                    MakeUintegerChecker<uint32_t> ())
	 .AddAttribute ("WindowScaleFactor",
	    			  "window scale factor",
					  UintegerValue (0),
					  MakeUintegerAccessor (&TcpSocketBase::GetWinScale,
											&TcpSocketBase::SetWinScale),
                    MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("TcpNoDelay", "Set to true to disable Nagle's algorithm",
                    BooleanValue (true),
                    MakeBooleanAccessor (&TcpSocketBase::GetTcpNoDelay,
                                         &TcpSocketBase::SetTcpNoDelay),
                    MakeBooleanChecker ())
     .AddAttribute ("PersistTimeout",
                    "Persist timeout to probe for rx window",
                    TimeValue (Seconds (6)),
                    MakeTimeAccessor (&TcpSocketBase::GetPersistTimeout,
                                      &TcpSocketBase::SetPersistTimeout),
                    MakeTimeChecker ())
    .AddAttribute ("MaxSegLifetime",
                   "Maximum segment lifetime in seconds, use for TIME_WAIT state transition to CLOSED state",
                   DoubleValue (120), /* RFC793 says MSL=2 minutes*/
                   MakeDoubleAccessor (&TcpSocketBase::m_msl),
                   MakeDoubleChecker<double> (0))
    .AddAttribute ("MaxWindowSize", "Max size of advertised window",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&TcpSocketBase::m_maxWinSize),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("RTO",
                     "Retransmission timeout",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_rto))
    .AddTraceSource ("RTT",
                     "Last RTT sample",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_lastRtt))
    .AddTraceSource ("NextTxSequence",
                     "Next sequence number to send (SND.NXT)",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_nextTxSequence))
    .AddTraceSource ("HighestSequence",
                     "Highest sequence number ever sent in socket's life time",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_highTxMark))
    .AddTraceSource ("State",
                     "TCP state",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_state))
    .AddTraceSource ("RWND",
                     "Remote side's flow control window",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_rWnd))
	 .AddTraceSource ("Buffer",
					  "Remote side's flow control window",
					  MakeTraceSourceAccessor (&TcpSocketBase::m_buffsize))
	// .AddTraceSource("LandaRate", "The data sent rate",
     //        	   MakeDoubleAccessor (&TcpSocketBase::GetLanda,
      //       			   	   	   	   &TcpSocketBase::SetLanda),
      //       			   	   	MakeDoubleChecker<double> (0));
  ;
  return tid;
}

TcpSocketBase::TcpSocketBase (void)
  : m_dupAckCount (0),
    m_delAckCount (0),
    m_endPoint (0),
    m_endPoint6 (0),
    m_node (0),
    m_tcp (0),
    m_rtt (0),
    m_nextTxSequence (0),
    // Change this for non-zero initial sequence number
    m_highTxMark (0),
    m_rxBuffer (0),
    m_txBuffer (0),
    m_lastSeq (0),
    m_size (0),
    m_fragment(false),
    //m_prevSeq(0),
    m_frag(0),
    m_cumfrag(0),
    m_cumsize(0),
    m_fragment2(false),
    m_frag2(0),
    m_cumfrag2(0),
    m_cumsize2(0),
    m_numfrags(1),
    m_state (CLOSED),
    m_errno (Socket::ERROR_NOTERROR),
    m_closeNotified (false),
    m_closeOnEmpty (false),
    m_shutdownSend (false),
    m_shutdownRecv (false),
    m_connected (false),
    //m_msl (120),
    m_segmentSize (0),
    // For attribute initialization consistency (quiet valgrind)
    //m_maxWinSize (65535),
    m_rWnd (0),
    m_socket(0),
    //m_seqTxMap(0),
    //m_sizeTxMap(0),
    //m_sizeRxMap(0),
    m_succeed(false),
    m_boundnetdevice(0),
    m_dataSeq(0),
    m_dataAck(0),
    m_sndscale(0),
    m_buffsize(0)
{
  NS_LOG_FUNCTION (this << GetInstanceTypeId ());
}

TcpSocketBase::TcpSocketBase (const TcpSocketBase& sock)
  : Object (sock),
    //copy object::m_tid and socket::callbacks
    m_dupAckCount (sock.m_dupAckCount),
    m_delAckCount (0),
    m_delAckMaxCount (sock.m_delAckMaxCount),
    m_noDelay (sock.m_noDelay),
    m_cnRetries (sock.m_cnRetries),
    m_delAckTimeout (sock.m_delAckTimeout),
    m_persistTimeout (sock.m_persistTimeout),
    m_cnTimeout (sock.m_cnTimeout),
    m_endPoint (0),
    m_endPoint6 (0),
    m_node (sock.m_node),
    m_tcp (sock.m_tcp),
    m_rtt (0),
    m_nextTxSequence (sock.m_nextTxSequence),
    m_highTxMark (sock.m_highTxMark),
    m_rxBuffer (sock.m_rxBuffer),
    m_txBuffer (sock.m_txBuffer),
    m_lastSeq (sock.m_lastSeq),
    m_size (sock.m_size),
    m_fragment(sock.m_fragment),
    //m_prevSeq(sock.m_prevSeq),
    m_frag(sock.m_frag),
    m_cumfrag(sock.m_cumfrag),
    m_cumsize(sock.m_cumsize),
    m_fragment2(sock.m_fragment2),
    m_frag2(sock.m_frag2),
    m_cumfrag2(sock.m_cumfrag2),
    m_cumsize2(sock.m_cumsize2),
    m_numfrags(sock.m_numfrags),
    m_state (sock.m_state),
    m_errno (sock.m_errno),
    m_closeNotified (sock.m_closeNotified),
    m_closeOnEmpty (sock.m_closeOnEmpty),
    m_shutdownSend (sock.m_shutdownSend),
    m_shutdownRecv (sock.m_shutdownRecv),
    m_connected (sock.m_connected),
    m_msl (sock.m_msl),
    m_segmentSize (sock.m_segmentSize),
    m_maxWinSize (sock.m_maxWinSize),
    m_rWnd (sock.m_rWnd),
    m_socket(sock.m_socket),
    m_seqTxMap(sock.m_seqTxMap),
    m_sizeTxMap(sock.m_sizeTxMap),
    m_sizeRxMap(sock.m_sizeRxMap),
    m_succeed(sock.m_succeed),
    m_boundnetdevice(sock.m_boundnetdevice),
    m_dataSeq(sock.m_dataSeq),
    m_dataAck(sock.m_dataAck),
    m_sndscale(sock.m_sndscale),
    m_rcvscale(sock.m_rcvscale),
    m_buffsize(sock.m_buffsize)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor" << GetInstanceTypeId () << " " << m_socket->GetTcpMpCapable());
  // Copy the rtt estimator if it is set
  if (sock.m_rtt)
    {
      m_rtt = sock.m_rtt->Copy ();
    }
  // Reset all callbacks to null
  if(!m_socket->GetTcpMpCapable()){
	  Callback<void, Ptr< Socket > > vPS = MakeNullCallback<void, Ptr<Socket> > ();
	  Callback<void, Ptr<Socket>, const Address &> vPSA = MakeNullCallback<void, Ptr<Socket>, const Address &> ();
	  Callback<void, Ptr<Socket>, uint32_t> vPSUI = MakeNullCallback<void, Ptr<Socket>, uint32_t> ();

	  m_socket->SetConnectCallback (vPS, vPS);
	  m_socket->SetDataSentCallback (vPSUI);
	  m_socket->SetSendCallback (vPSUI);
	  m_socket->SetRecvCallback (vPS);

  }
  m_socket->SetRxSocket(this);

}


TcpSocketBase::~TcpSocketBase (void)
{
  NS_LOG_FUNCTION (this);
//  m_node = 0;
  if (m_endPoint != 0)
    {
      NS_ASSERT (m_tcp != 0);
      /*
       * Upon Bind, an Ipv4Endpoint is allocated and set to m_endPoint, and
       * DestroyCallback is set to TcpSocketBase::Destroy. If we called
       * m_tcp->DeAllocate, it wil destroy its Ipv4EndpointDemux::DeAllocate,
       * which in turn destroys my m_endPoint, and in turn invokes
       * TcpSocketBase::Destroy to nullify m_node, m_endPoint, and m_tcp.
       */
      NS_ASSERT (m_endPoint != 0);
      m_tcp->DeAllocate (m_endPoint);
      NS_ASSERT (m_endPoint == 0);
    }
  if (m_endPoint6 != 0)
    {
      NS_ASSERT (m_tcp != 0);
      NS_ASSERT (m_endPoint6 != 0);
      m_tcp->DeAllocate (m_endPoint6);
      NS_ASSERT (m_endPoint6 == 0);
    }
  m_tcp = 0;
  CancelAllTimers ();
}

int
TcpSocketBase::Allocate(void)
{
	  NS_LOG_FUNCTION (this);

	  m_endPoint = m_tcp->Allocate ();
	  if (0 == m_endPoint)
	    {
	      return -1;
	    }

	  m_tcp->m_sockets.push_back (this);
	  return SetupCallback ();
}


int
TcpSocketBase::Allocate(uint16_t port)
{
	  NS_LOG_FUNCTION (this);

	  m_endPoint = m_tcp->Allocate (port);
	  if (0 == m_endPoint)
	    {
	      return -1;
	    }

	  m_tcp->m_sockets.push_back (this);
	  return SetupCallback ();
}

int
TcpSocketBase::Allocate(Ipv4Address ipv4)
{
	  NS_LOG_FUNCTION (this);

	  m_endPoint = m_tcp->Allocate (ipv4);
	  if (0 == m_endPoint)
	    {
	      return -1;
	    }

	  m_tcp->m_sockets.push_back (this);
	  return SetupCallback ();
}

int
TcpSocketBase::Allocate(Ipv4Address ipv4,uint16_t port)
{
	  NS_LOG_FUNCTION (this);

	  m_endPoint = m_tcp->Allocate (ipv4,port);
	  if (0 == m_endPoint)
	    {
	      return -1;
	    }

	  m_tcp->m_sockets.push_back (this);
	  return SetupCallback ();
}
int
TcpSocketBase::Allocate6(void)
{
	  NS_LOG_FUNCTION (this);

	  m_endPoint6 = m_tcp->Allocate6 ();
	   if (0 == m_endPoint6)
	     {
	       return -1;
	     }
	   m_tcp->m_sockets.push_back (this);
	   return SetupCallback ();
}
int
TcpSocketBase::Allocate6(uint16_t port)
{
	  NS_LOG_FUNCTION (this);

	  m_endPoint6 = m_tcp->Allocate6 (port);
	  if (0 == m_endPoint6)
	    {
	      return -1;
	    }

	  m_tcp->m_sockets.push_back (this);
	  return SetupCallback ();
}

int
TcpSocketBase::Allocate6(Ipv6Address ipv6)
{
	  NS_LOG_FUNCTION (this);

	  m_endPoint6 = m_tcp->Allocate6 (ipv6);
	  if (0 == m_endPoint6)
	    {
	      return -1;
	    }

	  m_tcp->m_sockets.push_back (this);
	  return SetupCallback ();
}

int
TcpSocketBase::Allocate6(Ipv6Address ipv6,uint16_t port)
{
	  NS_LOG_FUNCTION (this);

	  m_endPoint6 = m_tcp->Allocate6 (ipv6,port);
	  if (0 == m_endPoint6)
	    {
	      return -1;
	    }

	  m_tcp->m_sockets.push_back (this);
	  return SetupCallback ();
}


int
TcpSocketBase::Connect(const Address& address)
{
	// If haven't do so, Bind() this socket first
	if (InetSocketAddress::IsMatchingType (address) && m_endPoint6 == 0)
	  {
		if (m_endPoint == 0)
		  {
			if (Allocate() == -1)
			  {
				NS_ASSERT (m_endPoint == 0);
				return -1; // Bind() failed
			  }
			NS_ASSERT (m_endPoint != 0);
		  }
		InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
		m_endPoint->SetPeer (transport.GetIpv4 (), transport.GetPort ());
		m_endPoint6 = 0;

		// Get the appropriate local address and port number from the routing protocol and set up endpoint
		if (SetupEndpoint () != 0)
		  { // Route to destination does not exist
			return -1;
		  }
	  }
	else if (Inet6SocketAddress::IsMatchingType (address)  && m_endPoint == 0)
	  {
		// If we are operating on a v4-mapped address, translate the address to
		// a v4 address and re-call this function
		Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
		Ipv6Address v6Addr = transport.GetIpv6 ();
		if (v6Addr.IsIpv4MappedAddress () == true)
		  {
			Ipv4Address v4Addr = v6Addr.GetIpv4MappedAddress ();
			return Connect (InetSocketAddress (v4Addr, transport.GetPort ()));
		  }

		if (m_endPoint6 == 0)
		  {
			if (Allocate6() == -1)
			  {
				NS_ASSERT (m_endPoint6 == 0);
				return -1; // Bind() failed
			  }
			NS_ASSERT (m_endPoint6 != 0);
		  }
		m_endPoint6->SetPeer (v6Addr, transport.GetPort ());
		m_endPoint = 0;

		// Get the appropriate local address and port number from the routing protocol and set up endpoint
		if (SetupEndpoint6 () != 0)
		  { // Route to destination does not exist
			return -1;
		  }
	  }
	else
	  {
		return -1;
	  }

	// Re-initialize parameters in case this socket is being reused after CLOSE
	m_rtt->Reset ();
	m_cnCount = m_cnRetries;

	// DoConnect() will do state-checking and send a SYN packet
	return DoConnect ();
}
/** Clean up after Bind. Set up callback functions in the end-point. */
int
TcpSocketBase::SetupCallback (void)
{
  NS_LOG_FUNCTION (this);

  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      return -1;
    }
  if (m_endPoint != 0)
    {
      m_endPoint->SetRxCallback (MakeCallback (&TcpSocketBase::ForwardUp, Ptr<TcpSocketBase> (this)));
      m_endPoint->SetDestroyCallback (MakeCallback (&TcpSocketBase::Destroy, Ptr<TcpSocketBase> (this)));
    }
  if (m_endPoint6 != 0)
    {
      m_endPoint6->SetRxCallback (MakeCallback (&TcpSocketBase::ForwardUp6, Ptr<TcpSocketBase> (this)));
      m_endPoint6->SetDestroyCallback (MakeCallback (&TcpSocketBase::Destroy6, Ptr<TcpSocketBase> (this)));
    }

  return 0;
}

/** Perform the real connection tasks: Send SYN if allowed, RST if invalid */
int
TcpSocketBase::DoConnect (void)
{
  NS_LOG_FUNCTION (this);

  // A new connection is allowed only if this socket does not have a connection
  if (m_state == CLOSED || m_state == LISTEN || m_state == SYN_SENT || m_state == LAST_ACK || m_state == CLOSE_WAIT)
    { // send a SYN packet and change state into SYN_SENT
      SendEmptyPacket (TcpHeader::SYN);
      NS_LOG_INFO (TcpStateName[m_state] << " -> SYN_SENT");
      m_state = SYN_SENT;
    }
  else if (m_state != TIME_WAIT)
    { // In states SYN_RCVD, ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, and CLOSING, an connection
      // exists. We send RST, tear down everything, and close this socket.
      SendRST ();
      CloseAndNotify ();
    }
  return 0;
}

/** Do the action to close the socket. Usually send a packet with appropriate
    flags depended on the current m_state. */
int
TcpSocketBase::DoClose (void)
{
  NS_LOG_FUNCTION (this);
	/*for(std::map<SequenceNumber32,SequenceNumber64>::iterator iter = seqTxMap.begin(); iter != seqTxMap.end(); iter++)
	{
	  std::map<SequenceNumber64,uint32_t>::iterator it;
	  it=sizeTxMap.find(iter->second);
	  NS_LOG_FUNCTION(this << " DSS Sent Ack2:" << iter->first << " " << iter->second << " " << it->second);
	}*/
  switch (m_state)
    {
    case SYN_RCVD:
    case ESTABLISHED:
      // send FIN to close the peer
      SendEmptyPacket (TcpHeader::FIN);
      NS_LOG_INFO ("ESTABLISHED -> FIN_WAIT_1");
      m_state = FIN_WAIT_1;
      break;
    case CLOSE_WAIT:
      // send FIN+ACK to close the peer
      SendEmptyPacket (TcpHeader::FIN | TcpHeader::ACK);
      NS_LOG_INFO ("CLOSE_WAIT -> LAST_ACK");
      m_state = LAST_ACK;
      break;
    case SYN_SENT:
    case CLOSING:
      // Send RST if application closes in SYN_SENT and CLOSING
      SendRST ();
      CloseAndNotify ();
      break;
    case LISTEN:
    case LAST_ACK:
      // In these three states, move to CLOSED and tear down the end point
      CloseAndNotify ();
      break;
    case CLOSED:
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case TIME_WAIT:
    default: /* mute compiler */
      // Do nothing in these four states
      break;
    }
  return 0;
}

int
TcpSocketBase::Close(void)
{
  NS_LOG_FUNCTION (this);
  // First we check to see if there is any unread rx data
  // Bug number 426 claims we should send reset in this case.
  NS_LOG_LOGIC("rxbuffer " << m_rxBuffer.Size ());
  if (m_rxBuffer.Size () != 0)
	{
	  SendRST ();
	  return 0;
	}

  if (m_txBuffer.SizeFromSequence (m_nextTxSequence) > 0)
  //if(m_socket->GetPending() > 0)
	{ // App close with pending data must wait until all data transmitted
	  if (m_closeOnEmpty == false)
		{
		  m_closeOnEmpty = true;
		  NS_LOG_INFO ("Socket " << this << " deferring close, state " << TcpStateName[m_state]);
		}
	  return 0;
	}
  return DoClose ();
}
/** Peacefully close the socket by notifying the upper layer and deallocate end point */
void
TcpSocketBase::CloseAndNotify (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_closeNotified)
    {
      m_socket->NotifyNormalClose ();
    }
  if (m_state != TIME_WAIT)
    {
      DeallocateEndPoint ();
    }
  m_closeNotified = true;
  NS_LOG_INFO (TcpStateName[m_state] << " -> CLOSED");
  CancelAllTimers ();
  m_state = CLOSED;
}


int
TcpSocketBase::GetPending()
{
	return m_txBuffer.SizeFromSequence (m_nextTxSequence);
}


/** Tell if a sequence number range is out side the range that my rx buffer can
    accpet */
bool
TcpSocketBase::OutOfRange (SequenceNumber32 head, SequenceNumber32 tail) const
{
  if (m_state == LISTEN || m_state == SYN_SENT || m_state == SYN_RCVD)
    { // Rx buffer in these states are not initialized.
      return false;
    }
  if (m_state == LAST_ACK || m_state == CLOSING || m_state == CLOSE_WAIT)
    { // In LAST_ACK and CLOSING states, it only wait for an ACK and the
      // sequence number must equals to m_rxBuffer.NextRxSequence ()
      return (m_rxBuffer.NextRxSequence () != head);
    }

  // In all other cases, check if the sequence number is in range
  return (tail < m_rxBuffer.NextRxSequence () || m_rxBuffer.MaxRxSequence () <= head);
}

/** Function called by the L3 protocol when it received a packet to pass on to
    the TCP. This function is registered as the "RxCallback" function in
    SetupCallback(), which invoked by Bind(), and CompleteFork() */
void
TcpSocketBase::ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                          Ptr<Ipv4Interface> incomingInterface)
{
  DoForwardUp (packet, header, port, incomingInterface);
}

void
TcpSocketBase::ForwardUp6 (Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr, uint16_t port)
{
  DoForwardUp (packet, saddr, daddr, port);
}

/** The real function to handle the incoming packet from lower layers. This is
    wrapped by ForwardUp() so that this function can be overloaded by daughter
    classes. */
void
TcpSocketBase::DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                            Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_LOGIC ("Socket " << this << " forward up " <<
                m_endPoint->GetPeerAddress () <<
                ":" << m_endPoint->GetPeerPort () <<
                " to " << m_endPoint->GetLocalAddress () <<
                ":" << m_endPoint->GetLocalPort ());


  Address fromAddress = InetSocketAddress (header.GetSource (), port);
  Address toAddress = InetSocketAddress (header.GetDestination (), m_endPoint->GetLocalPort ());

  // Peel off TCP header and do validity checking
  TcpHeader tcpHeader;
  packet->RemoveHeader (tcpHeader);
  if (tcpHeader.GetFlags () & TcpHeader::ACK)
    {
      EstimateRtt (tcpHeader);
    }
  ReadOptions (tcpHeader);
  NS_LOG_FUNCTION (this << tcpHeader << " " << m_rWnd.Get () << tcpHeader.GetWindowSize());

  // Update Rx window size, i.e. the flow control window
  if (m_rWnd.Get () == 0 && tcpHeader.GetWindowSize () != 0)
    { // persist probes end
      NS_LOG_LOGIC (this << " Leaving zerowindow persist state");
      m_persistEvent.Cancel ();
    }
  //m_rWnd = tcpHeader.GetWindowSize ();
  if(tcpHeader.GetFlags () & TcpHeader::SYN)
	  m_rWnd = tcpHeader.GetWindowSize ();
  else
	  m_rWnd = tcpHeader.GetWindowSize () << m_sndscale;

  // Discard fully out of range data packets
  if (packet->GetSize ()
      && OutOfRange (tcpHeader.GetSequenceNumber (), tcpHeader.GetSequenceNumber () + packet->GetSize ()))
    {
      NS_LOG_LOGIC ("At state " << TcpStateName[m_state] <<
                    " received packet of seq [" << tcpHeader.GetSequenceNumber () <<
                    ":" << tcpHeader.GetSequenceNumber () + packet->GetSize () <<
                    ") out of range [" << m_rxBuffer.NextRxSequence () << ":" <<
                    m_rxBuffer.MaxRxSequence () << ")");
      // Acknowledgement should be sent for all unacceptable packets (RFC793, p.69)
      if (m_state == ESTABLISHED && !(tcpHeader.GetFlags () & TcpHeader::RST))
        {
          SendEmptyPacket (TcpHeader::ACK);
        }
      return;
    }

  // TCP state machine code in different process functions
  // C.f.: tcp_rcv_state_process() in tcp_input.c in Linux kernel
  switch (m_state)
    {
    case ESTABLISHED:
      ProcessEstablished (packet, tcpHeader);
      break;
    case LISTEN:
      ProcessListen (packet, tcpHeader, fromAddress, toAddress);
      break;
    case TIME_WAIT:
      // Do nothing
      break;
    case CLOSED:
      // Send RST if the incoming packet is not a RST
      if ((tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG)) != TcpHeader::RST)
        { // Since m_endPoint is not configured yet, we cannot use SendRST here
          TcpHeader h;
          h.SetFlags (TcpHeader::RST);
          h.SetSequenceNumber (m_nextTxSequence);
          h.SetAckNumber (m_rxBuffer.NextRxSequence ());
          h.SetSourcePort (tcpHeader.GetDestinationPort ());
          h.SetDestinationPort (tcpHeader.GetSourcePort ());
          h.SetWindowSize (AdvertisedWindowSize ());
          AddOptions (h,0);
          m_tcp->SendPacket (Create<Packet> (), h, header.GetDestination (), header.GetSource (), m_boundnetdevice);
        }
      break;
    case SYN_SENT:
      ProcessSynSent (packet, tcpHeader);
      break;
    case SYN_RCVD:
      ProcessSynRcvd (packet, tcpHeader, fromAddress, toAddress);
      break;
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case CLOSE_WAIT:
      ProcessWait (packet, tcpHeader);
      break;
    case CLOSING:
      ProcessClosing (packet, tcpHeader);
      break;
    case LAST_ACK:
      ProcessLastAck (packet, tcpHeader);
      break;
    default: // mute compiler
      break;
    }
}

void
TcpSocketBase::DoForwardUp (Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr, uint16_t port)
{
  NS_LOG_LOGIC ("Socket " << this << " forward up " <<
                m_endPoint6->GetPeerAddress () <<
                ":" << m_endPoint6->GetPeerPort () <<
                " to " << m_endPoint6->GetLocalAddress () <<
                ":" << m_endPoint6->GetLocalPort ());
  Address fromAddress = Inet6SocketAddress (saddr, port);
  Address toAddress = Inet6SocketAddress (daddr, m_endPoint6->GetLocalPort ());

  // Peel off TCP header and do validity checking
  TcpHeader tcpHeader;
  packet->RemoveHeader (tcpHeader);
  if (tcpHeader.GetFlags () & TcpHeader::ACK)
    {
      EstimateRtt (tcpHeader);
    }
  ReadOptions (tcpHeader);

  // Update Rx window size, i.e. the flow control window
  if (m_rWnd.Get () == 0 && tcpHeader.GetWindowSize () != 0)
    { // persist probes end
      NS_LOG_LOGIC (this << " Leaving zerowindow persist state");
      m_persistEvent.Cancel ();
    }
  //m_rWnd = tcpHeader.GetWindowSize ();
  if(tcpHeader.GetFlags () & TcpHeader::SYN)
	  m_rWnd = tcpHeader.GetWindowSize ();
  else
	  m_rWnd = tcpHeader.GetWindowSize () << m_sndscale;
  // Discard fully out of range packets
  if (packet->GetSize ()
      && OutOfRange (tcpHeader.GetSequenceNumber (), tcpHeader.GetSequenceNumber () + packet->GetSize ()))
    {
      NS_LOG_LOGIC ("At state " << TcpStateName[m_state] <<
                    " received packet of seq [" << tcpHeader.GetSequenceNumber () <<
                    ":" << tcpHeader.GetSequenceNumber () + packet->GetSize () <<
                    ") out of range [" << m_rxBuffer.NextRxSequence () << ":" <<
                    m_rxBuffer.MaxRxSequence () << ")");
      // Acknowledgement should be sent for all unacceptable packets (RFC793, p.69)
      if (m_state == ESTABLISHED && !(tcpHeader.GetFlags () & TcpHeader::RST))
        {
          SendEmptyPacket (TcpHeader::ACK);
        }
      return;
    }

  // TCP state machine code in different process functions
  // C.f.: tcp_rcv_state_process() in tcp_input.c in Linux kernel
  switch (m_state)
    {
    case ESTABLISHED:
      ProcessEstablished (packet, tcpHeader);
      break;
    case LISTEN:
      ProcessListen (packet, tcpHeader, fromAddress, toAddress);
      break;
    case TIME_WAIT:
      // Do nothing
      break;
    case CLOSED:
      // Send RST if the incoming packet is not a RST
      if ((tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG)) != TcpHeader::RST)
        { // Since m_endPoint is not configured yet, we cannot use SendRST here
          TcpHeader h;
          h.SetFlags (TcpHeader::RST);
          h.SetSequenceNumber (m_nextTxSequence);
          h.SetAckNumber (m_rxBuffer.NextRxSequence ());
          h.SetSourcePort (tcpHeader.GetDestinationPort ());
          h.SetDestinationPort (tcpHeader.GetSourcePort ());
          h.SetWindowSize (AdvertisedWindowSize ());
          AddOptions (h,0);
          m_tcp->SendPacket (Create<Packet> (), h, daddr, saddr, 0);
        }
      break;
    case SYN_SENT:
      ProcessSynSent (packet, tcpHeader);
      break;
    case SYN_RCVD:
      ProcessSynRcvd (packet, tcpHeader, fromAddress, toAddress);
      break;
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case CLOSE_WAIT:
      ProcessWait (packet, tcpHeader);
      break;
    case CLOSING:
      ProcessClosing (packet, tcpHeader);
      break;
    case LAST_ACK:
      ProcessLastAck (packet, tcpHeader);
      break;
    default: // mute compiler
      break;
    }
}

/** Received a packet upon ESTABLISHED state. This function is mimicking the
    role of tcp_rcv_established() in tcp_input.c in Linux kernel. */
void
TcpSocketBase::ProcessEstablished (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  // Different flags are different events
  if (tcpflags == TcpHeader::ACK)
    {
      ReceivedAck (packet, tcpHeader);
    }
  else if (tcpflags == TcpHeader::SYN)
    { // Received SYN, old NS-3 behaviour is to set state to SYN_RCVD and
      // respond with a SYN+ACK. But it is not a legal state transition as of
      // RFC793. Thus this is ignored.
    }
  else if (tcpflags == (TcpHeader::SYN | TcpHeader::ACK))
    { // No action for received SYN+ACK, it is probably a duplicated packet
    }
  else if (tcpflags == TcpHeader::FIN || tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
    { // Received FIN or FIN+ACK, bring down this socket nicely
      PeerClose (packet, tcpHeader);
    }
  else if (tcpflags == 0)
    { // No flags means there is only data
      ReceivedData (packet, tcpHeader);
      if (m_rxBuffer.Finished ())
        {
          PeerClose (packet, tcpHeader);
        }
    }
  else
    { // Received RST or the TCP flags is invalid, in either case, terminate this socket
      if (tcpflags != TcpHeader::RST)
        { // this must be an invalid flag, send reset
          NS_LOG_LOGIC ("Illegal flag " << tcpflags << " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
    }
}

Ptr<Packet>
TcpSocketBase::Recv (uint32_t maxSize, uint32_t flags)
{
	NS_LOG_FUNCTION (this <<  maxSize << flags);
	NS_ABORT_MSG_IF (flags, "use of flags is not supported in TcpSocket::Recv()");
	if (m_rxBuffer.Size () == 0 && m_state == CLOSE_WAIT)
	  {
		return Create<Packet> (); // Send EOF on connection close
	  }
	Ptr<Packet> outPacket = m_rxBuffer.Extract (maxSize);
	if (outPacket != 0 && outPacket->GetSize () != 0)
	  {
		SocketAddressTag tag;
		if (m_endPoint != 0)
		  {
			tag.SetAddress (InetSocketAddress (m_endPoint->GetPeerAddress (), m_endPoint->GetPeerPort ()));
		  }
		else if (m_endPoint6 != 0)
		  {
			tag.SetAddress (Inet6SocketAddress (m_endPoint6->GetPeerAddress (), m_endPoint6->GetPeerPort ()));
		  }
		outPacket->AddPacketTag (tag);
	  }
	return outPacket;
}

void
TcpSocketBase::BindToNetDevice (Ptr<NetDevice> netdevice)
{
  NS_LOG_FUNCTION (netdevice);
  if (m_endPoint == 0 && m_endPoint6 == 0)
	{
	  if (m_socket->Bind () == -1)
		{
		  NS_ASSERT ((m_endPoint == 0 && m_endPoint6 == 0));
		  return;
		}
	  //NS_ASSERT ((m_endPoint != 0 && m_endPoint6 != 0));
	}

  if (m_endPoint != 0)
	{
	  m_endPoint->BindToNetDevice (netdevice);
	}
  m_boundnetdevice = netdevice;
  // No BindToNetDevice() for Ipv6EndPoint
  return;
}
Ptr<Packet>
TcpSocketBase::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
{
	  NS_LOG_FUNCTION (this <<  maxSize << flags);

  Ptr<Packet> packet = Recv (maxSize, flags);
  // Null packet means no data to read, and an empty packet indicates EOF
  if (packet != 0 && packet->GetSize () != 0)
	{
	  if (m_endPoint != 0)
		{
		  fromAddress = InetSocketAddress (m_endPoint->GetPeerAddress (), m_endPoint->GetPeerPort ());
		}
	  else if (m_endPoint6 != 0)
		{
		  fromAddress = Inet6SocketAddress (m_endPoint6->GetPeerAddress (), m_endPoint6->GetPeerPort ());
		}
	  else
		{
		  fromAddress = InetSocketAddress (Ipv4Address::GetZero (), 0);
		}
	}
  NS_LOG_FUNCTION (this << " address " <<  fromAddress);

  return packet;
}
/** Process the newly received ACK */
void
TcpSocketBase::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader << m_nextTxSequence << m_txBuffer.HeadSequence ());

  // Received ACK. Compare the ACK number against highest unacked seqno
  if (0 == (tcpHeader.GetFlags () & TcpHeader::ACK))
    { // Ignore if no ACK flag
    }
  else if (tcpHeader.GetAckNumber () < m_txBuffer.HeadSequence ())
    { // Case 1: Old ACK, ignored.
      NS_LOG_LOGIC ("Ignored ack of " << tcpHeader.GetAckNumber ());
    }
  else if (tcpHeader.GetAckNumber () == m_txBuffer.HeadSequence ())
    { // Case 2: Potentially a duplicated ACK
      if (tcpHeader.GetAckNumber () < m_nextTxSequence)
        {
          NS_LOG_LOGIC ("Dupack of " << tcpHeader.GetAckNumber ());
          DupAck (tcpHeader, ++m_dupAckCount);
        }
      // otherwise, the ACK is precisely equal to the nextTxSequence
      NS_ASSERT (tcpHeader.GetAckNumber () <= m_nextTxSequence);
    }
  else if (tcpHeader.GetAckNumber () > m_txBuffer.HeadSequence ())
    { // Case 3: New ACK, reset m_dupAckCount and update m_txBuffer
      NS_LOG_LOGIC ("New ack of " << tcpHeader.GetAckNumber ());
      NewAck (tcpHeader.GetAckNumber ());
      m_dupAckCount = 0;
    }
  // If there is any data piggybacked, store it into m_rxBuffer
  if (packet->GetSize () > 0)
    {
      ReceivedData (packet, tcpHeader);
    }
}

/** Received a packet upon LISTEN state. */
void
TcpSocketBase::ProcessListen (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                              const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  // Fork a socket if received a SYN. Do nothing otherwise.
  // C.f.: the LISTEN part in tcp_v4_do_rcv() in tcp_ipv4.c in Linux kernel
  if (tcpflags != TcpHeader::SYN)
    {
      return;
    }

  // Call socket's notify function to let the server app know we got a SYN
  // If the server app refuses the connection, do nothing
  if (!m_socket->NotifyConnectionRequest (fromAddress))
    {

      return;
    }
  // Clone the socket, simulate fork
  Ptr<TcpSocketBase> newSock = Fork ();
  newSock->SetSocket(m_socket);
  NS_LOG_LOGIC ("Cloned a TcpSocketBase " << newSock);
  Simulator::ScheduleNow (&TcpSocketBase::CompleteFork, newSock,
                          packet, tcpHeader, fromAddress, toAddress);
}

/** Received a packet upon SYN_SENT */
void
TcpSocketBase::ProcessSynSent (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == 0)
    { // Bare data, accept it and move to ESTABLISHED state. This is not a normal behaviour. Remove this?
      NS_LOG_INFO ("SYN_SENT -> ESTABLISHED");
      m_state = ESTABLISHED;
      m_connected = true;
      m_retxEvent.Cancel ();
      m_delAckCount = m_delAckMaxCount;
      ReceivedData (packet, tcpHeader);
      Simulator::ScheduleNow (&TcpSocketBase::ConnectionSucceeded, this);
    }
  else if (tcpflags == TcpHeader::ACK)
    { // Ignore ACK in SYN_SENT
    }
  else if (tcpflags == TcpHeader::SYN)
    { // Received SYN, move to SYN_RCVD state and respond with SYN+ACK
      NS_LOG_INFO ("SYN_SENT -> SYN_RCVD");
      m_state = SYN_RCVD;
      m_cnCount = m_cnRetries;
      m_rxBuffer.SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
    }
  else if (tcpflags == (TcpHeader::SYN | TcpHeader::ACK)
           && m_nextTxSequence + SequenceNumber32 (1) == tcpHeader.GetAckNumber ())
    { // Handshake completed
      NS_LOG_INFO ("SYN_SENT + ACK -> ESTABLISHED");
      m_state = ESTABLISHED;
      m_connected = true;
      m_retxEvent.Cancel ();
      m_rxBuffer.SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      m_highTxMark = ++m_nextTxSequence;
      m_txBuffer.SetHeadSequence (m_nextTxSequence);
      SendEmptyPacket (TcpHeader::ACK);
      Simulator::ScheduleNow (&TcpSocketBase::ConnectionSucceeded, this);
      SendPendingData (m_connected);
      // Always respond to first data packet to speed up the connection.
      // Remove to get the behaviour of old NS-3 code.
      m_delAckCount = m_delAckMaxCount;
    }
  else
    { // Other in-sequence input
      if (tcpflags != TcpHeader::RST)
        { // When (1) rx of FIN+ACK; (2) rx of FIN; (3) rx of bad flags
          NS_LOG_LOGIC ("Illegal flag " << std::hex << static_cast<uint32_t> (tcpflags) << std::dec << " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
    }
}

/** Received a packet upon SYN_RCVD */
void
TcpSocketBase::ProcessSynRcvd (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                               const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == 0
      || (tcpflags == TcpHeader::ACK
          && m_nextTxSequence + SequenceNumber32 (1) == tcpHeader.GetAckNumber ()))
    { // If it is bare data, accept it and move to ESTABLISHED state. This is
      // possibly due to ACK lost in 3WHS. If in-sequence ACK is received, the
      // handshake is completed nicely.
      NS_LOG_INFO ("SYN_RCVD -> ESTABLISHED");
      m_state = ESTABLISHED;
      m_connected = true;
      m_retxEvent.Cancel ();
      m_highTxMark = ++m_nextTxSequence;
      m_txBuffer.SetHeadSequence (m_nextTxSequence);
      if (m_endPoint)
        {
          m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                               InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
        }
      else if (m_endPoint6)
        {
          m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
        }
      // Always respond to first data packet to speed up the connection.
      // Remove to get the behaviour of old NS-3 code.
      m_delAckCount = m_delAckMaxCount;
      ReceivedAck (packet, tcpHeader);
      m_socket->NotifyNewConnectionCreated (m_socket, fromAddress);
      // As this connection is established, the socket is available to send data now
      if (GetTxAvailable () > 0)
        {
    	  m_socket->NotifySend (GetTxAvailable ());
        }
    }
  else if (tcpflags == TcpHeader::SYN)
    { // Probably the peer lost my SYN+ACK
      m_rxBuffer.SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
    }
  else if (tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
    {
      if (tcpHeader.GetSequenceNumber () == m_rxBuffer.NextRxSequence ())
        { // In-sequence FIN before connection complete. Set up connection and close.
          m_connected = true;
          m_retxEvent.Cancel ();
          m_highTxMark = ++m_nextTxSequence;
          m_txBuffer.SetHeadSequence (m_nextTxSequence);
          if (m_endPoint)
            {
              m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                   InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          else if (m_endPoint6)
            {
              m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                    Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          PeerClose (packet, tcpHeader);
        }
    }
  else
    { // Other in-sequence input
      if (tcpflags != TcpHeader::RST)
        { // When (1) rx of SYN+ACK; (2) rx of FIN; (3) rx of bad flags
          NS_LOG_LOGIC ("Illegal flag " << tcpflags << " received. Reset packet is sent.");
          if (m_endPoint)
            {
              m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                   InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          else if (m_endPoint6)
            {
              m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                    Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          SendRST ();
        }
      CloseAndNotify ();
    }
}

/** Received a packet upon CLOSE_WAIT, FIN_WAIT_1, or FIN_WAIT_2 states */
void
TcpSocketBase::ProcessWait (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (packet->GetSize () > 0 && tcpflags != TcpHeader::ACK)
    { // Bare data, accept it
      ReceivedData (packet, tcpHeader);
    }
  else if (tcpflags == TcpHeader::ACK)
    { // Process the ACK, and if in FIN_WAIT_1, conditionally move to FIN_WAIT_2
      ReceivedAck (packet, tcpHeader);
      if (m_state == FIN_WAIT_1 && m_txBuffer.Size () == 0
          && tcpHeader.GetAckNumber () == m_highTxMark + SequenceNumber32 (1))
        { // This ACK corresponds to the FIN sent
          NS_LOG_INFO ("FIN_WAIT_1 -> FIN_WAIT_2");
          m_state = FIN_WAIT_2;
        }
    }
  else if (tcpflags == TcpHeader::FIN || tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
    { // Got FIN, respond with ACK and move to next state
      if (tcpflags & TcpHeader::ACK)
        { // Process the ACK first
          ReceivedAck (packet, tcpHeader);
        }
      m_rxBuffer.SetFinSequence (tcpHeader.GetSequenceNumber ());
    }
  else if (tcpflags == TcpHeader::SYN || tcpflags == (TcpHeader::SYN | TcpHeader::ACK))
    { // Duplicated SYN or SYN+ACK, possibly due to spurious retransmission
      return;
    }
  else
    { // This is a RST or bad flags
      if (tcpflags != TcpHeader::RST)
        {
          NS_LOG_LOGIC ("Illegal flag " << tcpflags << " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
      return;
    }

  // Check if the close responder sent an in-sequence FIN, if so, respond ACK
  if ((m_state == FIN_WAIT_1 || m_state == FIN_WAIT_2) && m_rxBuffer.Finished ())
    {
      if (m_state == FIN_WAIT_1)
        {
          NS_LOG_INFO ("FIN_WAIT_1 -> CLOSING");
          m_state = CLOSING;
          if (m_txBuffer.Size () == 0
              && tcpHeader.GetAckNumber () == m_highTxMark + SequenceNumber32 (1))
            { // This ACK corresponds to the FIN sent
              TimeWait ();
            }
        }
      else if (m_state == FIN_WAIT_2)
        {
          TimeWait ();
        }
      SendEmptyPacket (TcpHeader::ACK);
      if (!m_shutdownRecv)
        {
    	  m_socket->NotifyDataRecv ();
        }
    }
}

/** Received a packet upon CLOSING */
void
TcpSocketBase::ProcessClosing (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == TcpHeader::ACK)
    {
      if (tcpHeader.GetSequenceNumber () == m_rxBuffer.NextRxSequence ())
        { // This ACK corresponds to the FIN sent
          TimeWait ();
        }
    }
  else
    { // CLOSING state means simultaneous close, i.e. no one is sending data to
      // anyone. If anything other than ACK is received, respond with a reset.
      if (tcpflags == TcpHeader::FIN || tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
        { // FIN from the peer as well. We can close immediately.
          SendEmptyPacket (TcpHeader::ACK);
        }
      else if (tcpflags != TcpHeader::RST)
        { // Receive of SYN or SYN+ACK or bad flags or pure data
          NS_LOG_LOGIC ("Illegal flag " << tcpflags << " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
    }
}

/** Received a packet upon LAST_ACK */
void
TcpSocketBase::ProcessLastAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == 0)
    {
      ReceivedData (packet, tcpHeader);
    }
  else if (tcpflags == TcpHeader::ACK)
    {
      if (tcpHeader.GetSequenceNumber () == m_rxBuffer.NextRxSequence ())
        { // This ACK corresponds to the FIN sent. This socket closed peacefully.
          CloseAndNotify ();
        }
    }
  else if (tcpflags == TcpHeader::FIN)
    { // Received FIN again, the peer probably lost the FIN+ACK
      SendEmptyPacket (TcpHeader::FIN | TcpHeader::ACK);
    }
  else if (tcpflags == (TcpHeader::FIN | TcpHeader::ACK) || tcpflags == TcpHeader::RST)
    {
      CloseAndNotify ();
    }
  else
    { // Received a SYN or SYN+ACK or bad flags
      NS_LOG_LOGIC ("Illegal flag " << tcpflags << " received. Reset packet is sent.");
      SendRST ();
      CloseAndNotify ();
    }
}

/** Peer sent me a FIN. Remember its sequence in rx buffer. */
void
TcpSocketBase::PeerClose (Ptr<Packet> p, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Ignore all out of range packets
  if (tcpHeader.GetSequenceNumber () < m_rxBuffer.NextRxSequence ()
      || tcpHeader.GetSequenceNumber () > m_rxBuffer.MaxRxSequence ())
    {
      return;
    }
  // For any case, remember the FIN position in rx buffer first
  m_rxBuffer.SetFinSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (p->GetSize ()));
  NS_LOG_LOGIC ("Accepted FIN at seq " << tcpHeader.GetSequenceNumber () + SequenceNumber32 (p->GetSize ()));
  // If there is any piggybacked data, process it
  if (p->GetSize ())
    {
      ReceivedData (p, tcpHeader);
    }
  // Return if FIN is out of sequence, otherwise move to CLOSE_WAIT state by DoPeerClose
  if (!m_rxBuffer.Finished ())
    {
      return;
    }

  // Simultaneous close: Application invoked Close() when we are processing this FIN packet
  if (m_state == FIN_WAIT_1)
    {
      NS_LOG_INFO ("FIN_WAIT_1 -> CLOSING");
      m_state = CLOSING;
      return;
    }

  DoPeerClose (); // Change state, respond with ACK
}

/** Received a in-sequence FIN. Close down this socket. */
void
TcpSocketBase::DoPeerClose (void)
{
  NS_ASSERT (m_state == ESTABLISHED || m_state == SYN_RCVD);

  // Move the state to CLOSE_WAIT
  NS_LOG_INFO (TcpStateName[m_state] << " -> CLOSE_WAIT");
  m_state = CLOSE_WAIT;

  if (!m_closeNotified)
    {
      // The normal behaviour for an application is that, when the peer sent a in-sequence
      // FIN, the app should prepare to close. The app has two choices at this point: either
      // respond with ShutdownSend() call to declare that it has nothing more to send and
      // the socket can be closed immediately; or remember the peer's close request, wait
      // until all its existing data are pushed into the TCP socket, then call Close()
      // explicitly.
      NS_LOG_LOGIC ("TCP " << this << " calling NotifyNormalClose shutdownsend " << m_shutdownSend);
      m_socket->NotifyNormalClose ();
      m_closeNotified = true;
    }
  if (m_shutdownSend)
    { // The application declares that it would not sent any more, close this socket
	  NS_LOG_LOGIC (this << " shutdownsend Close");
	  Close ();
    }
  else
    { // Need to ack, the application will close later
	  NS_LOG_LOGIC (this << " ACK");
      SendEmptyPacket (TcpHeader::ACK);
    }
  if (m_state == LAST_ACK)
    {
      NS_LOG_LOGIC ("TcpSocketBase " << this << " scheduling LATO1");
      m_lastAckEvent = Simulator::Schedule (m_rtt->RetransmitTimeout (),
                                            &TcpSocketBase::LastAckTimeout, this);
    }
}
/** Kill this socket. This is a callback function configured to m_endpoint in
   SetupCallback(), invoked when the endpoint is destroyed. */
void
TcpSocketBase::Destroy (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint = 0;
  if (m_tcp != 0)
    {
      std::vector<Ptr<TcpSocketBase> >::iterator it
        = std::find (m_tcp->m_sockets.begin (), m_tcp->m_sockets.end (), this);
      if (it != m_tcp->m_sockets.end ())
        {
          m_tcp->m_sockets.erase (it);
        }
    }
  NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
  CancelAllTimers ();
}

/** Kill this socket. This is a callback function configured to m_endpoint in
   SetupCallback(), invoked when the endpoint is destroyed. */
void
TcpSocketBase::Destroy6 (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint6 = 0;
  if (m_tcp != 0)
    {
      std::vector<Ptr<TcpSocketBase> >::iterator it
        = std::find (m_tcp->m_sockets.begin (), m_tcp->m_sockets.end (), this);
      if (it != m_tcp->m_sockets.end ())
        {
          m_tcp->m_sockets.erase (it);
        }
    }
  NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
  CancelAllTimers ();
}

/** Send an empty packet with specified TCP flags */
void
TcpSocketBase::SendEmptyPacket (uint8_t flags)
{
  NS_LOG_FUNCTION (this << (uint32_t)flags << m_rxBuffer.NextRxSequence ());
  Ptr<Packet> p = Create<Packet> ();
  TcpHeader header;
  SequenceNumber32 s = m_nextTxSequence;

  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_WARN ("Failed to send empty packet due to null endpoint");
      return;
    }
  if (flags & TcpHeader::FIN)
    {
      flags |= TcpHeader::ACK;
    }
  else if (m_state == FIN_WAIT_1 || m_state == LAST_ACK || m_state == CLOSING)
    {
      ++s;
    }

  header.SetFlags (flags);
  header.SetSequenceNumber (s);
  header.SetAckNumber (m_rxBuffer.NextRxSequence ());
  if (m_endPoint != 0)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  if(flags & TcpHeader::SYN)
	  header.SetWindowSize (std::min (m_rxBuffer.MaxBufferSize () - m_rxBuffer.Size (), (uint32_t)m_maxWinSize));
  else {
	  header.SetWindowSize (AdvertisedWindowSize () >> m_rcvscale);
	  NS_LOG_LOGIC(Simulator::Now().GetSeconds() << " Advertised window " << AdvertisedWindowSize () << " " <<(AdvertisedWindowSize () >> m_rcvscale));
  }
  AddOptions (header,0);
  m_rto = m_rtt->RetransmitTimeout ();
  bool hasSyn = flags & TcpHeader::SYN;
  bool hasFin = flags & TcpHeader::FIN;
  bool isAck = flags == TcpHeader::ACK;
  if (hasSyn)
    {
      if (m_cnCount == 0)
        { // No more connection retries, give up
          NS_LOG_LOGIC ("Connection failed.");
          CloseAndNotify ();
          return;
        }
      else
        { // Exponential backoff of connection time out
          int backoffCount = 0x1 << (m_cnRetries - m_cnCount);
          m_rto = m_cnTimeout * backoffCount;
          m_cnCount--;
        }
    }
  if (m_endPoint != 0)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }
  if (flags & TcpHeader::ACK)
    { // If sending an ACK, cancel the delay ACK as well
      m_delAckEvent.Cancel ();
      m_delAckCount = 0;
    }
  if (m_retxEvent.IsExpired () && (hasSyn || hasFin) && !isAck )
    { // Retransmit SYN / SYN+ACK / FIN / FIN+ACK to guard against lost
      NS_LOG_LOGIC ("Schedule retransmission timeout at time "
                    << Simulator::Now ().GetSeconds () << " to expire at time "
                    << (Simulator::Now () + m_rto.Get ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, &TcpSocketBase::SendEmptyPacket, this, flags);
    }
}

/** This function closes the endpoint completely. Called upon RST_TX action. */
void
TcpSocketBase::SendRST (void)
{
  NS_LOG_FUNCTION (this);
  SendEmptyPacket (TcpHeader::RST);
  m_socket->NotifyErrorClose ();
  DeallocateEndPoint ();
}

/** Deallocate the end point and cancel all the timers */
void
TcpSocketBase::DeallocateEndPoint (void)
{
  if (m_endPoint != 0)
    {
      m_endPoint->SetDestroyCallback (MakeNullCallback<void> ());
      m_tcp->DeAllocate (m_endPoint);
      m_endPoint = 0;
      std::vector<Ptr<TcpSocketBase> >::iterator it
        = std::find (m_tcp->m_sockets.begin (), m_tcp->m_sockets.end (), this);
      if (it != m_tcp->m_sockets.end ())
        {
          m_tcp->m_sockets.erase (it);
        }
      CancelAllTimers ();
    }
  if (m_endPoint6 != 0)
    {
      m_endPoint6->SetDestroyCallback (MakeNullCallback<void> ());
      m_tcp->DeAllocate (m_endPoint6);
      m_endPoint6 = 0;
      std::vector<Ptr<TcpSocketBase> >::iterator it
        = std::find (m_tcp->m_sockets.begin (), m_tcp->m_sockets.end (), this);
      if (it != m_tcp->m_sockets.end ())
        {
          m_tcp->m_sockets.erase (it);
        }
      CancelAllTimers ();
    }
}

/** Configure the endpoint to a local address. Called by Connect() if Bind() didn't specify one. */
int
TcpSocketBase::SetupEndpoint ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
  NS_ASSERT (ipv4 != 0);
  if (ipv4->GetRoutingProtocol () == 0)
    {
      NS_FATAL_ERROR ("No Ipv4RoutingProtocol in the node");
    }
  // Create a dummy packet, then ask the routing function for the best output
  // interface's address
  Ipv4Header header;
  header.SetDestination (m_endPoint->GetPeerAddress ());
  Socket::SocketErrno errno_;
  Ptr<Ipv4Route> route;
  Ptr<NetDevice> oif = m_boundnetdevice;
  route = ipv4->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
  if (route == 0)
    {
      NS_LOG_LOGIC ("Route to " << m_endPoint->GetPeerAddress () << " does not exist");
      NS_LOG_ERROR (errno_);
      //m_socket->SetError(errno_);
      m_errno = errno_;
      return -1;
    }
  NS_LOG_LOGIC ("Route exists");
  m_endPoint->SetLocalAddress (route->GetSource ());
  return 0;
}

enum Socket::SocketErrno
TcpSocketBase::GetErrno (void) const
{
  return m_errno;
}

Ptr<Node>
TcpSocketBase::GetNode (void) const
{
  return m_node;
}

int
TcpSocketBase::SetupEndpoint6 ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol> ();
  NS_ASSERT (ipv6 != 0);
  if (ipv6->GetRoutingProtocol () == 0)
    {
      NS_FATAL_ERROR ("No Ipv6RoutingProtocol in the node");
    }
  // Create a dummy packet, then ask the routing function for the best output
  // interface's address
  Ipv6Header header;
  header.SetDestinationAddress (m_endPoint6->GetPeerAddress ());
  Socket::SocketErrno errno_;
  Ptr<Ipv6Route> route;
  Ptr<NetDevice> oif = m_boundnetdevice;
  route = ipv6->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
  if (route == 0)
    {
      NS_LOG_LOGIC ("Route to " << m_endPoint6->GetPeerAddress () << " does not exist");
      NS_LOG_ERROR (errno_);
      m_errno = errno_;
      return -1;
    }
  NS_LOG_LOGIC ("Route exists");
  m_endPoint6->SetLocalAddress (route->GetSource ());
  return 0;
}

/** Inherit from Socket class: Signal a termination of send */
void
TcpSocketBase::SetShutdownSend (bool shutdown)
{
  NS_LOG_FUNCTION (this);
  m_shutdownSend = shutdown;
}

/** Inherit from Socket class: Signal a termination of receive */
void
TcpSocketBase::SetShutdownRecv (bool shutdown)
{
  NS_LOG_FUNCTION (this);
  m_shutdownRecv = shutdown;
}
/** This function is called only if a SYN received in LISTEN state. After
   TcpSocketBase cloned, allocate a new end point to handle the incoming
   connection and send a SYN+ACK to complete the handshake. */
void
TcpSocketBase::CompleteFork (Ptr<Packet> p, const TcpHeader& h,
                             const Address& fromAddress, const Address& toAddress)
{
  // Get port and address from peer (connecting host)
  if (InetSocketAddress::IsMatchingType (toAddress))
    {
      m_endPoint = m_tcp->Allocate (InetSocketAddress::ConvertFrom (toAddress).GetIpv4 (),
                                    InetSocketAddress::ConvertFrom (toAddress).GetPort (),
                                    InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                    InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
      m_endPoint6 = 0;
    }
  else if (Inet6SocketAddress::IsMatchingType (toAddress))
    {
      m_endPoint6 = m_tcp->Allocate6 (Inet6SocketAddress::ConvertFrom (toAddress).GetIpv6 (),
                                      Inet6SocketAddress::ConvertFrom (toAddress).GetPort (),
                                      Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                      Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
      m_endPoint = 0;
    }
  m_tcp->m_sockets.push_back (this);

  // Change the cloned socket from LISTEN state to SYN_RCVD
  NS_LOG_INFO ("LISTEN -> SYN_RCVD");
  m_state = SYN_RCVD;
  m_cnCount = m_cnRetries;
  SetupCallback ();
  // Set the sequence number and send SYN+ACK
  m_rxBuffer.SetNextRxSequence (h.GetSequenceNumber () + SequenceNumber32 (1));
  SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
}

Ipv4EndPoint*
TcpSocketBase::GetEndPoint()
{
	NS_LOG_FUNCTION(this);
	return m_endPoint;
}

void
TcpSocketBase::ConnectionSucceeded ()
{
  NS_LOG_FUNCTION (this);
  // Wrapper to protected function NotifyConnectionSucceeded() so that it can
  // be called as a scheduled event
  m_succeed = true;
  if(!m_socket->GetSucceed())m_socket->NotifyConnectionSucceeded ();
  if(m_socket->GetTcpMpCapable())m_socket->NotifySubflowConnectionSucceeded(this) ;
  // The if-block below was moved from ProcessSynSent() to here because we need
  // to invoke the NotifySend() only after NotifyConnectionSucceeded() to
  // reflect the behaviour in the real world.
  if (GetTxAvailable () > 0)
    {
	  m_socket->NotifySend (GetTxAvailable ());
    }
}

void
TcpSocketBase::SetSucceed()
{
	NS_LOG_FUNCTION(this);
	m_succeed = true;
}


/*int
TcpSocketBase::Send (Ptr<Packet> p, SequenceNumber64 seq, uint32_t flags)
{

  NS_LOG_FUNCTION (this << m_txBuffer.TailSequence() << seq << p->GetSize());
  NS_ABORT_MSG_IF (flags, "use of flags is not supported in TcpSocket::Send()");
  if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
	{
	  NS_LOG_LOGIC ("next sequence " << m_nextTxSequence << " tail sequence " << m_txBuffer.TailSequence() << " size " << m_txBuffer.TailSequence() - m_nextTxSequence );

	  SequenceNumber32 tailSeq = m_txBuffer.TailSequence();
	  // Store the packet into Tx buffer
	  if (!m_txBuffer.Add (p))
		{ // TxBuffer overflow, send failed
	      //m_socket->SetError(Socket::ERROR_MSGSIZE);
		  NS_LOG_LOGIC("Buffer full");
		  m_errno = Socket::ERROR_MSGSIZE;
		  return 0;
		}
	  m_buffsize = m_txBuffer.Size();
	  NS_LOG_LOGIC("TailSeq " << m_txBuffer.TailSequence());
	  NS_LOG_LOGIC ("next sequence " << m_nextTxSequence << " tail sequence " << m_txBuffer.TailSequence() << " size " << m_txBuffer.TailSequence() - m_nextTxSequence );

	  m_seqTxMap.insert(SeqPair(tailSeq,seq));
	  m_sizeTxMap.insert(SizePair(seq,p->GetSize()));

	  NS_LOG_LOGIC (this << " txBufSize= " << m_txBuffer.Size () << " state " << TcpStateName[m_state]);
	  if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
		{ // Try to send the data out
		  SendPendingData (m_connected);
		}
	  return p->GetSize ();
	}
  else
	{ // Connection not established yet
	  //m_socket->SetError(Socket::ERROR_NOTCONN);
	  m_errno = Socket::ERROR_NOTCONN;
	  return -1; // Send failure
	}
  return -1;
}*/

int
TcpSocketBase::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p << p->GetSize());
  NS_ABORT_MSG_IF (flags, "use of flags is not supported in TcpSocket::Send()");
  if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
	{
	  NS_LOG_LOGIC("Established syn_sent or close_wait");

	  // Store the packet into Tx buffer
	  if (!m_txBuffer.Add (p))
		{ // TxBuffer overflow, send failed
	      //m_socket->SetError(Socket::ERROR_MSGSIZE);
		  m_errno = Socket::ERROR_MSGSIZE;
		  return -1;
		}
	  // Submit the data to lower layers
	  NS_LOG_LOGIC ("txBufSize=" << m_txBuffer.Size () << " state " << TcpStateName[m_state]);

	  if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
		{ // Try to send the data out
		  SendPendingData (m_connected);
		}
	  return p->GetSize ();
	}
  else
	{ // Connection not established yet
	  //m_socket->SetError(Socket::ERROR_NOTCONN);
	  NS_LOG_LOGIC("Error");
	  m_errno = Socket::ERROR_NOTCONN;
	  return -1; // Send failure
	}
  return -1;
}
/** Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
    TCP header, and send to TcpL4Protocol */
uint32_t
TcpSocketBase::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{

  //CalculateFragDss(seq-m_frag2,maxSize);
  //NS_LOG_FUNCTION (this << seq << maxSize << withAck << m_numfrags << m_cumsize2 << m_cumfrag2 << m_txBuffer.TailSequence());
  Ptr<Packet> p;
  //if(m_numfrags<4) p = m_txBuffer.CopyFromSequence (maxSize, seq);
  //else p = m_txBuffer.CopyFromSequence (maxSize-m_cumfrag2, seq);
  p = m_txBuffer.CopyFromSequence (CalculateFragDss(seq), seq);
  NS_LOG_FUNCTION (this << seq << p->GetSize() << m_txBuffer.TailSequence());

  m_numfrags = 1;
  uint32_t sz = p->GetSize (); // Size of packet

  NS_LOG_FUNCTION (this << sz);

  uint8_t flags = withAck ? TcpHeader::ACK : 0;
  uint32_t remainingData = m_txBuffer.SizeFromSequence (seq + SequenceNumber32 (sz));


  if (m_closeOnEmpty && (remainingData == 0))
    {
      flags |= TcpHeader::FIN;
      if (m_state == ESTABLISHED)
        { // On active close: I am the first one to send FIN
          NS_LOG_INFO ("ESTABLISHED -> FIN_WAIT_1");
          m_state = FIN_WAIT_1;
        }
      else if (m_state == CLOSE_WAIT)
        { // On passive close: Peer sent me FIN already
          NS_LOG_INFO ("CLOSE_WAIT -> LAST_ACK");
          m_state = LAST_ACK;
        }
    }
  TcpHeader header;
  header.SetFlags (flags);
  header.SetSequenceNumber (seq);
  header.SetAckNumber (m_rxBuffer.NextRxSequence ());
  if (m_endPoint)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  if(flags & TcpHeader::SYN){
	  header.SetWindowSize (std::min (m_rxBuffer.MaxBufferSize () - m_rxBuffer.Size (), (uint32_t)m_maxWinSize));
  }else {
	  header.SetWindowSize (AdvertisedWindowSize () >> m_rcvscale);
  }
  AddOptions (header,sz);
  if (m_retxEvent.IsExpired () )
    { // Schedule retransmit
      m_rto = m_rtt->RetransmitTimeout ();
      NS_LOG_LOGIC (this << " SendDataPacket Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto.Get ()).GetSeconds () );
      m_retxEvent = Simulator::Schedule (m_rto, &TcpSocketBase::ReTxTimeout, this);
    }
  NS_LOG_LOGIC ("Send packet via TcpL4Protocol with flags 0x" << std::hex << static_cast<uint32_t> (flags) << std::dec);
  if (m_endPoint)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }
  m_rtt->SentSeq (seq, sz);       // notify the RTT
  // Notify the application of the data being sent unless this is a retransmit
  if (seq == m_nextTxSequence)
    {
      Simulator::ScheduleNow (&TcpSocket::NotifyDataSent, m_socket, sz);
    }
  // Update highTxMark
  m_highTxMark = std::max (seq + sz, m_highTxMark.Get ());
  return sz;
}

/** Send as much pending data as possible according to the Tx window. Note that
 *  this function did not implement the PSH flag
 */
bool
TcpSocketBase::SendPendingData (bool withAck)
{
  NS_LOG_FUNCTION (this << withAck << m_nextTxSequence);
  if (m_txBuffer.Size () == 0)
    {
      return false;                           // Nothing to send

    }
  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_INFO ("TcpSocketBase::SendPendingData: No endpoint; m_shutdownSend=" << m_shutdownSend);
      return false; // Is this the right way to handle this condition?
    }
  uint32_t nPacketsSent = 0;
  while (m_txBuffer.SizeFromSequence (m_nextTxSequence))
    {
      uint32_t w = AvailableWindow (); // Get available window size
      NS_LOG_LOGIC ("TcpSocketBase " << this << " SendPendingData" <<
                    " w " << w <<
                    " rxwin " << m_rWnd <<
                    " segsize " << m_segmentSize <<
                    " nextTxSeq " << m_nextTxSequence <<
                    " highestRxAck " << m_txBuffer.HeadSequence () <<
                    " pd->Size " << m_txBuffer.Size () <<
                    " pd->SFS " << m_txBuffer.SizeFromSequence (m_nextTxSequence));
      // Quit if send disallowed
      if (m_shutdownSend)
        {
          //m_socket->SetError(Socket::ERROR_SHUTDOWN);
    	  m_errno = Socket::ERROR_SHUTDOWN;
          return false;
        }
      // Stop sending if we need to wait for a larger Tx window (prevent silly window syndrome)
      if (w < m_segmentSize && m_txBuffer.SizeFromSequence (m_nextTxSequence) > w)
        {
          break; // No more
        }
      // Nagle's algorithm (RFC896): Hold off sending if there is unacked data
      // in the buffer and the amount of data to send is less than one segment
      if (!m_noDelay && UnAckDataCount () > 0
          && m_txBuffer.SizeFromSequence (m_nextTxSequence) < m_segmentSize)
        {
          NS_LOG_LOGIC ("Invoking Nagle's algorithm. Wait to send.");
          break;
        }
      uint32_t s = std::min (w, m_segmentSize);  // Send no more than window
      uint32_t sz = SendDataPacket (m_nextTxSequence, s, withAck);
      nPacketsSent++;                             // Count sent this loop
      m_nextTxSequence += sz;                     // Advance next tx sequence
    }
  NS_LOG_LOGIC ("SendPendingData sent " << nPacketsSent << " packets");
  return (nPacketsSent > 0);
}

uint32_t
TcpSocketBase::UnAckDataCount ()
{
  //NS_LOG_FUNCTION (this);
  return m_nextTxSequence.Get () - m_txBuffer.HeadSequence ();
}

uint32_t
TcpSocketBase::BytesInFlight ()
{
  //NS_LOG_FUNCTION (this);
  return m_highTxMark.Get () - m_txBuffer.HeadSequence ();
}

uint32_t
TcpSocketBase::UnSentData ()
{
  NS_LOG_FUNCTION (this<<m_txBuffer.TailSequence ()<<m_nextTxSequence.Get()<<m_txBuffer.HeadSequence ()<<m_highTxMark.Get ());
  //return m_txBuffer.TailSequence () - m_txBuffer.HeadSequence ();
  return m_txBuffer.TailSequence () - m_nextTxSequence.Get();
}

Time
TcpSocketBase::GetLastRtt ()
{
	return m_lastRtt;
}

uint32_t
TcpSocketBase::BufferedData ()
{
  NS_LOG_FUNCTION (this<<m_txBuffer.TailSequence ()<<m_nextTxSequence.Get()<<m_txBuffer.HeadSequence ()<<m_highTxMark.Get ());
  return m_txBuffer.TailSequence () - m_txBuffer.HeadSequence ();
}

uint32_t
TcpSocketBase::Window ()
{
  NS_LOG_FUNCTION (this);
  return m_rWnd;
}


uint32_t
TcpSocketBase::AvailableWindow ()
{
  //NS_LOG_FUNCTION_NOARGS ();
  uint32_t unack = UnAckDataCount (); // Number of outstanding bytes
  uint32_t win = Window (); // Number of bytes allowed to be outstanding
  //NS_LOG_LOGIC ("UnAckCount=" << unack << ", Win=" << win);
  return (win < unack) ? 0 : (win - unack);
}

uint32_t
TcpSocketBase::AdvertisedWindowSize ()
{

  if(m_socket->GetTcpMpCapable())
  {
	  uint32_t val = std::min (m_socket->GetObject<MpTcpSocket>()->AdvertisedWindowSize(), (uint32_t)(m_maxWinSize<<m_rcvscale));
	  NS_LOG_FUNCTION(this << (uint32_t)(m_maxWinSize<<m_rcvscale) << val);
	  return std::min (m_socket->GetObject<MpTcpSocket>()->AdvertisedWindowSize(), (uint32_t)(m_maxWinSize<<m_rcvscale));
  } else {
	  NS_LOG_FUNCTION(this << m_rxBuffer.MaxBufferSize () << m_rxBuffer.Size () << (uint32_t)(m_maxWinSize<<m_rcvscale));
	return std::min (m_rxBuffer.MaxBufferSize () - m_rxBuffer.Size (), (uint32_t)(m_maxWinSize<<m_rcvscale));
  }
}

// Receipt of new packet, put into Rx buffer
void
TcpSocketBase::ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);
  NS_LOG_LOGIC ("seq " << tcpHeader.GetSequenceNumber () <<
                " ack " << tcpHeader.GetAckNumber () <<
                " pkt size " << p->GetSize () );


  SequenceNumber32 expectedSeq = m_rxBuffer.NextRxSequence ();
  //NS_LOG_LOGIC("ExpectedSeq " << expectedSeq);
  if (!m_rxBuffer.Add (p, tcpHeader))
    { // Insert failed: No data or RX buffer full
      SendEmptyPacket (TcpHeader::ACK);
      return;
    }

  // Notify app to receive if necessary
  if (expectedSeq < m_rxBuffer.NextRxSequence ())
    { // NextRxSeq advanced, we have something to send to the app
      if (!m_shutdownRecv)
        {
          if(!m_socket->GetTcpMpCapable())
		  {
	          m_socket->NotifyDataReceived ();
		  }
          //if(m_socket->GetTcpMpCapable())
          else {
        	 // m_socket->GetObject<MpTcpSocket>()->ReceivedData(this,m_dataSeq,m_rxBuffer.Extract (p->GetSize()));

          	  Ptr<Packet> packet;
        	  Ptr<Packet> p;
        	  uint32_t first = 0;
        	  uint32_t fragment = 0;
        	  SequenceNumber64 seq = SequenceNumber64(0);
        	  if(packet = m_rxBuffer.Extract(tcpHeader.GetSequenceNumber ())){
        		  for( std::map<SequenceNumber64,uint32_t>::iterator i=m_sizeRxMap.begin(); i!=m_sizeRxMap.end(); ++i)
        		    {
        			    NS_LOG_LOGIC("SizeRxMap " << (*i).first << ": " << (*i).second << " " << first << " " << m_lastSeq << " "<< m_size);
        		        p = packet->CreateFragment(fragment,(*i).second);
        		        if(seq.GetValue()==0){
        		        	if(m_lastSeq==(*i).first){
        			        	m_socket->GetObject<MpTcpSocket>()->ReceivedData(this,(*i).first+first+m_size,p);
        		        	}else{
        		        		m_socket->GetObject<MpTcpSocket>()->ReceivedData(this,(*i).first,p);
        		        	}
        		        } else {
        					m_socket->GetObject<MpTcpSocket>()->ReceivedData(this,(*i).first,p);
        		        }
        				seq = (*i).first;
        		        first = (*i).second;
        		        fragment += (*i).second;
        				m_sizeRxMap.erase(i);
        		    }
        		  m_lastSeq = seq;
        		  m_size = first;

        	  }
          }

        }
      // Handle exceptions
      if (m_closeNotified)
        {
          NS_LOG_WARN ("Why TCP " << this << " got data after close notification?");
        }
      // If we received FIN before and now completed all "holes" in rx buffer,
      // invoke peer close procedure
      if (m_rxBuffer.Finished () && (tcpHeader.GetFlags () & TcpHeader::FIN) == 0)
        {
          DoPeerClose ();
        }
    }

  // Now send a new ACK packet acknowledging all received and delivered data
  if (m_rxBuffer.Size () > m_rxBuffer.Available () || m_rxBuffer.NextRxSequence () > expectedSeq + p->GetSize ())
    { // A gap exists in the buffer, or we filled a gap: Always ACK
      SendEmptyPacket (TcpHeader::ACK);
    }
  else
    { // In-sequence packet: ACK if delayed ack count allows
      if (++m_delAckCount >= m_delAckMaxCount)
        {
          m_delAckEvent.Cancel ();
          m_delAckCount = 0;
          SendEmptyPacket (TcpHeader::ACK);
        }
      else if (m_delAckEvent.IsExpired ())
        {
          m_delAckEvent = Simulator::Schedule (m_delAckTimeout,
                                               &TcpSocketBase::DelAckTimeout, this);
          NS_LOG_LOGIC (this << " scheduled delayed ACK at " << (Simulator::Now () + Simulator::GetDelayLeft (m_delAckEvent)).GetSeconds ());
        }
    }
}

/** Called by ForwardUp() to estimate RTT */
void
TcpSocketBase::EstimateRtt (const TcpHeader& tcpHeader)
{
	  Time nextRtt =  m_rtt->AckSeq (tcpHeader.GetAckNumber () );

	  //nextRtt will be zero for dup acks.  Don't want to update lastRtt in that case
	  //but still needed to do list clearing that is done in AckSeq.
	  if(nextRtt != 0)
	  {
	    m_lastRtt = nextRtt;
	    NS_LOG_FUNCTION(this << m_lastRtt);
	  }

}

// Called by the ReceivedAck() when new ACK received and by ProcessSynRcvd()
// when the three-way handshake completed. This cancels retransmission timer
// and advances Tx window
void
TcpSocketBase::NewAck (SequenceNumber32 const& ack)
{
  NS_LOG_FUNCTION (this << ack);

  if (m_state != SYN_RCVD)
    { // Set RTO unless the ACK is received in SYN_RCVD state
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      // On recieving a "New" ack we restart retransmission timer .. RFC 2988
      m_rto = m_rtt->RetransmitTimeout ();
      NS_LOG_LOGIC (this << " Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto.Get ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, &TcpSocketBase::ReTxTimeout, this);
    }
  if (m_rWnd.Get () == 0 && m_persistEvent.IsExpired ())
    { // Zero window: Enter persist state to send 1 byte to probe
      NS_LOG_LOGIC (this << "Enter zerowindow persist state");
      NS_LOG_LOGIC (this << "Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      NS_LOG_LOGIC ("Schedule persist timeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_persistTimeout).GetSeconds ());
      m_persistEvent = Simulator::Schedule (m_persistTimeout, &TcpSocketBase::PersistTimeout, this);
      NS_ASSERT (m_persistTimeout == Simulator::GetDelayLeft (m_persistEvent));
    }
  // Note the highest ACK and tell app to send more
  NS_LOG_LOGIC ("TCP " << this << " NewAck " << ack <<
                " numberAck " << (ack - m_txBuffer.HeadSequence ()) <<
                " size " << m_txBuffer.Size()); // Number bytes ack'ed
  m_txBuffer.DiscardUpTo (ack);
  m_buffsize = m_txBuffer.Size();

  if(m_socket->GetTcpMpCapable())
  {
	  m_socket->GetObject<MpTcpSocket>()->ReceivedAck(this,m_dataAck);
	 /* for(std::map<SequenceNumber32,SequenceNumber64>::iterator it = seqTxMap.find(ack); it != seqTxMap.begin(); --it)
	  	{
		  NS_LOG_LOGIC("Seq acked " << it->first << " "<< it->second);
		  seqTxMap.erase(it);
	  	}*/

  }
  if (GetTxAvailable () > 0)
    {
	  NS_LOG_LOGIC ("Send available");
	  m_socket->NotifySend (GetTxAvailable ());
    } else {
  	  NS_LOG_LOGIC ("Send unavailable");
    }
  if (ack > m_nextTxSequence)
    {
      m_nextTxSequence = ack; // If advanced
    }
  if (m_txBuffer.Size () == 0 && m_state != FIN_WAIT_1 && m_state != CLOSING)
    { // No retransmit timer if no data to retransmit
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
    }
  // Try to send more data
  SendPendingData (m_connected);
}

// Retransmit timeout
void
TcpSocketBase::ReTxTimeout ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT)
    {
      return;
    }
  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer.HeadSequence () >= m_highTxMark)
    {
      return;
    }

  Retransmit ();
}

void
TcpSocketBase::DelAckTimeout (void)
{
  m_delAckCount = 0;
  SendEmptyPacket (TcpHeader::ACK);
}

void
TcpSocketBase::LastAckTimeout (void)
{
  NS_LOG_FUNCTION (this);

  m_lastAckEvent.Cancel ();
  if (m_state == LAST_ACK)
    {
      CloseAndNotify ();
    }
  if (!m_closeNotified)
    {
      m_closeNotified = true;
    }
}

// Send 1-byte data to probe for the window size at the receiver when
// the local knowledge tells that the receiver has zero window size
// C.f.: RFC793 p.42, RFC1112 sec.4.2.2.17
void
TcpSocketBase::PersistTimeout ()
{
  NS_LOG_LOGIC ("PersistTimeout expired at " << Simulator::Now ().GetSeconds ());
  m_persistTimeout = std::min (Seconds (60), Time (2 * m_persistTimeout)); // max persist timeout = 60s
  Ptr<Packet> p = m_txBuffer.CopyFromSequence (1, m_nextTxSequence);
  TcpHeader tcpHeader;
  tcpHeader.SetSequenceNumber (m_nextTxSequence);
  tcpHeader.SetAckNumber (m_rxBuffer.NextRxSequence ());
  tcpHeader.SetWindowSize (AdvertisedWindowSize ());
  if (m_endPoint != 0)
    {
      tcpHeader.SetSourcePort (m_endPoint->GetLocalPort ());
      tcpHeader.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      tcpHeader.SetSourcePort (m_endPoint6->GetLocalPort ());
      tcpHeader.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  AddOptions (tcpHeader,p->GetSize());

  if (m_endPoint != 0)
    {
      m_tcp->SendPacket (p, tcpHeader, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
  else
    {
      m_tcp->SendPacket (p, tcpHeader, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }
  NS_LOG_LOGIC ("Schedule persist timeout at time "
                << Simulator::Now ().GetSeconds () << " to expire at time "
                << (Simulator::Now () + m_persistTimeout).GetSeconds ());
  m_persistEvent = Simulator::Schedule (m_persistTimeout, &TcpSocketBase::PersistTimeout, this);
}

void
TcpSocketBase::Retransmit ()
{
  m_nextTxSequence = m_txBuffer.HeadSequence (); // Start from highest Ack
  m_rtt->IncreaseMultiplier (); // Double the timeout value for next retx timer
  m_dupAckCount = 0;
  DoRetransmit (); // Retransmit the packet
}

void
TcpSocketBase::DoRetransmit ()
{
  NS_LOG_FUNCTION (this);
  // Retransmit SYN packet
  if (m_state == SYN_SENT)
    {
      if (m_cnCount > 0)
        {
          SendEmptyPacket (TcpHeader::SYN);
        }
      else
        {
    	  m_socket->NotifyConnectionFailed ();
        }
      return;
    }
  // Retransmit non-data packet: Only if in FIN_WAIT_1 or CLOSING state
  if (m_txBuffer.Size () == 0)
    {
      if (m_state == FIN_WAIT_1 || m_state == CLOSING)
        { // Must have lost FIN, re-send
          SendEmptyPacket (TcpHeader::FIN);
        }
      return;
    }
  // Retransmit a data packet: Call SendDataPacket
  NS_LOG_LOGIC ("TcpSocketBase " << this << " retxing seq " << m_txBuffer.HeadSequence ());
  uint32_t sz = SendDataPacket (m_txBuffer.HeadSequence (), m_segmentSize, true);
  // In case of RTO, advance m_nextTxSequence
  m_nextTxSequence = std::max (m_nextTxSequence.Get (), m_txBuffer.HeadSequence () + sz);

}

void
TcpSocketBase::CancelAllTimers ()
{
  m_retxEvent.Cancel ();
  m_persistEvent.Cancel ();
  m_delAckEvent.Cancel ();
  m_lastAckEvent.Cancel ();
  m_timewaitEvent.Cancel ();
}

/** Move TCP to Time_Wait state and schedule a transition to Closed state */
void
TcpSocketBase::TimeWait ()
{
  NS_LOG_INFO (TcpStateName[m_state] << " -> TIME_WAIT");
  m_state = TIME_WAIT;
  CancelAllTimers ();
  // Move from TIME_WAIT to CLOSED after 2*MSL. Max segment lifetime is 2 min
  // according to RFC793, p.28
  m_timewaitEvent = Simulator::Schedule (Seconds (2 * m_msl),
                                         &TcpSocketBase::CloseAndNotify, this);
}

/** Below are the attribute get/set functions */

void
TcpSocketBase::SetSndBufSize (uint32_t size)
{
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
TcpSocketBase::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}

void
TcpSocketBase::SetRcvBufSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_rxBuffer.SetMaxBufferSize (size);
}

uint32_t
TcpSocketBase::GetRcvBufSize (void) const
{
  return m_rxBuffer.MaxBufferSize ();
}

uint32_t
TcpSocketBase::GetRxAvailable (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxBuffer.Available ();
}

uint32_t
TcpSocketBase::GetTxAvailable (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txBuffer.Available ();
}

void
TcpSocketBase::SetSegSize (uint32_t size)
{
  m_segmentSize = size;
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "Cannot change segment size dynamically.");
}

int
TcpSocketBase::GetSocketName(Address &address) const
{
	  NS_LOG_FUNCTION (this);
	  if (m_endPoint != 0)
	    {
	      address = InetSocketAddress (m_endPoint->GetLocalAddress (), m_endPoint->GetLocalPort ());
	    }
	  else if (m_endPoint6 != 0)
	    {
	      address = Inet6SocketAddress (m_endPoint6->GetLocalAddress (), m_endPoint6->GetLocalPort ());
	    }
	  else
	    { // It is possible to call this method on a socket without a name
	      // in which case, behavior is unspecified
	      // Should this return an InetSocketAddress or an Inet6SocketAddress?
	      address = InetSocketAddress (Ipv4Address::GetZero (), 0);
	    }
	  return 0;
}
uint32_t
TcpSocketBase::GetSegSize (void) const
{
  return m_segmentSize;
}

void
TcpSocketBase::SetConnTimeout (Time timeout)
{
  m_cnTimeout = timeout;
}

Time
TcpSocketBase::GetConnTimeout (void) const
{
  return m_cnTimeout;
}

void
TcpSocketBase::SetConnCount (uint32_t count)
{
  m_cnRetries = count;
}

uint32_t
TcpSocketBase::GetConnCount (void) const
{
  return m_cnRetries;
}

void
TcpSocketBase::SetDelAckTimeout (Time timeout)
{
  m_delAckTimeout = timeout;
}

Time
TcpSocketBase::GetDelAckTimeout (void) const
{
  return m_delAckTimeout;
}

void
TcpSocketBase::SetDelAckMaxCount (uint32_t count)
{
  m_delAckMaxCount = count;
}

uint32_t
TcpSocketBase::GetDelAckMaxCount (void) const
{
  return m_delAckMaxCount;
}

void
TcpSocketBase::SetTcpNoDelay (bool noDelay)
{
  m_noDelay = noDelay;
}

bool
TcpSocketBase::GetTcpNoDelay (void) const
{
  return m_noDelay;
}

void
TcpSocketBase::SetWinScale (uint32_t winscale)
{
	m_rcvscale = winscale;
}

uint32_t
TcpSocketBase::GetWinScale (void) const
{
  return m_rcvscale;
}


void
TcpSocketBase::SetPersistTimeout (Time timeout)
{
  m_persistTimeout = timeout;
}

Time
TcpSocketBase::GetPersistTimeout (void) const
{
  return m_persistTimeout;
}

bool
TcpSocketBase::SetAllowBroadcast (bool allowBroadcast)
{
  // Broadcast is not implemented. Return true only if allowBroadcast==false
  return (!allowBroadcast);
}

void
TcpSocketBase::SetDataAck (SequenceNumber64 dataAck)
{
  NS_LOG_FUNCTION (this << dataAck);
  m_dataAck = dataAck;
}

TcpStates_t
TcpSocketBase::GetState(void)
{
	return m_state;
}

void
TcpSocketBase::SetState(TcpStates_t state)
{
	m_state = state;
}

void
TcpSocketBase::SetSocket(Ptr<Socket> socket)
{
	NS_LOG_FUNCTION(this<<socket);
	m_socket = socket->GetObject<TcpSocket>();
}

bool
TcpSocketBase::GetAllowBroadcast (void) const
{
  return false;
}

/** Placeholder function for future extension that reads more from the TCP header */
void
TcpSocketBase::ReadOptions (const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this);
  for (int i=0; i<tcpHeader.GetOptionSize(); i++)
	   {
		   Ptr<TcpOption> tcpOption;
		   if(m_socket->GetTcpMp()){

				   tcpOption = tcpHeader.GetOption(TcpHeader::TCPOPTION_MPTCP);
				   if(tcpOption)
				   {
					   Ptr<TcpOptionMptcp> mpOption = tcpOption->GetObject<TcpOptionMptcp>();
					   //NS_LOG_LOGIC("Tcp Option subtype: "<<i<< " " << (uint32_t)mpOption->GetSubtype() << " " << tcpOption << " " << mpOption);
					   if(mpOption->GetSubtype()==MPTCP_SUB_CAPABLE)
					   {
						   Ptr<TcpOptionMpCapable> mcap = mpOption->GetSubOption()->GetObject<TcpOptionMpCapable>();

						   if(tcpHeader.GetFlags() == TcpHeader::SYN){
							   NS_LOG_LOGIC("MPCAPABLE SYN READ");
							   m_socket->SetRemoteKey(mcap->GetSenderKey());

						   }else if(tcpHeader.GetFlags() == (TcpHeader::SYN | TcpHeader::ACK))
						   {
							   NS_LOG_LOGIC("MPCAPABLE SYNACK READ");

							   m_socket->SetRemoteKey(mcap->GetSenderKey());
							   m_socket->SetTcpMpCapable();

						   }
						   else if(tcpHeader.GetFlags() == TcpHeader::ACK)
						   {
							   NS_LOG_LOGIC("MPCAPABLE ACK");

							   if(mcap->GetSenderKey()==m_socket->GetRemoteKey() && mcap->GetReceiverKey()==m_socket->GetLocalKey())
							   {
								   NS_LOG_LOGIC("MPCAPABLE ACK READ");
									 m_socket->SetTcpMpCapable();
							   }

						   }

					   }
					   if(mpOption->GetSubtype()==MPTCP_SUB_DSS)
					   {
						   std::vector<Ptr<TcpOption> > tcpOptions = tcpHeader.GetOptions(TcpHeader::TCPOPTION_MPTCP);
						   Ptr<TcpOptionMptcp> mpOption = tcpOptions[i]->GetObject<TcpOptionMptcp>();
						   Ptr<TcpOptionMpDss> mpDss = mpOption->GetSubOption()->GetObject<TcpOptionMpDss>();
							if(mpDss->GetFlags()==TcpOptionMpDss::SEQACK||mpDss->GetFlags()==TcpOptionMpDss::FINSEQACK||mpDss->GetFlags()==TcpOptionMpDss::ACK){
								m_dataAck =  mpDss->GetDataAcknowledgment64();
							}
							if (mpDss->GetFlags()==TcpOptionMpDss::SEQACK||mpDss->GetFlags()==TcpOptionMpDss::FINSEQACK||mpDss->GetFlags()==TcpOptionMpDss::SEQ){
								m_dataSeq = mpDss->GetDataSequence64();
							}
							if (mpDss->GetFlags()==TcpOptionMpDss::SEQ32){
								NS_LOG_LOGIC("Seq " << tcpHeader.GetSequenceNumber() << " DataSeq32 " << " " << mpDss->GetDataSequence32().GetValue() << " length " << mpDss->GetDataLength());
								m_dataSeq = SequenceNumber64(mpDss->GetDataSequence32().GetValue());
								NS_LOG_LOGIC("SizeRxMap " << m_dataSeq << " " << mpDss->GetDataLength());
								m_sizeRxMap.insert(SizePair(m_dataSeq,mpDss->GetDataLength()));
							}

							//m_dataAck = mpDss->GetDataAcknowledgment64();
						   //mdss->GetSubflowSequence();
							//mpDss->GetDataLength();
							//seqRxMap.insert(SeqPair(mpDss->GetSubflowSequence(),mpDss->GetDataSequence64()));

							NS_LOG_FUNCTION(this << " DSS Recv Ack:" << m_dataAck << " Seq:" << m_dataSeq);

					   }
				   }


			   }
			   tcpOption = tcpHeader.GetOption(TcpHeader::TCPOPTION_WINDOW_SCALE);
						   if(tcpOption){
							   m_sndscale = tcpOption->GetObject<TcpOptionWinScale>()->GetScale();
							 NS_LOG_LOGIC("Received Window scale " << (uint32_t)tcpOption->GetObject<TcpOptionWinScale>()->GetScale());
						   }

		   }


		   /*
		   tcpOption = tcpHeader.GetOption(TcpHeader::TCPOPTION_MIH);
		   if(tcpOption){
			   m_tcpMih = tcpOption->GetObject<TcpOptionMih>();

				if(m_tcpMih->GetDirection()==0){
					m_nextTxSequence = tcpHeader.GetAckNumber();
					NS_LOG_LOGIC("Direction 0");
				}
				else if (m_tcpMih->GetDirection()==1)
					NS_LOG_LOGIC("Direction 1");
			   NS_LOG_LOGIC("Clients " << m_tcpMih->GetClients() << " Throughput " << m_tcpMih->GetThroughput() << " Delay " << m_tcpMih->GetDelay() << " MacAddress " << m_tcpMih->GetMacAddress());
			   m_hand(this,m_tcpMih->GetDirection(),m_tcpMih->GetDelay(),m_tcpMih->GetClients(),m_tcpMih->GetThroughput(),m_tcpMih->GetMacAddress());
			   //Handover(m_tcpMih->GetDelay(),m_tcpMih->GetClients(),m_tcpMih->GetThroughput());
		   }

		   tcpOption = tcpHeader.GetOption(TcpHeader::TCPOPTION_HAND);
		   if(tcpOption){
			   Ptr<TcpOptionHand> m_tcpHand = tcpOption->GetObject<TcpOptionHand>();
			   NS_LOG_LOGIC("Mac " << m_tcpHand->GetMacAddress());
			   m_posthand(this,m_tcpHand->GetMacAddress());
		   }
		   */

}

/** Placeholder function for future extension that changes the TCP header */
void
TcpSocketBase::AddOptions (TcpHeader& tcpHeader,uint32_t size)
{
	NS_LOG_FUNCTION(this);
  //NS_LOG_FUNCTION(this << " DSS mpcapsent " << m_socket->GetMpcapSent());

	if(m_socket->GetTcpMp()){
		  NS_LOG_LOGIC("DSS " << this << " seq " << tcpHeader.GetSequenceNumber () << " ack " << m_socket->GetObject<MpTcpSocket>()->GetRxSequence() << " " << tcpHeader.GetAckNumber ());

		Ptr<TcpOptionMptcp> mpOption = TcpOption::CreateOption(30)->GetObject<TcpOptionMptcp>();

		if((tcpHeader.GetFlags() == TcpHeader::SYN)&&!m_socket->GetMpcapSent())
		{
			NS_LOG_LOGIC("MPCAPSENT SYN " << this << " " << m_socket->GetMpcapSent());

			Ptr<TcpOptionMpCapable> mcap = TcpSubOption::CreateSubOption(0)->GetObject<TcpOptionMpCapable>();
			/*uint64_t R0 = (uint64_t)random() << 48;
			uint64_t R1 = (uint64_t)random() << 48 >> 16;
			uint64_t R2 = (uint64_t)random() << 48 >> 32;
			uint64_t R3 = (uint64_t)random() << 48 >> 48;
			m_socket->SetLocalKey(R0 | R1 | R2 | R3);
			*/
			m_socket->SetLocalKey((uint64_t)0);
			NS_LOG_FUNCTION ("key " << m_socket->GetLocalKey());
			mcap->SetSenderKey(m_socket->GetLocalKey());
			mpOption->AddSubOption(mcap);
			tcpHeader.AppendOption(mpOption);
		//	Ptr<TcpOptionSackPermitted> sackPermitted = tcp->CreateOption(4,0)->GetObject<TcpOptionSackPermitted>();
		//    tcpHeader.AppendOption(sackPermitted);
			//m_tcpSackTx = tcp->CreateOption(5)->GetObject<TcpOptionSack>();
		} else if ((tcpHeader.GetFlags() == (TcpHeader::SYN | TcpHeader::ACK))&&!m_socket->GetMpcapSent())
		{
			Ptr<TcpOptionMpCapable> mcap = TcpSubOption::CreateSubOption(0)->GetObject<TcpOptionMpCapable>();
			/*uint64_t R0 = (uint64_t)random() << 48;
			uint64_t R1 = (uint64_t)random() << 48 >> 16;
			uint64_t R2 = (uint64_t)random() << 48 >> 32;
			uint64_t R3 = (uint64_t)random() << 48 >> 48;
			m_socket->SetLocalKey(R0 | R1 | R2 | R3);*/
			m_socket->SetLocalKey((uint64_t)0);
			NS_LOG_FUNCTION ("key " << m_socket->GetLocalKey());
			mcap->SetSenderKey(m_socket->GetLocalKey());
			mpOption->AddSubOption(mcap);
			tcpHeader.AppendOption(mpOption);
			NS_LOG_LOGIC("MPCAPSENT SYNACK " << this << " " << m_socket->GetMpcapSent());
			m_socket->SetMpcapSent(true);

		} else if((tcpHeader.GetFlags() == TcpHeader::ACK)&&!m_socket->GetMpcapSent())
		{
			Ptr<TcpOptionMpCapable> mcap = TcpSubOption::CreateSubOption(0)->GetObject<TcpOptionMpCapable>();
			NS_LOG_FUNCTION ("key " << m_socket->GetLocalKey());
			mcap->SetSenderKey(m_socket->GetLocalKey());
			mcap->SetReceiverKey(m_socket->GetRemoteKey());
			mpOption->AddSubOption(mcap);
			tcpHeader.AppendOption(mpOption);
			NS_LOG_LOGIC("MPCAPSENT ACK " << this << " " << m_socket->GetMpcapSent());
			m_socket->SetMpcapSent(true);

		//} else if((tcpHeader.GetFlags() != TcpHeader::SYN)&&(m_socket->GetMpcapSent()))
		}
		//if((m_succeed)&&(m_socket->GetTcpMpCapable()))
		//NS_LOG_FUNCTION(this << " DSS capable " << m_socket->GetTcpMpCapable() << " suboption " << !mpOption->GetSubOption() << " succeed " << m_succeed << " SYN " << !(tcpHeader.GetFlags()& TcpHeader::SYN));
		if((m_socket->GetTcpMpCapable())&&(!mpOption->GetSubOption())&&m_succeed&&!(tcpHeader.GetFlags()& TcpHeader::SYN))
		{

			if(tcpHeader.GetAckNumber()<=SequenceNumber32(1)){
				//NS_LOG_FUNCTION(this << " DSS Sent Seq:" << m_socket->GetObject<MpTcpSocket>()->GetRxSequence() << " " << SequenceNumber64(tcpHeader.GetSequenceNumber ().GetValue()));
				CalculateDss(tcpHeader,tcpHeader.GetSequenceNumber()-m_frag, size);
			}
			else
			{
				Ptr<TcpOptionMptcp> mpOption = TcpOption::CreateOption(30)->GetObject<TcpOptionMptcp>();
				Ptr<TcpOptionMpDss> mpDss = TcpSubOption::CreateSubOption(2)->GetObject<TcpOptionMpDss>();
				//NS_LOG_FUNCTION(this << " DSS Sent Ack:" << m_socket->GetObject<MpTcpSocket>()->GetRxSequence() << " " << SequenceNumber64(tcpHeader.GetSequenceNumber ().GetValue()));
				mpDss->SetDataAcknowledgment64(m_socket->GetObject<MpTcpSocket>()->GetRxSequence());
				mpOption->AddSubOption(mpDss);
				tcpHeader.AppendOption(mpOption);
			}/*
			NS_LOG_FUNCTION(this << " DSS Sent");
			std::map<SequenceNumber32,SequenceNumber64>::iterator iter;
			iter=seqTxMap.find(tcpHeader.GetSequenceNumber ());
			Ptr<TcpOptionMpDss> mpDss = TcpSubOption::CreateSubOption(2)->GetObject<TcpOptionMpDss>();
			mpDss->SetDataAcknowledgment64(m_socket->GetObject<MpTcpSocket>()->GetRxSequence());

			if(iter!=seqTxMap.end()){
				mpDss->SetDataSequence64(iter->second);
				mpDss->SetSubflowSequence(tcpHeader.GetSequenceNumber ());
				mpDss->SetDataLength(1400);
				NS_LOG_FUNCTION(this << " DSS Sent Ack1:" << m_socket->GetObject<MpTcpSocket>()->GetRxSequence() << " Seq:" << iter->second);

			} else {
				mpDss->SetDataSequence64(SequenceNumber64(tcpHeader.GetSequenceNumber ().GetValue()));
				mpDss->SetSubflowSequence(tcpHeader.GetSequenceNumber ());
				mpDss->SetDataLength(1400);
				NS_LOG_FUNCTION(this << " DSS Sent Ack2:" << m_socket->GetObject<MpTcpSocket>()->GetRxSequence() << " Seq:" << tcpHeader.GetSequenceNumber ().GetValue());

			}
			//seqTxMap.erase(iter);
			mpOption->AddSubOption(mpDss);
			tcpHeader.AppendOption(mpOption);*/
		}


	}
	if(GetWinScale()>0){
		if(tcpHeader.GetFlags() == TcpHeader::SYN)
		{
			Ptr<TcpOptionWinScale> winscale = TcpOption::CreateOption(3)->GetObject<TcpOptionWinScale>();
			winscale->SetScale(m_rcvscale);
			tcpHeader.AppendOption(winscale);
		} else if (tcpHeader.GetFlags() == (TcpHeader::SYN | TcpHeader::ACK))
		{
			Ptr<TcpOptionWinScale> winscale = TcpOption::CreateOption(3)->GetObject<TcpOptionWinScale>();
			winscale->SetScale(m_rcvscale);
		    tcpHeader.AppendOption(winscale);


		}
	}

}
uint32_t
TcpSocketBase::CalculateFragDss(SequenceNumber32 seq){
	NS_LOG_FUNCTION(this << seq);
	std::map<SequenceNumber32,SequenceNumber64>::iterator iter;
	std::map<SequenceNumber64,uint32_t>::iterator it;
	iter=m_seqTxMap.find(seq);
	if(iter!=m_seqTxMap.end()){
		it=m_sizeTxMap.find(iter->second);
		if(it!=m_sizeTxMap.end()){
			return it->second;
		}
	}
	return 0;
}

void
TcpSocketBase::CalculateFragDss(SequenceNumber32 seq, uint32_t size)
{

	NS_LOG_FUNCTION(this << seq << m_frag2 << size << m_fragment2 << m_cumsize2);
	std::map<SequenceNumber32,SequenceNumber64>::iterator iter;
	std::map<SequenceNumber64,uint32_t>::iterator it;
	iter=m_seqTxMap.find(seq);
	if(iter!=m_seqTxMap.end()){
		NS_LOG_LOGIC("Seq " << iter->first << " " << iter->second);
		if(m_fragment2){
			if(!m_cumsize2){
				NS_LOG_LOGIC("cumsize");
				it=m_sizeTxMap.find(iter->second);
				if(it->second-m_cumfrag2>0){
					m_frag2 = it->second-m_cumfrag2;
					m_cumfrag2+=m_frag2;
					m_cumsize2+=m_frag2;
				} else {
					m_frag2=0;
				}
			}else{
				NS_LOG_LOGIC("no cumsize");
				it=m_sizeTxMap.find(iter->second);

				if(it->second>=size-(m_cumsize2-m_cumfrag2)){
					m_frag2 = size-(m_cumsize2-m_cumfrag2);
				}else{
					m_frag2=it->second;
				}
				m_cumfrag2+=m_frag2;
				m_cumsize2+=m_frag2;
			}

			if(m_cumsize2>=size){
				m_cumsize2=0;
				return;
			} else
			if(m_cumfrag2>=it->second){
				m_cumfrag2=0;
				m_numfrags++;
				CalculateFragDss(seq + it->second,size);

			} else {
				m_numfrags++;
				CalculateFragDss(seq,size);
			}
		}
		else{
			it=m_sizeTxMap.find(iter->second);
			NS_LOG_LOGIC("Seq2 " << iter->first << " " << iter->second << " " << it->second);

			if(it->second<size){
				m_fragment2=true;
				m_frag2=it->second;
				m_cumsize2+=m_frag2;
				m_numfrags++;
				CalculateFragDss(seq + it->second,size);
			}
		}


	} else {
		m_frag2=0;
		m_cumsize2=0;
		m_cumfrag2=0;
	}
}

void
TcpSocketBase::CalculateDss(TcpHeader& tcpHeader, SequenceNumber32 seq, uint32_t size)
{

	NS_LOG_FUNCTION(this << seq << m_frag << size);
	Ptr<TcpOptionMptcp> mpOption = TcpOption::CreateOption(30)->GetObject<TcpOptionMptcp>();
	Ptr<TcpOptionMpDss> mpDss = TcpSubOption::CreateSubOption(2)->GetObject<TcpOptionMpDss>();
	std::map<SequenceNumber32,SequenceNumber64>::iterator iter;
	std::map<SequenceNumber64,uint32_t>::iterator it;
	iter=m_seqTxMap.find(seq);
	NS_LOG_LOGIC(iter->second);
	if(iter!=m_seqTxMap.end()){
		if(m_fragment){
			NS_LOG_LOGIC(this << " DSS Sent Seq3:" << " cumsize " << m_cumsize << " cumfrag " << m_cumfrag);
			if(!m_cumsize){
				it=m_sizeTxMap.find(iter->second);
				NS_LOG_LOGIC("cumsize " << it->second);

				if(it->second-m_cumfrag>0){
					SequenceNumber64 seq64 = iter->second;
					SequenceNumber32 seq32= SequenceNumber32(uint32_t(seq64.GetValue()));
					mpDss->SetDataSequence32(seq32);
					//mpDss->SetSubflowSequence(tcpHeader.GetSequenceNumber ());
					mpDss->SetDataLength(it->second-m_cumfrag);
					mpOption->AddSubOption(mpDss);
					tcpHeader.AppendOption(mpOption);
					m_frag = it->second-m_cumfrag;
					NS_LOG_LOGIC(this << " DSS Sent Seq: " << seq32 << " length: " << m_frag);
					m_cumfrag+=m_frag;
					m_cumsize+=m_frag;
				} else {
					m_frag=0;
				}
			}else{
				NS_LOG_LOGIC("no cumsize");
				it=m_sizeTxMap.find(iter->second);
				SequenceNumber64 seq64 = iter->second;
				SequenceNumber32 seq32= SequenceNumber32(uint32_t(seq64.GetValue()));
				mpDss->SetDataSequence32(seq32);
				//mpDss->SetSubflowSequence(tcpHeader.GetSequenceNumber ());
				if(it->second>=size-(m_cumsize-m_cumfrag)){
					mpDss->SetDataLength(size-(m_cumsize-m_cumfrag));
					m_frag = size-(m_cumsize-m_cumfrag);
				}else{
					mpDss->SetDataLength(it->second);
					m_frag=it->second;
				}
				NS_LOG_LOGIC(this << " DSS Sent Seq: " << seq32 << " length: " << m_frag);
				mpOption->AddSubOption(mpDss);
				tcpHeader.AppendOption(mpOption);
				m_cumfrag+=m_frag;
				m_cumsize+=m_frag;
			}

			NS_LOG_LOGIC(this << " DSS Sent Seq1:" << seq << " frag:" << m_frag << " seq " << tcpHeader.GetSequenceNumber() << " cumsize " << m_cumsize << " cumfrag " << m_cumfrag);

			if(m_cumsize>=size&&m_cumfrag>=it->second){
				m_cumfrag=0;
				m_cumsize=0;
				m_frag=0;
				return;
			}
			if(m_cumsize>=size){
				m_cumsize=0;
				return;
			} else
			if(m_cumfrag>=it->second){
				m_cumfrag=0;
				CalculateDss(tcpHeader,seq + it->second,size);

			} else {
				CalculateDss(tcpHeader,seq,size);
			}
		}
		else{
			it=m_sizeTxMap.find(iter->second);
			NS_LOG_LOGIC(this << " DSS Sent Seq2:" << seq<< " frag:" << it->second << " seq " << tcpHeader.GetSequenceNumber());
			SequenceNumber64 seq64 = iter->second;
			SequenceNumber32 seq32= SequenceNumber32(uint32_t(seq64.GetValue()));
			mpDss->SetDataSequence32(seq32);
			mpDss->SetSubflowSequence(tcpHeader.GetSequenceNumber ());
			mpDss->SetDataLength(it->second);
			mpOption->AddSubOption(mpDss);
			tcpHeader.AppendOption(mpOption);
			//NS_LOG_LOGIC(this << " DSS Sent Seq: " << seq32 << " length: " << it->second);

			if(it->second<size){
				m_fragment=true;
				m_frag=it->second;
				m_cumsize+=m_frag;
				CalculateDss(tcpHeader,seq + it->second,size);
			}
		}


	} else {
		m_frag=0;
		m_cumsize=0;
		m_cumfrag=0;
	}
}
/** Associate a node with this TCP socket */
void
TcpSocketBase::SetNode (Ptr<Node> node)
{
  m_node = node;
}

/** Associate the L4 protocol (e.g. mux/demux) with this socket */
void
TcpSocketBase::SetTcp (Ptr<TcpL4Protocol> tcp)
{
  m_tcp = tcp;
}

/** Set an RTT estimator with this socket */
void
TcpSocketBase::SetRtt (Ptr<RttEstimator> rtt)
{
  m_rtt = rtt;
}

Ptr<RttEstimator>
TcpSocketBase::GetRtt (void)
{
  return m_rtt;
}


} // namespace ns3