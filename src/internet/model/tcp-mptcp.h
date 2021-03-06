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

#ifndef TCP_MPTCP_H
#define TCP_MPTCP_H

#include "tcp-socket-base.h"
#include "ns3/packet.h"

namespace ns3 {

/**
 * \ingroup socket
 * \ingroup tcp
 *
 * \brief An implementation of a stream socket using TCP.
 *
 * This class contains the NewReno implementation of TCP, as of RFC2582.
 */
class TcpMpTcp : public TcpSocketBase
{
public:
  static TypeId GetTypeId (void);
  /**
   * Create an unbound tcp socket.
   */
  TcpMpTcp (void);
  TcpMpTcp (const TcpMpTcp& sock);
  virtual ~TcpMpTcp (void);
  virtual double GetCurrentBw (void);
  virtual Time GetMinRtt(void);
  virtual double GetLastBw (void);

  // From TcpSocketBase
 // virtual int Connect (const Address &address);
 // virtual int Listen (void);

protected:
  virtual uint32_t Window (void); // Return the max possible number of unacked bytes
  virtual Ptr<TcpSocketBase> Fork (void); // Call CopyObject<TcpMpTcp> to clone me
  virtual void NewAck (SequenceNumber32 const& seq); // Inc cwnd and call NewAck() of parent
  virtual void ReceivedAck (Ptr<Packet> packet, const TcpHeader& tcpHeader);
  virtual void EstimateRtt (const TcpHeader& header);
  virtual void DupAck (const TcpHeader& t, uint32_t count);  // Halving cwnd and reset nextTxSequence
  virtual void Retransmit (void); // Exit fast recovery upon retransmit timeout
  virtual uint32_t GetCongestionWindow (void);
  // Implementing ns3::TcpSocket -- Attribute get/set
  virtual void     SetSegSize (uint32_t size);
  virtual void     SetSSThresh (uint32_t threshold);
  virtual uint32_t GetSSThresh (void) const;
  virtual void     SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;
private:
  void InitializeCwnd (void);            // set m_cWnd when connection starts
  int CountAck (const TcpHeader& tcpHeader);
  void UpdateAckedSegments (int acked);
  void EstimateBW (int acked, const TcpHeader& tcpHeader, Time rtt);
  void Filtering (void);

protected:
  TracedValue<uint32_t>  m_cWnd;         //< Congestion window
  uint32_t               m_ssThresh;     //< Slow Start Threshold
  uint32_t               m_initialCWnd;  //< Initial cWnd value
  SequenceNumber32       m_recover;      //< Previous highest Tx seqnum for fast recovery
  uint32_t               m_retxThresh;   //< Fast Retransmit threshold
  bool                   m_inFastRec;    //< currently in fast recovery
  bool                   m_limitedTx;    //< perform limited transmit

  TracedValue<double>    m_currentBW;              //< Current value of the estimated BW
  double                 m_lastSampleBW;           //< Last bandwidth sample
  double                 m_lastBW;                 //< Last bandwidth sample after being filtered
  Time                   m_minRtt;                 //< Minimum RTT
  double                 m_lastAck;                //< The time last ACK was received
  SequenceNumber32       m_prevAckNo;              //< Previously received ACK number
  int                    m_accountedFor;           //< The number of received DUPACKs

  int                    m_ackedSegments;          //< The number of segments ACKed between RTTs
  bool                   m_IsCount;                //< Start keeping track of m_ackedSegments for Westwood+ if TRUE
  EventId                m_bwEstimateEvent;        //< The BW estimation event for Westwood+
};

} // namespace ns3

#endif /* TCP_NEWRENO_H */
