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

#ifndef TCP_RX_BUFFER64_H
#define TCP_RX_BUFFER64_H

#include <map>
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/sequence-number.h"
#include "ns3/ptr.h"
#include "ns3/tcp-header.h"

namespace ns3 {
class Packet;

/**
 * \ingroup tcp
 *
 * \brief class for the reordering buffer that keeps the data from lower layer, i.e.
 *        TcpL4Protocol, sent to the application
 */
class TcpRxBuffer64 : public Object
{
public:
  static TypeId GetTypeId (void);
  TcpRxBuffer64 (uint64_t n = 0);
  virtual ~TcpRxBuffer64 ();

  // Accessors
  SequenceNumber64 NextRxSequence (void) const;
  SequenceNumber64 MaxRxSequence (void) const;
  void IncNextRxSequence (void);
  void SetNextRxSequence (const SequenceNumber64& s);
  void SetFinSequence (const SequenceNumber64& s);
  uint64_t MaxBufferSize (void) const;
  void SetMaxBufferSize (uint64_t s);
  uint64_t Size (void) const;
  uint64_t Available () const;
  bool Finished (void);

  /**
   * Insert a packet into the buffer and update the availBytes counter to
   * reflect the number of bytes ready to send to the application. This
   * function handles overlap by triming the head of the inputted packet and
   * removing data from the buffer that overlaps the tail of the inputted
   * packet
   *
   * \return True when success, false otherwise.
   */
  bool Add (Ptr<Packet> p, SequenceNumber64 seq);

  /**
   * Extract data from the head of the buffer as indicated by nextRxSeq.
   * The extracted data is going to be forwarded to the application.
   */
  Ptr<Packet> Extract (uint64_t maxSize);
public:
  typedef std::map<SequenceNumber64, Ptr<Packet> >::iterator BufIterator;
  TracedValue<SequenceNumber64> m_nextRxSeq; //< Seqnum of the first missing byte in data (RCV.NXT)
  SequenceNumber64 m_finSeq;                 //< Seqnum of the FIN packet
  bool m_gotFin;                             //< Did I received FIN packet?
  uint64_t m_size;                           //< Number of total data bytes in the buffer, not necessarily contiguous
  uint64_t m_maxBuffer;                      //< Upper bound of the number of data bytes in buffer (RCV.WND)
  uint64_t m_availBytes;                     //< Number of bytes available to read, i.e. contiguous block at head
  std::map<SequenceNumber64, Ptr<Packet> > m_data;
  //< Corresponding data (may be null)
};

} //namepsace ns3

#endif /* TCP_RX_BUFFER_H */
