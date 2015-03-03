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

#include <iostream>
#include <algorithm>
#include <string.h>
#include "ns3/packet.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "tcp-tx-buffer64.h"

NS_LOG_COMPONENT_DEFINE ("TcpTxBuffer64");

namespace ns3 {

TypeId
TcpTxBuffer64::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpTxBuffer64")
    .SetParent<Object> ()
    .AddConstructor<TcpTxBuffer64> ()
    .AddTraceSource ("UnackSequence",
                     "First unacknowledged sequence number (SND.UNA)",
                     MakeTraceSourceAccessor (&TcpTxBuffer64::m_firstByteSeq))
    .AddTraceSource ("LastSequence",
	    			 "First unacknowledged sequence number (SND.UNA)",
				     MakeTraceSourceAccessor (&TcpTxBuffer64::m_lastByteSeq))
  ;
  return tid;
}

/* A user is supposed to create a TcpSocket through a factory. In TcpSocket,
 * there are attributes SndBufSize and RcvBufSize to control the default Tx and
 * Rx window sizes respectively, with default of 128 KiByte. The attribute
 * SndBufSize is passed to TcpTxBuffer64 by TcpSocketBase::SetSndBufSize() and in
 * turn, TcpTxBuffer64:SetMaxBufferSize(). Therefore, the m_maxBuffer value
 * initialized below is insignificant.
 */
TcpTxBuffer64::TcpTxBuffer64 (uint32_t n)
  : m_firstByteSeq (n), m_size (0), m_maxBuffer (32768)//, m_data (0)
{
	m_data.clear();
}

TcpTxBuffer64::~TcpTxBuffer64 (void)
{
	m_data.clear();
}

SequenceNumber64
TcpTxBuffer64::HeadSequence (void) const
{
  return m_firstByteSeq;
}

SequenceNumber64
TcpTxBuffer64::TailSequence (void) const
{

    //NS_LOG_FUNCTION(m_lastByteSeq);
	return m_lastByteSeq;
    //return m_firstByteSeq + SequenceNumber64 (m_size);
}

uint32_t
TcpTxBuffer64::Size (void) const
{
  return m_size;
}

uint32_t
TcpTxBuffer64::MaxBufferSize (void) const
{
  return m_maxBuffer;
}

void
TcpTxBuffer64::SetMaxBufferSize (uint32_t n)
{
  NS_LOG_FUNCTION(this<<n);
  m_maxBuffer = n;
}

uint32_t
TcpTxBuffer64::Available (void) const
{
  return m_maxBuffer - m_size;
}

bool
TcpTxBuffer64::Add (Ptr<Packet> p)
{

  NS_LOG_FUNCTION (this << p);
  NS_LOG_LOGIC ("Packet of size " << p->GetSize () << " appending to window starting at "
                                  << m_firstByteSeq << ", availSize="<< Available ());

  if (p->GetSize () <= Available ())
    {
      if (p->GetSize () > 0)
        {
    	  NS_LOG_LOGIC("Insert " << m_lastByteSeq << " " << p << " " << p->GetSize());
          m_data.insert (DataPair(m_lastByteSeq,p));
          m_size += p->GetSize ();
          m_lastByteSeq  += p->GetSize ();
          NS_LOG_LOGIC ("Updated size=" << m_size << ", lastSeq=" << m_firstByteSeq + SequenceNumber64 (m_size) << " or " << TailSequence());
        }

      for(BufIterator i = m_data.begin();i!=m_data.end();++i)
      {
    	  NS_LOG_LOGIC ("Seq=" << (*i).first << ", size=" << (*i).second->GetSize() << " " << (*i).second);
      }

      return true;
    }
  NS_LOG_LOGIC ("Rejected. Not enough room to buffer packet.");
  return false;
}


Ptr<Packet>
TcpTxBuffer64::CopyFromSequence (uint32_t numBytes, const SequenceNumber64& seq)
{
  NS_LOG_FUNCTION (this << numBytes << seq);

  BufIterator i = m_data.find(seq);

  /*for(BufIterator i = m_data.begin();i!=m_data.end();++i)
  {
	  NS_LOG_LOGIC ("Seq=" << (*i).first << ", size=" << (*i).second->GetSize() << " " << (*i).second);
  }*/

  Ptr<Packet> p;

  if(i!=m_data.end()){
	  p= i->second->Copy();
  } else {
	  i++;
	  while(i!=m_data.end()){

		  p= i->second->Copy();
		  NS_LOG_FUNCTION(p<<p->GetSize());
		  if(p->GetSize()>0)
			  break;
		  i++;
	  }
  }


  return p;


}

void
TcpTxBuffer64::SetHeadSequence (const SequenceNumber64& seq)
{
  NS_LOG_FUNCTION (this << seq);
  m_firstByteSeq = seq;
  m_lastByteSeq = seq;
}

void
TcpTxBuffer64::DiscardUpTo (const SequenceNumber64& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("current data size=" << m_size << ", headSeq=" << m_firstByteSeq << ", maxBuffer=" << m_maxBuffer
                                     << ", numPkts=" << m_data.size ());
  // Cases do not need to scan the buffer
  if (m_firstByteSeq >= seq) return;

  // Scan the buffer and discard packets
  uint64_t offset = seq - m_firstByteSeq.Get ();  // Number of bytes to remove
  NS_LOG_LOGIC ("Offset=" << offset);

  for(BufIterator i = m_data.begin ();i!=m_data.end();++i)
  {
	  //NS_LOG_FUNCTION (m_firstByteSeq << seq << i->first);
	  if(i->first<seq){
		  m_size-=(*i).second->GetSize();
		  m_firstByteSeq+=(*i).second->GetSize();
		  //NS_LOG_FUNCTION (m_firstByteSeq << m_size << (*i).second->GetSize());
		  m_data.erase(i);
	  }
	  else break;
  }
  m_firstByteSeq = seq;
  NS_LOG_LOGIC ("firstByteSeq=" << m_firstByteSeq);
  // Catching the case of ACKing a FIN
  if (m_size == 0)
    {
	  m_firstByteSeq = seq;
    }
  NS_LOG_LOGIC ("size=" << m_size << " headSeq=" << m_firstByteSeq << " maxBuffer=" << m_maxBuffer
                        <<" numPkts="<< m_data.size ());
  //NS_ASSERT (m_firstByteSeq == seq);
}

void
TcpTxBuffer64::Remove (const SequenceNumber64& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("current data size=" << m_size << ", headSeq=" << m_firstByteSeq << ", maxBuffer=" << m_maxBuffer
                                     << ", numPkts=" << m_data.size ());
  // Cases do not need to scan the buffer
  if (m_firstByteSeq > seq) return;

  BufIterator i = m_data.find(seq);
  if(i!=m_data.end())
  {
      if(m_firstByteSeq==seq)m_firstByteSeq+=(*i).second->GetSize();
      m_size-=(*i).second->GetSize();
	  m_data.erase(i);

  }
  // Catching the case of ACKing a FIN
  if (m_size == (uint32_t)0)
    {
      m_firstByteSeq = seq;
    }
  NS_LOG_LOGIC ("size=" << m_size << " headSeq=" << m_firstByteSeq << " maxBuffer=" << m_maxBuffer
                        <<" numPkts="<< m_data.size ());
  //NS_ASSERT (m_firstByteSeq == seq);
}
} // namepsace ns3
