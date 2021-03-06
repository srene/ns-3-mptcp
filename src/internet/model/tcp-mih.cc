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

#include "tcp-mih.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"
#include "tcp-option-mih.h"

NS_LOG_COMPONENT_DEFINE ("TcpMih");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpMih);

TypeId
TcpMih::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpMih")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpMih> ()
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&TcpMih::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpMih::m_cWnd))
  ;
  return tid;
}

TcpMih::TcpMih (void) : m_retxThresh (3), m_inFastRec (false), m_delay(0), m_freeze(false)
{
  NS_LOG_FUNCTION (this);
}

TcpMih::TcpMih (const TcpMih& sock)
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

TcpMih::~TcpMih (void)
{
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpMih::Listen (void)
{
  NS_LOG_FUNCTION (this);
  InitializeCwnd ();
  return TcpSocketBase::Listen ();
}

/** We initialize m_cWnd from this function, after attributes initialized */
int
TcpMih::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  InitializeCwnd ();
  return TcpSocketBase::Connect (address);
}

/** Limit the size of in-flight data by cwnd and receiver's rxwin */
uint32_t
TcpMih::Window (void)
{
  //NS_LOG_FUNCTION (this);
  return std::min (m_rWnd.Get (), m_cWnd.Get ());
}

Ptr<TcpSocketBase>
TcpMih::Fork (void)
{
  return CopyObject<TcpMih> (this);
}

/** New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
void
TcpMih::NewAck (const SequenceNumber32& seq)
{
  //NS_LOG_FUNCTION (this << seq);
  //NS_LOG_LOGIC ("TcpReno receieved ACK for seq " << seq <<
   //             " cwnd " << m_cWnd <<
  //              " ssthresh " << m_ssThresh);

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
 //     NS_LOG_INFO ("In SlowStart, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }
  else
    { // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
      // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
      double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ();
      adder = std::max (1.0, adder);
      m_cWnd += static_cast<uint32_t> (adder);
  //    NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
    }

  // Complete newAck processing
  TcpSocketBase::NewAck (seq);
  if(m_freeze){
      m_retxEvent.Cancel ();
  }

}

// Fast recovery and fast retransmit
void
TcpMih::DupAck (const TcpHeader& t, uint32_t count)
{
  //NS_LOG_FUNCTION (this << "t " << count);
  if (count == m_retxThresh && !m_inFastRec)
    { // triple duplicate ack triggers fast retransmit (RFC2581, sec.3.2)
      //m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
      m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_inFastRec = true;
      NS_LOG_INFO ("Triple dupack. Reset cwnd to " << m_cWnd << ", ssthresh to " << m_ssThresh);
      DoRetransmit ();
    }
  else if (m_inFastRec)
    { // In fast recovery, inc cwnd for every additional dupack (RFC2581, sec.3.2)
      //m_cWnd += m_segmentSize;
      NS_LOG_INFO ("Increased cwnd to " << m_cWnd);
      SendPendingData (m_connected);
    };
}

// Retransmit timeout
void TcpMih::Retransmit (void)
{
  //NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
  m_inFastRec = false;

  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT) return;
  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= ESTABLISHED && m_txBuffer.HeadSequence () >= m_highTxMark) return;

  // According to RFC2581 sec.3.1, upon RTO, ssthresh is set to half of flight
  // size and cwnd is set to 1*MSS, then the lost packet is retransmitted and
  // TCP back to slow start
  //m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
  m_cWnd = m_ssThresh;
  m_nextTxSequence = m_txBuffer.HeadSequence (); // Restart from highest Ack
  NS_LOG_INFO ("RTO. Reset cwnd to " << m_cWnd <<
               ", ssthresh to " << m_ssThresh << ", restart from seqnum " << m_nextTxSequence);
  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  DoRetransmit ();                          // Retransmit the packet
}

void
TcpMih::SetSegSize (uint32_t size)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpReno::SetSegSize() cannot change segment size after connection started.");
  m_segmentSize = size;
}

void
TcpMih::SetSSThresh (uint32_t threshold)
{
  m_ssThresh = threshold;
}

uint32_t
TcpMih::GetSSThresh (void) const
{
  return m_ssThresh;
}

void
TcpMih::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpReno::SetInitialCwnd() cannot change initial cwnd after connection started.");
  m_initialCWnd = cwnd;
}

uint32_t
TcpMih::GetInitialCwnd (void) const
{
  return m_initialCWnd;
}

void 
TcpMih::InitializeCwnd (void)
{
  /*
   * Initialize congestion window, default to 1 MSS (RFC2001, sec.1) and must
   * not be larger than 2 MSS (RFC2581, sec.3.1). Both m_initiaCWnd and
   * m_segmentSize are set by the attribute system in ns3::TcpSocket.
   */
  m_cWnd = m_initialCWnd * m_segmentSize;
}

void
TcpMih::SetHandover (uint8_t direct, uint32_t delay, uint32_t n, uint32_t throughput, Mac48Address mac)
{
	NS_LOG_FUNCTION(this);
	if(m_state == ESTABLISHED){
		NS_LOG_LOGIC("AddOption " << m_handover);
		m_handover = true;
		Ptr<TcpOption> tcp;
		m_tcpMih = tcp->CreateOption(31)->GetObject<TcpOptionMih>();
		m_tcpMih->SetHandover(direct,delay,n,throughput,mac);
		if(direct==0)
		{
			m_rxBuffer.m_nextRxSeq++;
			SendEmptyPacket(TcpHeader::ACK);
		}else if (direct==1||direct==2){
			uint32_t sz = SendDataPacket (m_nextTxSequence, m_segmentSize, true);
			m_nextTxSequence += sz;                     // Advance next tx sequence
		}
	}

}
void
TcpMih::Handover (uint32_t delay, uint32_t n, uint32_t throughput)
{
	if(m_state == ESTABLISHED)
	{
		//double rtt = (2*(double)delay)/1000000;
		Time t,t2;
		if(m_delay>delay)
			t2 = MicroSeconds(m_delay-delay);
		else
			t2 = MicroSeconds(delay-m_delay);
		//t = MicroSeconds(m_rtt->GetCurrentEstimate ().GetMicroSeconds()-(2*t2.GetMicroSeconds()));
                t = Seconds(0.25-(2*t2.GetMicroSeconds()));
                if (t<0)t = MicroSeconds(m_validRtt.Get().GetMicroSeconds()-(2*t2.GetMicroSeconds()));
                if (t<0)t = MicroSeconds(1000);
                NS_LOG_LOGIC (this << " rtt " << m_rtt->GetCurrentEstimate ().GetSeconds() << "t2" << Seconds(2*t2.GetSeconds()));
		m_delay = delay;
		uint32_t bdp;
		if(throughput<1000000){
			bdp = ((3000000/8) / n) * (t.GetSeconds());
			//bdp = ((3000000/8) / n) * (Seconds(0.5));
		} else {
			bdp = ((throughput/8) / n) * (t.GetSeconds());
			//bdp = ((throughput/8) / n) * (Seconds(0.5));
		}

		NS_LOG_LOGIC (this << " delay " << delay << " throughput " << throughput << " n " << n << " m_validRtt " << m_validRtt.Get().GetSeconds() << " " << t.GetSeconds() << " " << t2.GetSeconds() << " rtt " << m_rtt->GetCurrentEstimate ().GetSeconds() << " bdp " << bdp);
		//m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
		m_ssThresh = std::max (2 * m_segmentSize, bdp);
		m_cWnd = m_ssThresh;
		NS_LOG_LOGIC("TCP Handover " << m_cWnd);

		m_rtt->SetCurrentEstimate (t);


		if(m_freeze){
			m_rWnd = m_freezerWnd;
			m_freeze = false;
			SendPendingData (m_connected);
		}
	}

}


void
TcpMih::PostHandover ()
{


}
void
TcpMih::Freeze ()
{
	NS_LOG_FUNCTION (this);

	if(m_state == ESTABLISHED && !m_freeze)
	{
		m_ssThresh = 0;
		m_cWnd = 0;
		m_freezerWnd = m_rWnd;
		m_rWnd = 0;
		m_freeze=true;
		uint32_t sz = SendDataPacket (m_nextTxSequence, m_segmentSize, true);
		m_nextTxSequence += sz;                     // Advance next tx sequence
	}

}
bool
TcpMih::IsLost(SequenceNumber32 seq) const
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
