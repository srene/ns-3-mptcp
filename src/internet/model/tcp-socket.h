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

#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "rtt-estimator.h"
#include "tcp-tx-buffer.h"
#include "tcp-rx-buffer.h"
//#include "tcp-l4-protocol.h"
//#include "tcp-socket-base.h"

namespace ns3 {

class Node;
class Packet;
class TcpSocketBase;
class TcpL4Protocol;

/* Names of the 11 TCP states */
typedef enum {
  CLOSED,       // 0
  LISTEN,       // 1
  SYN_SENT,     // 2
  SYN_RCVD,     // 3
  ESTABLISHED,  // 4
  CLOSE_WAIT,   // 5
  LAST_ACK,     // 6
  FIN_WAIT_1,   // 7
  FIN_WAIT_2,   // 8
  CLOSING,      // 9
  TIME_WAIT,   // 10
  LAST_STATE
} TcpStates_t;

/**
 * \ingroup socket
 *
 * \brief (abstract) base class of all TcpSockets
 *
 * This class exists solely for hosting TcpSocket attributes that can
 * be reused across different implementations.
 */
class TcpSocket : public Socket
{
public:
  static TypeId GetTypeId (void);
 
  TcpSocket (void);
  TcpSocket (TypeId socketTypeId);
  virtual ~TcpSocket (void);
  virtual void CreateSocket(TypeId socketTypeId);
  virtual void NotifyDataReceived(void);
  virtual void SetNode (Ptr<Node> node);
  virtual void SetTcp (Ptr<TcpL4Protocol> tcp);
  virtual void SetRtt (Ptr<RttEstimator> rtt);
  bool GetSucceed();
  void SetSocketBase (Ptr<TcpSocketBase> socketBase);
  void SetRxSocket(Ptr<TcpSocketBase> socketBase);
  void ResetCallback(void);
  void SetError(enum SocketErrno errno);
  // Necessary implementations of null functions from ns3::Socket
  virtual enum SocketErrno GetErrno (void) const;    // returns m_errno
  virtual enum SocketType GetSocketType (void) const; // returns socket type
  virtual Ptr<Node> GetNode (void) const;            // returns m_node
  virtual int Bind (void);    // Bind a socket by setting up endpoint in TcpL4Protocol
  virtual int Bind6 (void);    // Bind a socket by setting up endpoint in TcpL4Protocol
  virtual int Bind (const Address &address);         // ... endpoint of specific addr or port
  virtual int Connect (const Address &address);      // Setup endpoint and call ProcessAction() to connect
  virtual int Listen (void);  // Verify the socket is in a correct state and call ProcessAction() to listen
  virtual int Close (void);   // Close by app: Kill socket upon tx buffer emptied
  virtual int ShutdownSend (void);    // Assert the m_shutdownSend flag to prevent send to network
  virtual int ShutdownRecv (void);    // Assert the m_shutdownRecv flag to prevent forward to app
  virtual int Send (Ptr<Packet> p, uint32_t flags);  // Call by app to send data to network
  virtual int SendTo (Ptr<Packet> p, uint32_t flags, const Address &toAddress); // Same as Send(), toAddress is insignificant
  virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags); // Return a packet to be forwarded to app
  virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress); // ... and write the remote address at fromAddress
  virtual uint32_t GetTxAvailable (void) const; // Available Tx buffer size
  virtual uint32_t GetRxAvailable (void) const; // Available-to-read data size, i.e. value of m_rxAvailable
  virtual int GetSockName (Address &address) const; // Return local addr:port in address
  virtual void BindToNetDevice (Ptr<NetDevice> netdevice); // NetDevice with my m_endPoint
  virtual bool     SetAllowBroadcast (bool allowBroadcast);
  virtual bool     GetAllowBroadcast (void) const;
  virtual bool GetTcpMpCapable();
  virtual void SetTcpMp (bool multipath);
  virtual bool GetTcpMp (void) const;
  virtual void SetTcpMpCapable(void);
  virtual void SetLocalKey(uint64_t);
  virtual void SetRemoteKey(uint64_t);
  virtual uint64_t GetLocalKey (void);
  virtual uint64_t GetRemoteKey (void);
  virtual void Join(void);
  virtual void SetMpcapSent(bool sent);
  virtual bool GetMpcapSent(void);
  virtual int NotifySubflowConnectionSucceeded(Ptr<TcpSocketBase> socket);
  void SetSndBufSize (uint32_t size);
  uint32_t GetSndBufSize (void) const;
  void SetRcvBufSize (uint32_t size);
  uint32_t GetRcvBufSize (void) const;
  int GetPending(void);
 // TcpTxBuffer                   m_txBuffer;       //< Tx buffer
 // TcpRxBuffer                   m_rxBuffer;
  bool m_succeed;
  std::vector<Ptr<TcpSocketBase> > GetSocketsBase(void);
  std::vector<Ptr<TcpSocketBase> > GetRxSocketsBase(void);
protected:
  std::vector<Ptr<TcpSocketBase> > m_sockets;
  std::vector<Ptr<TcpSocketBase> > m_rxSockets;

  //Ptr<TcpSocketBase> m_tcpSocketBase;
  //Ptr<TcpSocketBase> m_tcpRxSocketBase;
  enum SocketErrno    m_errno;         //< Socket error code
  bool                     m_shutdownSend;  //< Send no longer allowed
  bool                     m_shutdownRecv;  //< Receive no longer allowed
  Ptr<Node> m_node;
  Ptr<TcpL4Protocol> m_tcp;
private:
  // Indirect the attribute setting and getting through private virtual methods

  void SetSegSize (uint32_t size);
  uint32_t GetSegSize (void) const;
  void SetSSThresh (uint32_t threshold);
  uint32_t GetSSThresh (void) const;
  void SetInitialCwnd (uint32_t count);
  uint32_t GetInitialCwnd (void) const;
  void SetConnTimeout (Time timeout);
  Time GetConnTimeout (void) const;
  void SetConnCount (uint32_t count);
  uint32_t GetConnCount (void) const;
  void SetDelAckTimeout (Time timeout);
  Time GetDelAckTimeout (void) const;
  void SetDelAckMaxCount (uint32_t count);
  uint32_t GetDelAckMaxCount (void) const;
  void SetTcpNoDelay (bool noDelay);
  bool GetTcpNoDelay (void) const;
  void SetPersistTimeout (Time timeout);
  Time GetPersistTimeout (void) const;

  TypeId m_socketTypeId;


};

} // namespace ns3

#endif /* TCP_SOCKET_H */


