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
#ifndef TCP_SOCKET_BASE_H
#define TCP_SOCKET_BASE_H

#include <stdint.h>
#include <queue>
#include "ns3/callback.h"
#include "ns3/traced-value.h"
#include "ns3/tcp-socket.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-interface.h"
#include "ns3/event-id.h"
#include "tcp-tx-buffer.h"
#include "tcp-rx-buffer.h"
#include "rtt-estimator.h"
#include "tcp-option-mptcp.h"

namespace ns3 {

class Ipv4EndPoint;
class Ipv6EndPoint;
class Node;
class Packet;
class TcpL4Protocol;
class TcpHeader;
//class TcpSocket;

/* Names of the 11 TCP states */

/**
 * \ingroup socket
 * \ingroup tcp
 *
 * \brief A base class for implementation of a stream socket using TCP.
 *
 * This class contains the essential components of TCP, as well as a sockets
 * interface for upper layers to call. This serves as a base for other TCP
 * functions where the sliding window mechanism is handled here. This class
 * provides connection orientation and sliding window flow control. Part of
 * this class is modified from the original NS-3 TCP socket implementation
 * (TcpSocketImpl) by Raj Bhattacharjea <raj.b@gatech.edu> of Georgia Tech.
 */
class TcpSocketBase : public Object
{
public:

	typedef std::pair<SequenceNumber32,SequenceNumber64> SeqPair;
	typedef std::map<SequenceNumber32,SequenceNumber64> SeqMap;
	typedef std::pair<SequenceNumber64,uint32_t> SizePair;
	typedef std::map<SequenceNumber64,uint32_t> SizeMap;
  static TypeId GetTypeId (void);
  /**
   * Create an unbound TCP socket
   */
  TcpSocketBase (void);

  /**
   * Clone a TCP socket, for use upon receiving a connection request in LISTEN state
   */
  TcpSocketBase (const TcpSocketBase& sock);

  virtual ~TcpSocketBase (void);

  // Literal names of TCP states for use in log messages */
  static const char* const TcpStateName[LAST_STATE];

  // Set associated Node, TcpL4Protocol, RttEstimator to this socket
  virtual void SetNode (Ptr<Node> node);
  virtual void SetTcp (Ptr<TcpL4Protocol> tcp);
  virtual void SetRtt (Ptr<RttEstimator> rtt);

  Ptr<RttEstimator> GetRtt (void);
  void SetSucceed();
  void SetDataAck (SequenceNumber64 dataAck);
  int Send (Ptr<Packet> p, SequenceNumber64 seq, uint32_t flags);
  int Allocate(void);
  int Allocate(uint16_t port);
  int Allocate(Ipv4Address ipv4);
  int Allocate(Ipv4Address ipv4,uint16_t port);
  int Allocate6(void);
  int Allocate6(uint16_t port);
  int Allocate6(Ipv6Address ipv6);
  int Allocate6(Ipv6Address ipv6,uint16_t port);
  uint32_t GetRxAvailable (void) const;
  uint32_t GetTxAvailable (void) const;
  int Connect(const Address& address);
  TcpStates_t GetState();
  void SetState(TcpStates_t state);
  void SetSocket(Ptr<Socket> socket);
  int GetSocketName(Address &address) const;
  int Close();
  Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags);
  int Send (Ptr<Packet> p, uint32_t flags);
  Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress);
  void BindToNetDevice (Ptr<NetDevice> netdevice);
  enum Socket::SocketErrno GetErrno (void) const;    // returns m_errno
  Ptr<Node> GetNode (void) const;
  void SetShutdownSend(bool shutdown);
  void SetShutdownRecv(bool shutdown);
  int GetPending();
  // Necessary implementations of null functions from ns3::Socket
  /*virtual enum SocketErrno GetErrno (void) const;    // returns m_errno
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
  virtual void BindToNetDevice (Ptr<NetDevice> netdevice); // NetDevice with my m_endPoint*/
  Ipv4EndPoint* GetEndPoint();
  // Implementing ns3::TcpSocket -- Attribute get/set
  void     SetSndBufSize (uint32_t size);
  uint32_t GetSndBufSize (void) const;
  void     SetRcvBufSize (uint32_t size);
  uint32_t GetRcvBufSize (void) const;
  void     SetSegSize (uint32_t size);
  uint32_t GetSegSize (void) const;
  virtual void     SetSSThresh (uint32_t threshold) = 0;
  virtual uint32_t GetSSThresh (void) const = 0;
  virtual void     SetInitialCwnd (uint32_t cwnd) = 0;
  virtual uint32_t GetInitialCwnd (void) const = 0;
  void     SetConnTimeout (Time timeout);
  Time     GetConnTimeout (void) const;
  void     SetConnCount (uint32_t count);
  uint32_t GetConnCount (void) const;
  void     SetDelAckTimeout (Time timeout);
  Time     GetDelAckTimeout (void) const;
  void     SetDelAckMaxCount (uint32_t count);
  uint32_t GetDelAckMaxCount (void) const;
  void     SetTcpNoDelay (bool noDelay);
  bool     GetTcpNoDelay (void) const;
  void	   SetWinScale (uint32_t winscale);
  uint32_t GetWinScale (void) const;
  void     SetPersistTimeout (Time timeout);
  Time     GetPersistTimeout (void) const;
  bool     SetAllowBroadcast (bool allowBroadcast);
  bool     GetAllowBroadcast (void) const;
  virtual uint32_t GetCongestionWindow (void)  = 0;
  virtual double GetCurrentBw (void) = 0;
  virtual double GetLastBw (void) = 0;
  virtual Time GetMinRtt(void)  = 0;
  virtual uint32_t BytesInFlight (void);        // Return total bytes in flight
  virtual uint32_t AvailableWindow (void);      // Return unfilled portion of window
  virtual uint32_t UnSentData (void);
  virtual uint32_t UnAckDataCount (void);       // Return count of number of unacked bytes
  virtual uint32_t BufferedData(void);
  virtual Time GetLastRtt ();

protected:


  // Helper functions: Connection set up
  int SetupCallback (void);        // Common part of the two Bind(), i.e. set callback and remembering local addr:port
  int DoConnect (void);            // Sending a SYN packet to make a connection if the state allows
  void ConnectionSucceeded (void); // Schedule-friendly wrapper for Socket::NotifyConnectionSucceeded()
  int SetupEndpoint (void);        // Configure m_endpoint for local addr for given remote addr
  int SetupEndpoint6 (void);       // Configure m_endpoint6 for local addr for given remote addr
  void CompleteFork (Ptr<Packet>, const TcpHeader&, const Address& fromAddress, const Address& toAdress);

  // Helper functions: Transfer operation
  void ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface);
  void ForwardUp6 (Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr, uint16_t port);
  virtual void DoForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface); //Get a pkt from L3
  virtual void DoForwardUp (Ptr<Packet> packet, Ipv6Address saddr, Ipv6Address daddr, uint16_t port); // Ipv6 version
  bool SendPendingData (bool withAck = false); // Send as much as the window allows
  uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck); // Send a data packet
  void SendEmptyPacket (uint8_t flags); // Send a empty packet that carries a flag, e.g. ACK
  void SendRST (void); // Send reset and tear down this socket
  bool OutOfRange (SequenceNumber32 head, SequenceNumber32 tail) const; // Check if a sequence number range is within the rx window

  // Helper functions: Connection close
  int DoClose (void); // Close a socket by sending RST, FIN, or FIN+ACK, depend on the current state
  void CloseAndNotify (void); // To CLOSED state, notify upper layer, and deallocate end point
  void Destroy (void); // Kill this socket by zeroing its attributes
  void Destroy6 (void); // Kill this socket by zeroing its attributes
  void DeallocateEndPoint (void); // Deallocate m_endPoint
  void PeerClose (Ptr<Packet>, const TcpHeader&); // Received a FIN from peer, notify rx buffer
  void DoPeerClose (void); // FIN is in sequence, notify app and respond with a FIN
  void CancelAllTimers (void); // Cancel all timer when endpoint is deleted
  void TimeWait (void);  // Move from CLOSING or FIN_WAIT_2 to TIME_WAIT state

  // State transition functions
  void ProcessEstablished (Ptr<Packet>, const TcpHeader&); // Received a packet upon ESTABLISHED state
  void ProcessListen (Ptr<Packet>, const TcpHeader&, const Address&, const Address&); // Process the newly received ACK
  void ProcessSynSent (Ptr<Packet>, const TcpHeader&); // Received a packet upon SYN_SENT
  void ProcessSynRcvd (Ptr<Packet>, const TcpHeader&, const Address&, const Address&); // Received a packet upon SYN_RCVD
  void ProcessWait (Ptr<Packet>, const TcpHeader&); // Received a packet upon CLOSE_WAIT, FIN_WAIT_1, FIN_WAIT_2
  void ProcessClosing (Ptr<Packet>, const TcpHeader&); // Received a packet upon CLOSING
  void ProcessLastAck (Ptr<Packet>, const TcpHeader&); // Received a packet upon LAST_ACK

  // Window management
  virtual uint32_t Window (void);               // Return the max possible number of unacked bytes
  virtual uint32_t AdvertisedWindowSize (void); // The amount of Rx window announced to the peer

  // Manage data tx/rx
  virtual Ptr<TcpSocketBase> Fork (void) = 0; // Call CopyObject<> to clone me
  virtual void ReceivedAck (Ptr<Packet>, const TcpHeader&); // Received an ACK packet
  virtual void ReceivedData (Ptr<Packet>, const TcpHeader&); // Recv of a data, put into buffer, call L7 to get it if necessary
  virtual void EstimateRtt (const TcpHeader&); // RTT accounting
  virtual void NewAck (SequenceNumber32 const& seq); // Update buffers w.r.t. ACK
  virtual void DupAck (const TcpHeader& t, uint32_t count) = 0; // Received dupack
  virtual void ReTxTimeout (void); // Call Retransmit() upon RTO event
  virtual void Retransmit (void); // Halving cwnd and call DoRetransmit()
  virtual void DelAckTimeout (void);  // Action upon delay ACK timeout, i.e. send an ACK
  virtual void LastAckTimeout (void); // Timeout at LAST_ACK, close the connection
  virtual void PersistTimeout (void); // Send 1 byte probe to get an updated window size
  virtual void DoRetransmit (void); // Retransmit the oldest packet
  virtual void ReadOptions (const TcpHeader&); // Read option from incoming packets
  virtual void AddOptions (TcpHeader&, uint32_t size); // Add option to outgoing packets
  virtual void CalculateDss(TcpHeader& tcpHeader,SequenceNumber32 seq, uint32_t size);
  virtual void CalculateFragDss(SequenceNumber32 seq, uint32_t size);
  virtual uint32_t CalculateFragDss(SequenceNumber32 seq);

protected:
  // Counters and events
  EventId           m_retxEvent;       //< Retransmission event
  EventId           m_lastAckEvent;    //< Last ACK timeout event
  EventId           m_delAckEvent;     //< Delayed ACK timeout event
  EventId           m_persistEvent;    //< Persist event: Send 1 byte to probe for a non-zero Rx window
  EventId           m_timewaitEvent;   //< TIME_WAIT expiration event: Move this socket to CLOSED state
  uint32_t          m_dupAckCount;     //< Dupack counter
  uint32_t          m_delAckCount;     //< Delayed ACK counter
  uint32_t          m_delAckMaxCount;  //< Number of packet to fire an ACK before delay timeout
  bool              m_noDelay;         //< Set to true to disable Nagle's algorithm
  uint32_t          m_cnCount;         //< Count of remaining connection retries
  uint32_t          m_cnRetries;       //< Number of connection retries before giving up
  TracedValue<Time> m_rto;             //< Retransmit timeout
  TracedValue<Time> m_lastRtt;         //< Last RTT sample collected
  Time              m_delAckTimeout;   //< Time to delay an ACK
  Time              m_persistTimeout;  //< Time between sending 1-byte probes
  Time              m_cnTimeout;       //< Timeout for connection retry

  // Connections to other layers of TCP/IP
  Ipv4EndPoint*       m_endPoint;
  Ipv6EndPoint*       m_endPoint6;
  Ptr<Node>           m_node;
  Ptr<TcpL4Protocol>  m_tcp;

  // Round trip time estimation
  Ptr<RttEstimator> m_rtt;

  // Rx and Tx buffer management
  TracedValue<SequenceNumber32> m_nextTxSequence; //< Next seqnum to be sent (SND.NXT), ReTx pushes it back
  TracedValue<SequenceNumber32> m_highTxMark;     //< Highest seqno ever sent, regardless of ReTx
  TcpRxBuffer                   m_rxBuffer;       //< Rx buffer (reordering buffer)
  TcpTxBufferWrapper            m_txBuffer;       //< Tx buffer

  SequenceNumber64 m_lastSeq;
  uint32_t	m_size;
  bool m_fragment;
  //SequenceNumber64 m_prevSeq;
  uint32_t m_frag;
  uint32_t m_cumfrag;
  uint32_t m_cumsize;
  bool m_fragment2;
  uint32_t m_frag2;
  uint32_t m_cumfrag2;
  uint32_t m_cumsize2;
  uint32_t m_numfrags;
  // State-related attributes
  TracedValue<TcpStates_t> m_state;         //< TCP state
  enum Socket::SocketErrno         m_errno;         //< Socket error code
  bool                     m_closeNotified; //< Told app to close socket
  bool                     m_closeOnEmpty;  //< Close socket upon tx buffer emptied
  bool                     m_shutdownSend;  //< Send no longer allowed
  bool                     m_shutdownRecv;  //< Receive no longer allowed
  bool                     m_connected;     //< Connection established
  double                   m_msl;           //< Max segment lifetime

  // Window management
  uint32_t              m_segmentSize; //< Segment size
  //uint16_t              m_maxWinSize;  //< Maximum window size to advertise
  uint32_t              m_maxWinSize;
  TracedValue<uint32_t> m_rWnd;        //< Flow control window at remote side

  Ptr<TcpSocket> m_socket;

  SeqMap m_seqTxMap;
  SizeMap m_sizeTxMap;
  SizeMap m_sizeRxMap;
  //SeqMap m_seqRxMap;

  bool m_succeed;

  Ptr<NetDevice> m_boundnetdevice;

  SequenceNumber64 m_dataSeq;
  SequenceNumber64 m_dataAck;

  uint8_t m_sndscale;
  uint8_t m_rcvscale;


  TracedValue<uint32_t> m_buffsize;
  uint32_t m_maxBuffsize;


};

} // namespace ns3

#endif /* TCP_SOCKET_BASE_H */
