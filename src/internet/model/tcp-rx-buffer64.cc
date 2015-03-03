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

#include "ns3/packet.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "tcp-rx-buffer64.h"

NS_LOG_COMPONENT_DEFINE ("TcpRxBuffer64");

namespace ns3 {

TypeId
TcpRxBuffer64::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpRxBuffer64")
    .SetParent<Object> ()
    .AddConstructor<TcpRxBuffer64> ()
    .AddTraceSource ("NextRxSequence",
                     "Next sequence number expected (RCV.NXT)",
                     MakeTraceSourceAccessor (&TcpRxBuffer64::m_nextRxSeq))
  ;
  return tid;
}

/* A user is supposed to create a TcpSocket through a factory. In TcpSocket,
 * there are attributes SndBufSize and RcvBufSize to control the default Tx and
 * Rx window sizes respectively, with default of 128 KiByte. The attribute
 * RcvBufSize is passed to TcpRxBuffer64 by TcpSocketBase::SetRcvBufSize() and in
 * turn, TcpRxBuffer64:SetMaxBufferSize(). Therefore, the m_maxBuffer value
 * initialized below is insignificant.
 */
TcpRxBuffer64::TcpRxBuffer64 (uint64_t n)
  : m_nextRxSeq (n), m_gotFin (false), m_size (0), m_maxBuffer (32768), m_availBytes (0)
{
}

TcpRxBuffer64::~TcpRxBuffer64 ()
{
}

SequenceNumber64
TcpRxBuffer64::NextRxSequence (void) const
{
  return m_nextRxSeq;
}

void
TcpRxBuffer64::SetNextRxSequence (const SequenceNumber64& s)
{
  m_nextRxSeq = s;
}

uint64_t
TcpRxBuffer64::MaxBufferSize (void) const
{
  return m_maxBuffer;
}

void
TcpRxBuffer64::SetMaxBufferSize (uint64_t s)
{
  m_maxBuffer = s;
}

uint64_t
TcpRxBuffer64::Size (void) const
{
  return m_size;
}

uint64_t
TcpRxBuffer64::Available () const
{
  return m_availBytes;
}

void
TcpRxBuffer64::IncNextRxSequence ()
{
  NS_LOG_FUNCTION (this);
  // Increment nextRxSeq is valid only if we don't have any data buffered,
  // this is supposed to be called only during the three-way handshake
  NS_ASSERT (m_size == 0);
  m_nextRxSeq++;
}

// Return the lowest sequence number that this TcpRxBuffer64 cannot accept
SequenceNumber64
TcpRxBuffer64::MaxRxSequence (void) const
{
  if (m_gotFin)
    { // No data allowed beyond FIN
      return m_finSeq;
    }
  else if (m_data.size ())
    { // No data allowed beyond Rx window allowed
      return m_data.begin ()->first + SequenceNumber64 (m_maxBuffer);
    }
  return m_nextRxSeq + SequenceNumber64 (m_maxBuffer);
}

void
TcpRxBuffer64::SetFinSequence (const SequenceNumber64& s)
{
  NS_LOG_FUNCTION (this);

  m_gotFin = true;
  m_finSeq = s;
  if (m_nextRxSeq == m_finSeq) ++m_nextRxSeq;
}

bool
TcpRxBuffer64::Finished (void)
{
  return (m_gotFin && m_finSeq < m_nextRxSeq);
}

bool
TcpRxBuffer64::Add (Ptr<Packet> p, SequenceNumber64 seq)
{
  NS_LOG_FUNCTION (this << p << seq);

  uint32_t pktSize = p->GetSize ();
  SequenceNumber64 headSeq = seq;
  SequenceNumber64 tailSeq = headSeq + SequenceNumber64 (pktSize);
  NS_LOG_LOGIC ("Add pkt " << p << " len=" << pktSize << " seq=" << headSeq << " tailseq=" << tailSeq
                           << ", when NextRxSeq=" << m_nextRxSeq << ", buffsize=" << m_size << ",maxbuffer=" << m_maxBuffer);

  // Trim packet to fit Rx window specification
  //if (headSeq < m_nextRxSeq) headSeq = m_nextRxSeq;
  //if (m_data.size ())
  //  {
  //    SequenceNumber64 maxSeq = m_data.begin ()->first + SequenceNumber64 (m_maxBuffer);
  //    if (maxSeq < tailSeq) tailSeq = maxSeq;
  //    if (tailSeq < headSeq) headSeq = tailSeq;
  //  }
  // Remove overlapped bytes from packet
  BufIterator i = m_data.begin ();

  if(headSeq < m_nextRxSeq)return false;
  while (i != m_data.end () && i->first <= tailSeq)
    {
      SequenceNumber64 lastByteSeq = i->first + SequenceNumber64 (i->second->GetSize ());
      if (lastByteSeq > headSeq)
        {
          if (i->first > headSeq && lastByteSeq < tailSeq)
            { // Rare case: Existing packet is embedded fully in the new packet
              m_size -= i->second->GetSize ();
              m_data.erase (i++);
              continue;
            }
          if (i->first <= headSeq)
            { // Incoming head is overlapped
              headSeq = lastByteSeq;
            }
          if (lastByteSeq >= tailSeq)
            { // Incoming tail is overlapped
              tailSeq = i->first;
            }
        }
      ++i;
    }
  NS_LOG_LOGIC ("Headseq " << headSeq << " tailseq " << tailSeq);

  // We now know how much we are going to store, trim the packet
  if (headSeq >= tailSeq)
    {
      NS_LOG_LOGIC ("Nothing to buffer");
      return false; // Nothing to buffer anyway
    }
  else
    {
      uint64_t start = headSeq - seq;
      uint64_t length = tailSeq - headSeq;
      p = p->CreateFragment (start, length);
      NS_ASSERT (length == p->GetSize ());
   }
  // Insert packet into buffer
	/*  for(BufIterator iter = m_data.begin(); iter != m_data.end(); iter++)
	  {
		  NS_LOG_LOGIC("Sequence="<<iter->first);
	  }*/
  NS_ASSERT (m_data.find (headSeq) == m_data.end ()); // Shouldn't be there yet
  m_data [ headSeq ] = p;
  NS_LOG_LOGIC ("Buffered packet of seqno=" << headSeq << " len=" << p->GetSize ());
  // Update variables
  m_size += p->GetSize ();      // Occupancy
  for (BufIterator i = m_data.begin (); i != m_data.end (); ++i)
    {
      if (i->first < m_nextRxSeq)
        {
          continue;
        }
      else if (i->first > m_nextRxSeq)
        {
          break;
        };
      m_nextRxSeq = i->first + SequenceNumber64 (i->second->GetSize ());
      //m_nextRxSeq = i->first + SequenceNumber64 (p->GetSize ());
      m_availBytes += i->second->GetSize ();
    }
  NS_LOG_LOGIC ("Updated buffer occupancy=" << m_size << " nextRxSeq=" << m_nextRxSeq);
  if (m_gotFin && m_nextRxSeq == m_finSeq)
    { // Account for the FIN packet
      ++m_nextRxSeq;
    };
  return true;
}

Ptr<Packet>
TcpRxBuffer64::Extract (uint64_t maxSize)
{
  NS_LOG_FUNCTION (this << maxSize);

  uint64_t extractSize = std::min (maxSize, m_availBytes);
  NS_LOG_LOGIC ("Requested to extract " << extractSize << " bytes from TcpRxBuffer64 of size=" << m_size);
  if (extractSize == 0) return 0;  // No contiguous block to return
  NS_ASSERT (m_data.size ()); // At least we have something to extract
  Ptr<Packet> outPkt = Create<Packet> (); // The packet that contains all the data to return
  BufIterator i;
  while (extractSize)
    { // Check the buffered data for delivery
      i = m_data.begin ();
      NS_ASSERT (i->first <= m_nextRxSeq); // in-sequence data expected
      // Check if we send the whole pkt or just a partial
      uint32_t pktSize = i->second->GetSize ();
      if (pktSize <= extractSize)
        { // Whole packet is extracted
          outPkt->AddAtEnd (i->second);
          m_data.erase (i);
          m_size -= pktSize;
          m_availBytes -= pktSize;
          extractSize -= pktSize;
        }
      else
        { // Partial is extracted and done
          outPkt->AddAtEnd (i->second->CreateFragment (0, extractSize));
          m_data[i->first + SequenceNumber64 (extractSize)] = i->second->CreateFragment (extractSize, pktSize - extractSize);
          m_data.erase (i);
          m_size -= extractSize;
          m_availBytes -= extractSize;
          extractSize = 0;
        }
    }
  if (outPkt->GetSize () == 0)
    {
      NS_LOG_LOGIC ("Nothing extracted.");
      return 0;
    }
  NS_LOG_LOGIC ("Extracted " << outPkt->GetSize ( ) << " bytes, bufsize=" << m_size
                             << ", num pkts in buffer=" << m_data.size ());
  return outPkt;
}

} //namepsace ns3
