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

#include "tcp-partial-mih.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"
#include "tcp-option-mih.h"

NS_LOG_COMPONENT_DEFINE ("TcpPartialMih");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpPartialMih);

TypeId
TcpPartialMih::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpPartialMih")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpPartialMih> ()
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&TcpPartialMih::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpPartialMih::m_cWnd))
  ;
  return tid;
}

TcpPartialMih::TcpPartialMih (void) : m_retxThresh (3), m_inFastRec (false) , m_delay(0), m_freeze(false)
{
  NS_LOG_FUNCTION (this);
}

TcpPartialMih::TcpPartialMih (const TcpPartialMih& sock)
  : TcpSocketBase (sock),
    m_cWnd (sock.m_cWnd),
    m_ssThresh (sock.m_ssThresh),
    m_initialCWnd (sock.m_initialCWnd),
    m_retxThresh (sock.m_retxThresh),
    m_inFastRec (false),
    m_freeze(false)

{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpPartialMih::~TcpPartialMih (void)
{
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpPartialMih::Listen (void)
{
  NS_LOG_FUNCTION (this);
  InitializeCwnd ();
  return TcpSocketBase::Listen ();
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpPartialMih::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  InitializeCwnd ();
  return TcpSocketBase::Connect (address);
}

/** Limit the size of in-flight data by cwnd and receiver's rxwin */
uint32_t
TcpPartialMih::Window (void)
{
  NS_LOG_FUNCTION (this);
  return std::min (m_rWnd.Get (), m_cWnd.Get ());
}

Ptr<TcpSocketBase>
TcpPartialMih::Fork (void)
{
  return CopyObject<TcpPartialMih> (this);
}

/** New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
void
TcpPartialMih::NewAck (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("TcpReno receieved ACK for seq " << seq <<
                " cwnd " << m_cWnd <<
                " ssthresh " << m_ssThresh);

  // Check for exit condition of fast recovery
  if (m_inFastRec)
    { // RFC2001, sec.4; RFC2581, sec.3.2
      // First new ACK after fast recovery: reset cwnd
      m_cWnd = m_ssThresh;
      m_inFastRec = false;
      NS_LOG_INFO ("Reset cwnd to " << m_cWnd);
    };

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
  if(m_freeze){
      m_retxEvent.Cancel ();
  }

}

// Fast recovery and fast retransmit
void
TcpPartialMih::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << "t " << count);
  if (count == m_retxThresh && !m_inFastRec)
    { // triple duplicate ack triggers fast retransmit (RFC2581, sec.3.2)
      m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
      m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_inFastRec = true;
      NS_LOG_INFO ("Triple dupack. Reset cwnd to " << m_cWnd << ", ssthresh to " << m_ssThresh);
      DoRetransmit ();
    }
  else if (m_inFastRec)
    { // In fast recovery, inc cwnd for every additional dupack (RFC2581, sec.3.2)
      m_cWnd += m_segmentSize;
      NS_LOG_INFO ("Increased cwnd to " << m_cWnd);
      SendPendingData (m_connected);
    };
}

// Retransmit timeout
void TcpPartialMih::Retransmit (void)
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
TcpPartialMih::SetSegSize (uint32_t size)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpReno::SetSegSize() cannot change segment size after connection started.");
  m_segmentSize = size;
}

void
TcpPartialMih::SetSSThresh (uint32_t threshold)
{
  m_ssThresh = threshold;
}

uint32_t
TcpPartialMih::GetSSThresh (void) const
{
  return m_ssThresh;
}

void
TcpPartialMih::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpReno::SetInitialCwnd() cannot change initial cwnd after connection started.");
  m_initialCWnd = cwnd;
}

uint32_t
TcpPartialMih::GetInitialCwnd (void) const
{
  return m_initialCWnd;
}

void 
TcpPartialMih::InitializeCwnd (void)
{
  /*
   * Initialize congestion window, default to 1 MSS (RFC2001, sec.1) and must
   * not be larger than 2 MSS (RFC2581, sec.3.1). Both m_initiaCWnd and
   * m_segmentSize are set by the attribute system in ns3::TcpSocket.
   */
  m_cWnd = m_initialCWnd * m_segmentSize;
}

void
TcpPartialMih::SetHandover (uint32_t delay, uint32_t n, uint32_t throughput, Mac48Address mac)
{

	m_handover = true;
	Ptr<TcpOption> tcp;
	m_tcpMih = tcp->CreateOption(31)->GetObject<TcpOptionMih>();
	m_tcpMih->SetHandover(0,delay,n,throughput,mac);
	m_rxBuffer.m_nextRxSeq++;
	SendEmptyPacket(TcpHeader::ACK);

}
void
TcpPartialMih::Handover (uint32_t delay, uint32_t n, uint32_t throughput)
{
	Time t;
	if(m_delay>delay)
		t = MicroSeconds(m_delay-delay);
	else
		t = MicroSeconds(delay-m_delay);

		//std::cout << "Estimate "<< m_rtt->GetCurrentEstimate ().GetSeconds() << " " << t.GetSeconds() << " " << delay << " " << m_delay << std::endl;
		m_rtt->SetCurrentEstimate (m_rtt->GetCurrentEstimate () + t);

    m_delay = delay;

    //m_ssThresh = m_freezeSsThresh;
    //m_cWnd = m_freezeCwnd;
    //m_rWnd = m_freezerWnd;
    //m_freeze = false;
   // SendPendingData (m_connected);


}


void
TcpPartialMih::PostHandover ()
{


}
void
TcpPartialMih::Freeze ()
{
	NS_LOG_FUNCTION (this);
	if(m_state == ESTABLISHED)
	{
		m_freezeCwnd = m_cWnd;
		m_freezeSsThresh = m_ssThresh;
		m_ssThresh = 0;
		m_cWnd = 0;
		m_freezerWnd = m_rWnd;
		m_rWnd = 0;
		m_freeze=true;
	}
}

void
TcpPartialMih::Unfreeze ()
{
	NS_LOG_FUNCTION (this);

	if(m_freeze){
		m_ssThresh = m_freezeSsThresh;
		m_cWnd = m_freezeCwnd;
		m_rWnd = m_freezerWnd;
		m_freeze = false;
		SendPendingData (m_connected);
	}
}
//m_ssThresh = m_freezeSsThresh;
//m_cWnd = m_freezeCwnd;
//m_rWnd = m_freezerWnd;
//m_freeze = false;
// SendPendingData (m_connected);
bool
TcpPartialMih::IsLost(SequenceNumber32 seq) const
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
