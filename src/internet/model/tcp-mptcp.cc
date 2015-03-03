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

/*
  #define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }
*/
#include "tcp-mptcp.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"
#include "ns3/mp-tcp-socket.h"

NS_LOG_COMPONENT_DEFINE ("TcpMpTcp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpMpTcp);

TypeId
TcpMpTcp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpMpTcp")
    .SetParent<TcpSocketBase> ()
    .AddConstructor<TcpMpTcp> ()
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&TcpMpTcp::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LimitedTransmit", "Enable limited transmit",
		    BooleanValue (false),
		    MakeBooleanAccessor (&TcpMpTcp::m_limitedTx),
		    MakeBooleanChecker ())
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpMpTcp::m_cWnd))
  ;
  return tid;
}

TcpMpTcp::TcpMpTcp (void)
  : m_retxThresh (3), // mute valgrind, actual value set by the attribute system
    m_inFastRec (false),
    m_limitedTx (false) // mute valgrind, actual value set by the attribute system
{
  NS_LOG_FUNCTION (this << GetInstanceTypeId());

}

TcpMpTcp::TcpMpTcp (const TcpMpTcp& sock)
  : TcpSocketBase (sock),
    m_cWnd (sock.m_cWnd),
    m_ssThresh (sock.m_ssThresh),
    m_initialCWnd (sock.m_initialCWnd),
    m_retxThresh (sock.m_retxThresh),
    m_inFastRec (false),
    m_limitedTx (sock.m_limitedTx)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor" << GetInstanceTypeId ());

}

TcpMpTcp::~TcpMpTcp (void)
{
}

double TcpMpTcp::GetCurrentBw (void)
{
	return m_lastBW;
}
double TcpMpTcp::GetLastBw (void)
{
	return m_currentBW;
}
Time TcpMpTcp::GetMinRtt(void)
{
	NS_LOG_FUNCTION(this << m_minRtt);
	return m_minRtt;
}
/** We initialize m_cWnd from this function, after attributes initialized */
/*
int
TcpMpTcp::Listen (void)
{
  NS_LOG_FUNCTION (this);
  InitializeCwnd ();
  return TcpSocketBase::Listen ();
}

int
TcpMpTcp::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  InitializeCwnd ();
  return TcpSocketBase::Connect (address);
}*/

/** Limit the size of in-flight data by cwnd and receiver's rxwin */
uint32_t
TcpMpTcp::Window (void)
{
  NS_LOG_FUNCTION (this << m_rWnd.Get () << m_cWnd.Get () << std::min (m_rWnd.Get (), m_cWnd.Get ()));
  return std::min (m_rWnd.Get (), m_cWnd.Get ());
}

uint32_t
TcpMpTcp::GetCongestionWindow (void)
{
  NS_LOG_FUNCTION (this);
  return m_cWnd.Get ();

}

Ptr<TcpSocketBase>
TcpMpTcp::Fork (void)
{
  NS_LOG_FUNCTION(this << GetInstanceTypeId ());
  return CopyObject<TcpMpTcp> (this);
}

void
TcpMpTcp::ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this);
  int acked = 0;
  if ((0 != (tcpHeader.GetFlags () & TcpHeader::ACK)) && tcpHeader.GetAckNumber() >= m_prevAckNo)
    {// It is a duplicate ACK or a new ACK. Old ACK is ignored.
	  if (m_IsCount)
		{
		  acked = CountAck (tcpHeader);
		  UpdateAckedSegments (acked);
		}
    }

  TcpSocketBase::ReceivedAck (packet, tcpHeader);
}

int
TcpMpTcp::CountAck (const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this);

  // Calculate the number of acknowledged segments based on the received ACK number
  int cumul_ack = (tcpHeader.GetAckNumber() - m_prevAckNo) / m_segmentSize;

  if (cumul_ack == 0)
    {// A DUPACK counts for 1 segment delivered successfully
      m_accountedFor++;
      cumul_ack = 1;
    }
  if (cumul_ack > 1)
    {// A delayed ACK or a cumulative ACK after a retransmission
     // Check how much new data it ACKs
      if (m_accountedFor >= cumul_ack)
        {
          m_accountedFor -= cumul_ack;
          cumul_ack = 1;
        }
      else if (m_accountedFor < cumul_ack)
        {
          cumul_ack -= m_accountedFor;
          m_accountedFor = 0;
        }
    }

  // Update the previous ACK number
  m_prevAckNo = tcpHeader.GetAckNumber();

  return cumul_ack;
}

void
TcpMpTcp::UpdateAckedSegments (int acked)
{
  m_ackedSegments += acked;
}

void
TcpMpTcp::EstimateRtt (const TcpHeader& tcpHeader)
{
  //NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_FUNCTION(this<<m_minRtt<<m_lastRtt);
  // Calculate m_lastRtt
  TcpSocketBase::EstimateRtt (tcpHeader);

  // Update minRtt
  if (m_minRtt == 0)
    {
      m_minRtt = m_lastRtt;
    }
  else
    {
      if (m_lastRtt < m_minRtt)
        {
          m_minRtt = m_lastRtt;
        }
    }

  // For Westwood+, start running a clock on the currently estimated RTT if possible
  // to trigger a new BW sampling event
	 if(m_lastRtt != 0 && m_state == ESTABLISHED && !m_IsCount)
	   {
		 m_IsCount = true;
		 m_bwEstimateEvent.Cancel();
		 m_bwEstimateEvent = Simulator::Schedule (m_lastRtt, &TcpMpTcp::EstimateBW,this,m_ackedSegments,tcpHeader,m_lastRtt);
	   }

}

void
TcpMpTcp::EstimateBW (int acked, const TcpHeader& tcpHeader, Time rtt)
{
  NS_LOG_FUNCTION (this);

      // Calculate the BW
      m_currentBW = m_ackedSegments * m_segmentSize / rtt.GetSeconds();
      // Reset m_ackedSegments and m_IsCount for the next sampling
      m_ackedSegments = 0;
      m_IsCount = false;

  NS_LOG_LOGIC(m_currentBW);
  // Filter the BW sample
  Filtering();
  NS_LOG_LOGIC(m_lastBW);

}

void
TcpMpTcp::Filtering ()
{
  NS_LOG_FUNCTION (this);

  double alpha = 0.9;


      double sample_bwe = m_currentBW;
      m_currentBW = (alpha * m_lastBW) + ((1 - alpha) * ((sample_bwe + m_lastSampleBW) / 2));
      m_lastSampleBW = sample_bwe;
      m_lastBW = m_currentBW;
}

/** New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
void
TcpMpTcp::NewAck (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("TcpMpTcp receieved ACK for seq " << seq <<
                " cwnd " << m_cWnd <<
                " ssthresh " << m_ssThresh);

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
      //double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ();

	  double adder = std::min( m_socket->GetObject<MpTcpSocket>()->GetAlpha() * static_cast<double>(m_segmentSize * m_segmentSize) / m_socket->GetObject<MpTcpSocket>()->GetCwndTotal(),static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ());
      adder = std::max (1.0, adder);
      m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, cwnd " << m_cWnd.Get ());
      NS_LOG_INFO ("In CongAvoid, alpha " << m_socket->GetObject<MpTcpSocket>()->GetAlpha() << " cwndtotal " << m_socket->GetObject<MpTcpSocket>()->GetCwndTotal());
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh << " adder " << adder);
    }

  // Complete newAck processing
  TcpSocketBase::NewAck (seq);
}

/** Cut cwnd and enter fast recovery mode upon triple dupack */
void
TcpMpTcp::DupAck (const TcpHeader& t, uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  if (count == m_retxThresh && !m_inFastRec)
    { // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
      m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
      m_cWnd = m_ssThresh + 3 * m_segmentSize;
      m_recover = m_highTxMark;
      m_inFastRec = true;
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
TcpMpTcp::Retransmit (void)
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
TcpMpTcp::SetSegSize (uint32_t size)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpMpTcp::SetSegSize() cannot change segment size after connection started.");
  m_segmentSize = size;
}

void
TcpMpTcp::SetSSThresh (uint32_t threshold)
{
  m_ssThresh = threshold;
}

uint32_t
TcpMpTcp::GetSSThresh (void) const
{
  return m_ssThresh;
}

void
TcpMpTcp::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS (m_state == CLOSED, "TcpMpTcp::SetInitialCwnd() cannot change initial cwnd after connection started.");
  m_initialCWnd = cwnd;
  InitializeCwnd ();

}

uint32_t
TcpMpTcp::GetInitialCwnd (void) const
{
  return m_initialCWnd;
}

void 
TcpMpTcp::InitializeCwnd (void)
{
  /*
   * Initialize congestion window, default to 1 MSS (RFC2001, sec.1) and must
   * not be larger than 2 MSS (RFC2581, sec.3.1). Both m_initiaCWnd and
   * m_segmentSize are set by the attribute system in ns3::TcpSocket.
   */
  m_cWnd = m_initialCWnd * m_segmentSize;
}

} // namespace ns3
