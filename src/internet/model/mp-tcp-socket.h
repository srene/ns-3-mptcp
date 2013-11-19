/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
 *               2007 INRIA
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
 * Authors: George F. Riley<riley@ece.gatech.edu>
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef MP_TCP_SOCKET_H
#define MP_TCP_SOCKET_H

#include "ns3/tcp-socket.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/tcp-socket-base.h"
#include "tcp-tx-buffer64.h"
#include "tcp-rx-buffer64.h"
#include "ns3/inet-socket-address.h"
#include "mp-scheduler.h"

namespace ns3 {

typedef std::map<Ptr<TcpSocketBase>,uint32_t> SocketPacketMap;
typedef std::pair<Ptr<TcpSocketBase>,uint32_t> SocketPacketPair;
typedef std::map<Ptr<TcpSocketBase>,uint32_t>::iterator SocketPacketIte;
typedef std::pair<SequenceNumber64,uint32_t> SizePair;
typedef std::map<SequenceNumber64,uint32_t> SizeMap;


typedef enum {
  Round_Robin,        // 0
  Water_Filling,
  Wardrop
  } DataDistribAlgo_t;


/**
 * \ingroup socket
 *
 * \brief (abstract) base class of all TcpSockets
 *
 * This class exists solely for hosting TcpSocket attributes that can
 * be reused across different implementations.
 */

class MpTcpSocket : public TcpSocket
{
public:

  typedef std::pair<SequenceNumber64,uint8_t > SeqPair;
  typedef std::map<SequenceNumber64, uint8_t > SeqMap;

  static TypeId GetTypeId (void);
 
  MpTcpSocket (void);
  virtual ~MpTcpSocket (void);

  int GetPending(void);
  int GetPending(SequenceNumber64 seq);

  void SetSubflowCallback (Callback<void, Ptr<Packet> > socketSubflow);
  void SetRecvCallback (Callback<void, Ptr<Packet>,SequenceNumber64, Ptr<TcpSocketBase> > socketRecv);

  void SetSubflow (uint8_t subflow,uint32_t size);
  void CloseSubflow(InetSocketAddress socket);
  int Discard (SequenceNumber64 ack);
  SequenceNumber64 GetRxSequence();
  virtual void ReceivedData(Ptr<TcpSocketBase> socket, SequenceNumber64 seq, Ptr<Packet> p);
  virtual void ReceivedAck(Ptr<TcpSocketBase> socket, SequenceNumber64 ack);
  virtual void SetTcpMp (bool multipath);
  virtual bool GetTcpMp (void) const;
  virtual void SetMpcapSent(bool sent);
  virtual bool GetMpcapSent(void);
  virtual void SetTcpMpCapable(void);
  virtual void SetLocalKey(uint64_t);
  virtual void SetRemoteKey(uint64_t);
  virtual uint64_t GetLocalKey (void);
  virtual uint64_t GetRemoteKey (void);
  virtual void Join(InetSocketAddress socket, TypeId congestion);
  virtual int Send (Ptr<Packet> p, uint32_t flags);  // Call by app to send data to network
  virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags); // Return a packet to be forwarded to app
  virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress); // ... and write the remote address at fromAddress
  virtual bool GetTcpMpCapable();
  virtual int NotifySubflowConnectionSucceeded(Ptr<TcpSocketBase> socket);
  void SetRcvBufSize (uint32_t size);
  DataDistribAlgo_t GetDistribAlgo (void) const;
  void SetDistribAlgo (DataDistribAlgo_t distribAlgo);
  uint32_t GetRcvBufSize (void) const;
  TypeId GetSocketTypeId();
  uint32_t GetCwndTotal(void);
  double GetAlpha(void);
  uint32_t AdvertisedWindowSize ();
  Ptr<MpScheduler> GetScheduler();


private:
  bool SocketCanSend(Ptr<TcpSocketBase> socket);
  TcpRxBuffer64  m_rxBuffer;
  TracedValue<TcpStates_t> m_state;         //< TCP state

  bool 	m_multipath;
  uint64_t m_localKey;
  uint64_t m_remoteKey;
  bool m_capable;
  bool m_mpcapSent;
  // Rx and Tx buffer management
  TracedValue<SequenceNumber64> m_nextTxSequence; //< Next seqnum to be sent (SND.NXT), ReTx pushes it back
  TracedValue<SequenceNumber64> m_highTxMark;     //< Highest seqno ever sent, regardless of ReTx
  DataDistribAlgo_t m_distribAlgo;
  TypeId m_socketTypeId;
  uint32_t m_segmentSize;
  SeqMap seqTx;

  TracedCallback<Ptr<const Packet>, const Address &> m_packetRxTrace;
  TracedCallback<Ptr<TcpSocketBase> > m_joined;
  TracedValue<uint32_t>  m_cWndTotal;
  TracedValue<double>  m_alpha;
  //TracedValue<uint32_t> m_pSize;
  uint16_t              m_maxWinSize;  //< Maximum window size to advertise
  Callback<void, Ptr<Packet> > m_socketSubflow;
  Callback<void, Ptr<Packet>, SequenceNumber64, Ptr<TcpSocketBase> > m_socketRecv;

  uint8_t m_subflow;

  Ptr<MpScheduler> m_scheduler;

  SocketPacketMap sPacketMap;
  //SizeMap m_sizeTxMap;
  double m_landaRate;
  double m_lastSampleLanda;
  double m_lastLanda;
  Time t;
  uint32_t data;

  TracedCallback<Ptr<const Packet> > m_drop;

  bool succeed;
};

} // namespace ns3

#endif /* MP_TCP_SOCKET_H */


