/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#ifndef BULK_SEND_MP_APPLICATION2_H
#define BULK_SEND_MP_APPLICATION2_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/data-rate.h"

namespace ns3 {

class Address;
class Socket;
class RandomVariableStream;

//class MpTcpSocket;

/**
 * \ingroup applications
 * \defgroup bulksend BulkSendApplication
 *
 * This traffic generator simply sends data
 * as fast as possible up to MaxBytes or until
 * the appplication is stopped if MaxBytes is
 * zero. Once the lower layer send buffer is
 * filled, it waits until space is free to
 * send more data, essentially keeping a
 * constant flow of data. Only SOCK_STREAM 
 * and SOCK_SEQPACKET sockets are supported. 
 * For example, TCP sockets can be used, but 
 * UDP sockets can not be used.
 */
class BulkSendMpApplication2 : public Application
{
public:
  static TypeId GetTypeId (void);

  BulkSendMpApplication2 ();

  virtual ~BulkSendMpApplication2 ();


  /**
   * \param maxBytes the total number of bytes to send
   *
   * Set the total number of bytes to send. Once these bytes are sent, no packet
   * is sent again, even in on state. The value zero means that there is no
   * limit.
   */
  void SetMaxBytes (uint32_t maxBytes);

  /**
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;


  void SetDataRate(DataRate rate);

 /**
  * Assign a fixed random variable stream number to the random variables
  * used by this model.  Return the number of streams (possibly zero) that
  * have been assigned.
  *
  * \param stream first stream index to use
  * \return the number of stream indices assigned by this model
  */
  int64_t AssignStreams (int64_t stream);
  void SetCallback (Callback<void> callback);
protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  //helpers
  void CancelEvents ();

  void Construct (Ptr<Node> n,
                  const Address &remote,
                  std::string tid,
                  const RandomVariable& ontime,
                  const RandomVariable& offtime,
                  uint32_t size);


  // Event handlers
  void StartSending ();
  void StopSending ();
  void SendPacket ();
  void DataSend (Ptr<Socket>, uint32_t);
  Callback<void> m_callback;

  Ptr<Socket>     m_socket;       // Associated socket
  Address         m_peer;         // Peer address
  bool            m_connected;    // True if connected
  Ptr<RandomVariableStream>  m_onTime;       // rng for On Time
  Ptr<RandomVariableStream>  m_offTime;      // rng for Off Time
  DataRate        m_cbrRate;      // Rate that data is generated
  uint32_t        m_pktSize;      // Size of packets
  uint32_t        m_residualBits; // Number of generated, but not sent, bits
  Time            m_lastStartTime; // Time last packet sent
  uint32_t        m_maxBytes;     // Limit total number of bytes sent
  uint32_t        m_totBytes;     // Total bytes sent so far
  EventId         m_startStopEvent;     // Event id for next start or stop event
  EventId         m_sendEvent;    // Eventid of pending "send packet" event
  bool            m_sending;      // True if currently in sending state
  TypeId          m_tid;
  TracedCallback<Ptr<const Packet> > m_txTrace;
  TypeId	  	  m_socketType;
  TypeId		  m_congestion;
  bool m_succeed;
  uint32_t m_packetId;
  std::vector<Ptr<Packet> > buff;
private:
  void ScheduleNextTx ();
  void ScheduleStartEvent ();
  void ScheduleStopEvent ();
  void ConnectionSucceeded (Ptr<Socket>);
  void ConnectionFailed (Ptr<Socket>);
  void Ignore (Ptr<Socket>);
  void Succeeded(void);
};

} // namespace ns3

#endif /* BULK_SEND_APPLICATION_H */
