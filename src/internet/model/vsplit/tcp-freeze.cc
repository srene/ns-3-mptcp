/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "tcp-freeze.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"

NS_LOG_COMPONENT_DEFINE ("TcpFreeze");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpFreeze);

TypeId
TcpFreeze::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpFreeze")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpFreeze> ()
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&TcpFreeze::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LimitedTransmit", "Enable limited transmit",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpFreeze::m_limitedTx),
		    MakeBooleanChecker ())
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpFreeze::m_cWnd))
  ;
  return tid;
}

TcpFreeze::TcpFreeze (void)
  : m_retxThresh (3), // mute valgrind, actual value set by the attribute system
    m_inFastRec (false),
    m_limitedTx (false), // mute valgrind, actual value set by the attribute system
    m_freeze(false)
{
  NS_LOG_FUNCTION (this);
  SetDelAckMaxCount (0);

}

TcpFreeze::TcpFreeze (const TcpFreeze& sock)
  : TcpSocketBase (sock),
    m_cWnd (sock.m_cWnd),
    m_ssThresh (sock.m_ssThresh),
    m_initialCWnd (sock.m_initialCWnd),
    m_retxThresh (sock.m_retxThresh),
    m_inFastRec (false),
    m_limitedTx (sock.m_limitedTx),
    m_freeze(false)

{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
  SetDelAckMaxCount (0);
}

TcpFreeze::~TcpFreeze (void)
{
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpFreeze::Listen (void)
{
  NS_LOG_FUNCTION (this);
  InitializeCwnd ();
  return TcpSocketBase::Listen ();
}

void
TcpFreeze::handover (uint16_t delay)
{
	NS_LOG_FUNCTION (this);
}
void
TcpFreeze::Freeze ()
{
	if(m_state == ESTABLISHED && !m_freeze)
	{
	NS_LOG_FUNCTION (this);
	m_freezeCwnd = m_cWnd;
	m_freezeSsThresh = m_ssThresh;
	m_ssThresh = 0;
	m_cWnd = 0;
	m_freezerWnd = m_rWnd;
	m_rWnd = 0;
	m_freeze = true;
	}

}

void
TcpFreeze::Unfreeze ()
{
	NS_LOG_FUNCTION (this);
	if(m_state == ESTABLISHED && m_freeze)
	{
		m_cWnd = m_freezeCwnd;
		m_ssThresh = m_freezeSsThresh;
		m_rWnd = m_freezerWnd;
		SendPendingData (m_connected);
		m_freeze = false;
	}

}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpFreeze::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  InitializeCwnd ();
  return TcpSocketBase::Connect (address);
}

/** Limit the size of in-flight data by cwnd and receiver's rxwin */
uint32_t
TcpFreeze::Window (void)
{
  NS_LOG_FUNCTION (this);
  return std::min (m_rWnd.Get (), m_cWnd.Get ());
}

Ptr<TcpSocketBase>
TcpFreeze::Fork (void)
{
  return CopyObject<TcpFreeze> (this);
}

/** New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
void
TcpFreeze::NewAck (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("TcpFreeze receieved ACK for seq " << seq <<
                " cwnd " << m_cWnd <<
                " ssthresh " << m_ssThresh);
  if(GetTcpSackPermitted())SetPipe();
  // Check for exit condition of fast recovery
  if (m_inFastRec && seq < m_recover)
    { // Partial ACK, partial window deflation (RFC2582 sec.3 bullet #5 paragraph 3)
      m_cWnd -= seq - m_txBuffer.HeadSequence ();
      m_cWnd += m_segmentSize;  // increase cwnd
      NS_LOG_INFO ("Partial ACK in fast recovery: cwnd set to " << m_cWnd);
      TcpSocketBase::NewAck (seq); // update m_nextTxSequence and send new data if allowed by window
      DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
      return;
    }
  else if (m_inFastRec && seq >= m_recover)
    { // Full ACK (RFC2582 sec.3 bullet #5 paragraph 2, option 1)
      m_cWnd = std::min (m_ssThresh, BytesInFlight () + m_segmentSize);
      m_inFastRec = false;
      NS_LOG_INFO ("Received full ACK. Leaving fast recovery with cwnd set to " << m_cWnd);
    }

  // Increase of cwnd based on current phase (slow start or congestion avoidance)
  if (m_cWnd < m_ssThresh)
    { // Slow start mode, add one segSize to cWnd. Default m_ssThresh is 65535. (RFC2001, sec.1)
      m_cWnd += m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }
  else
    { // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
      // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
      double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ();
      adder = std::max (1.0, adder);
      m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }

  // Complete newAck processing
  TcpSocketBase::NewAck (seq);
}

/** Cut cwnd and enter fast recovery mode upon triple dupack */
void
TcpFreeze::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  if (count == m_retxThresh && !m_inFastRec)
    { // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
      m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
      m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_recover = m_highTxMark;
      m_inFastRec = true;
      if(GetTcpSackPermitted())SetPipe();
      NS_LOG_INFO ("Triple dupack. Enter fast recovery mode. Reset cwnd to " << m_cWnd <<
                   ", ssthresh to " << m_ssThresh << " at fast recovery seqnum " << m_recover);
      DoRetransmit ();
    }
  else if (m_inFastRec)
    { // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
      m_cWnd += m_segmentSize;
      NS_LOG_INFO ("Dupack in fast recovery mode. Increase cwnd to " << m_cWnd);
      SendPendingData (m_connected);
    }
  else if (!m_inFastRec && m_limitedTx && m_txBuffer.SizeFromSequence (m_nextTxSequence) > 0)
    { // RFC3042 Limited transmit: Send a new packet for each duplicated ACK before fast retransmit
      NS_LOG_INFO ("Limited transmit");
      uint32_t sz = SendDataPacket (m_nextTxSequence, m_segmentSize, true);
      m_nextTxSequence += sz;                    // Advance next tx sequence
    };
}

/** Retransmit timeout */
void
TcpFreeze::Retransmit (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
  m_inFastRec = false;

  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT) return;
  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer.HeadSequence () >= m_highTxMark) return;

  // According to RFC2581 sec.3.1, upon RTO, ssthresh is set to half of flight
  // size and cwnd is set to 1*MSS, then the lost packet is retransmitted and
  // TCP back to slow start
  m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
  m_cWnd = m_segmentSize;
  m_nextTxSequence = m_txBuffer.HeadSequence (); // Restart from highest Ack
  NS_LOG_INFO ("RTO. Reset cwnd to " << m_cWnd <<
               ", ssthresh to " << m_ssThresh << ", restart from seqnum " << m_nextTxSequence);
  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  DoRetransmit ();                          // Retransmit the packet
}

void
TcpFreeze::SetSegSize (uint32_t size)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpFreeze::SetSegSize() cannot change segment size after connection started.");
  m_segmentSize = size;
}

void
TcpFreeze::SetSSThresh (uint32_t threshold)
{
  m_ssThresh = threshold;
}

uint32_t
TcpFreeze::GetSSThresh (void) const
{
  return m_ssThresh;
}

void
TcpFreeze::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpFreeze::SetInitialCwnd() cannot change initial cwnd after connection started.");
  m_initialCWnd = cwnd;
}

uint32_t
TcpFreeze::GetInitialCwnd (void) const
{
  return m_initialCWnd;
}

void 
TcpFreeze::InitializeCwnd (void)
{
  /*
   * Initialize congestion window, default to 1 MSS (RFC2001, sec.1) and must
   * not be larger than 2 MSS (RFC2581, sec.3.1). Both m_initiaCWnd and
   * m_segmentSize are set by the attribute system in ns3::TcpSocket.
   */
  m_cWnd = m_initialCWnd * m_segmentSize;
}

bool
TcpFreeze::IsLost(SequenceNumber32 seq) const
{
    NS_ASSERT (GetTcpSackPermitted());
    // RFC 3517, page 3: "This routine returns whether the given sequence number is
    // considered to be lost.  The routine returns true when either
    // DupThresh discontiguous SACKed sequences have arrived above
    // 'SeqNum' or (DupThresh * SMSS) bytes with sequence numbers greater
    // than 'SeqNum' have been SACKed.  Otherwise, the routine returns
    // false."
    bool isLost = false;

    SequenceNumber32 snd_una = m_txBuffer.HeadSequence ();
    NS_ASSERT(seqGE(seq,snd_una)); // HighAck = snd_una

    if (m_tcpSackRexmitQueue->getNumOfDiscontiguousSacks(seq) >= m_retxThresh ||     // DUPTHRESH = 3
    		m_tcpSackRexmitQueue->getAmountOfSackedBytes(seq) >= (m_retxThresh * m_segmentSize))
        isLost = true;
    else
        isLost = false;

    return isLost;
}


} // namespace ns3
