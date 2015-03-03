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
#include "tcp-tx-buffer-wrapper.h"
#include "mp-tcp-socket.h"


NS_LOG_COMPONENT_DEFINE ("TcpTxBufferWrapper");

namespace ns3 {

TypeId
TcpTxBufferWrapper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpTxBufferWrapper")
    .SetParent<Object> ()
    .AddConstructor<TcpTxBufferWrapper> ()
    /*.AddTraceSource ("UnackSequence",
                     "First unacknowledged sequence number (SND.UNA)",
                     MakeTraceSourceAccessor (&TcpTxBufferWrapper::m_firstByteSeq))
	 .AddTraceSource ("Size",
					  "Buffer size",
					  MakeTraceSourceAccessor (&TcpTxBufferWrapper::m_size))*/
  ;
  return tid;
}

/* A user is supposed to create a TcpSocket through a factory. In TcpSocket,
 * there are attributes SndBufSize and RcvBufSize to control the default Tx and
 * Rx window sizes respectively, with default of 128 KiByte. The attribute
 * SndBufSize is passed to TcpTxBufferWrapper by TcpSocketBase::SetSndBufSize() and in
 * turn, TcpTxBufferWrapper:SetMaxBufferSize(). Therefore, the m_maxBuffer value
 * initialized below is insignificant.
 */
TcpTxBufferWrapper::TcpTxBufferWrapper (uint32_t n,Ptr<TcpSocketBase> tcpSocketBase)
  //: m_firstByteSeq (n), m_size (0), m_maxBuffer (32768), m_data (0)
{
	m_tcpSocketBase = tcpSocketBase;
}

TcpTxBufferWrapper::~TcpTxBufferWrapper (void)
{
}

SequenceNumber32
TcpTxBufferWrapper::HeadSequence (void) const
{
  NS_LOG_FUNCTION(this);
  if(!m_tcp->GetTcpMpCapable())
  {
	  return m_txBuffer.HeadSequence();
  } else {
	  //NS_LOG_FUNCTION(this<<m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->HeadSequence(m_tcpSocketBase));
	  return m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->HeadSequence(m_tcpSocketBase);
  }
}

SequenceNumber32
TcpTxBufferWrapper::TailSequence (void) const
{
  NS_LOG_FUNCTION(this);
  if(!m_tcp->GetTcpMpCapable())
  {
	  return m_txBuffer.TailSequence();
  } else {
	  //NS_LOG_FUNCTION(this<<m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->TailSequence(m_tcpSocketBase));
	  return m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->TailSequence(m_tcpSocketBase);
  }
  return SequenceNumber32(0);
}

uint32_t
TcpTxBufferWrapper::Size () const
{
  NS_LOG_FUNCTION(this);
  if(!m_tcp->GetTcpMpCapable())
  {
	  return m_txBuffer.Size();
  } else {
	  return m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->Size(m_tcpSocketBase);
  }
  return 0;
}

uint32_t
TcpTxBufferWrapper::MaxBufferSize (void) const
{
  NS_LOG_FUNCTION(this);
  if(!m_tcp->GetTcpMpCapable())
    {
  	  return m_txBuffer.MaxBufferSize();
    } else {
  	  return m_tcp->GetSndBufSize();
    }
  return 0;
}

void
TcpTxBufferWrapper::SetMaxBufferSize (uint32_t n)
{
  NS_LOG_FUNCTION(this<<n);
  if(!m_tcp->GetTcpMpCapable())
    {
  	  m_txBuffer.SetMaxBufferSize(n);
    } else {
  	  m_tcp->SetSndBufSize(n);
    }
}

uint32_t
TcpTxBufferWrapper::Available (void) const
{
  NS_LOG_FUNCTION(this);
  if(!m_tcp->GetTcpMpCapable())
  {
	  return m_txBuffer.Available();
  } else {
	  return m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->Available();
  }
  return 0;
}

bool
TcpTxBufferWrapper::Add (Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this<<p->GetSize());
  if(!m_tcp->GetTcpMpCapable())
  {
	  return m_txBuffer.Add(p);
  } else {
	  return 1;
  }
}

uint32_t
TcpTxBufferWrapper::SizeFromSequence (const SequenceNumber32& seq) const
{
  NS_LOG_FUNCTION(this << seq);
  if(!m_tcp->GetTcpMpCapable())
  {
	  return m_txBuffer.SizeFromSequence(seq);
  } else {
	  return m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->SizeFromSequence(m_tcpSocketBase,seq);;
  }
}

Ptr<Packet>
TcpTxBufferWrapper::CopyFromSequence (uint32_t numBytes, const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION(this << numBytes << seq);
  if(!m_tcp->GetTcpMpCapable())
  {
	  return m_txBuffer.CopyFromSequence(numBytes,seq);
  } else {
	  return m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->CopyFromSequence(m_tcpSocketBase,numBytes,seq);
  }
}

void
TcpTxBufferWrapper::SetHeadSequence (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  //NS_LOG_FUNCTION(this << m_maxBuffer << m_size);
  if(!m_tcp->GetTcpMpCapable())
  {
	  m_txBuffer.SetHeadSequence(seq);
  } else {
	  m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->SetHeadSequence(m_tcpSocketBase,seq);
  }
}

void
TcpTxBufferWrapper::DiscardUpTo (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  //NS_LOG_FUNCTION(this << m_maxBuffer << m_size);
  if(!m_tcp->GetTcpMpCapable())
  {
	  m_txBuffer.DiscardUpTo(seq);
  } else {
	  m_tcp->GetObject<MpTcpSocket>()->GetScheduler()->DiscardUpTo(m_tcpSocketBase,seq);
  }
}

void
TcpTxBufferWrapper::SetTcp (Ptr<TcpSocket> tcp)
{
	NS_LOG_FUNCTION (this << tcp);
	m_tcp = tcp;
	if(!m_tcp->GetTcpMpCapable())
	{
		m_txBuffer = TcpTxBuffer(0);
	}
}
} // namepsace ns3
