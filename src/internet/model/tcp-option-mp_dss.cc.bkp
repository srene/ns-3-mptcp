/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
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

#include "tcp-option-mp_dss.h"
#include "ns3/log.h"


NS_LOG_COMPONENT_DEFINE ("TcpOptionMpDss");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpDss);

TcpOptionMpDss::TcpOptionMpDss ()
: m_flags(0),
  m_dataAck64(0),
  m_dataAck32(0),
  m_dataSeq64(0),
  m_dataSeq32(0),
  m_subflowSeq(0),
  m_dataLength(0),
  m_checksum(0),
  m_size(0)
{
}

TcpOptionMpDss::~TcpOptionMpDss ()
{
}

TypeId
TcpOptionMpDss::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpDss")
    .SetParent<TcpOptionMptcp> ()
  ;
  return tid;
}

TypeId
TcpOptionMpDss::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionMpDss::Print (std::ostream &os)
{
	//for (ScoreBoard::iterator i = m_sb.begin (); i != m_sb.end (); ++i)
	//    {
			//NS_LOG_LOGIC(i->first << " " << i->second);
	//		os << i->first << " " << i->second;
	//    }

}
uint8_t
TcpOptionMpDss::GetSubType (void) const
{
	return 2;
}

uint32_t
TcpOptionMpDss::GetSerializedSize (void) const
{

  if((m_flags == ACK)||(m_flags == FINACK))
  {
	  return 12;
  }
  if((m_flags == ACK32)||(m_flags == FINACK32))
  {
	  return 8;
  }
  if((m_flags == SEQ)||(m_flags == FINSEQ))
  {
	  return 20;
  }
  if((m_flags == SEQ32)||(m_flags == FINSEQ32))
  {
	  return 16;
  }
  if((m_flags == SEQACK)||(m_flags == FINSEQACK))
  {
	  return 28;
  }

  if((m_flags == SEQACK32)||(m_flags == FINSEQACK32))
  {
	  return 20;
  }
  return 28;

}

void
TcpOptionMpDss::Serialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t kind = 2;
  i.WriteU8 (kind << 4 | 0); //reserved bits are all zero
  i.WriteU8(m_flags & 0x1F);
  //std::cout << "flags " << (uint32_t)(m_flags & 0x1F) << std::endl;
  if((m_flags == ACK)||(m_flags == FINACK))
  {
	  i.WriteHtonU64 (m_dataAck64.GetValue());
  }
  if((m_flags == ACK32)||(m_flags == FINACK32))
  {
	  i.WriteHtonU32 (m_dataAck32.GetValue());
  }
  if((m_flags == SEQ)||(m_flags == FINSEQ))
  {
	  i.WriteHtonU64 (m_dataSeq64.GetValue());
	  i.WriteHtonU32 (m_subflowSeq.GetValue());
	  i.WriteHtonU16 (m_dataLength);
	  i.WriteHtonU16 (0);
  }
  if((m_flags == SEQ32)||(m_flags == FINSEQ32))
  {
	  i.WriteHtonU32 (m_dataSeq32.GetValue());
	  i.WriteHtonU32 (m_subflowSeq.GetValue());
	  i.WriteHtonU16 (m_dataLength);
	  i.WriteHtonU16 (0);

  }
  if((m_flags == SEQACK)||(m_flags == FINSEQACK))
  {
	  i.WriteHtonU64 (m_dataAck64.GetValue());
	  i.WriteHtonU64 (m_dataSeq64.GetValue());
	  i.WriteHtonU32 (m_subflowSeq.GetValue());
	  i.WriteHtonU16 (m_dataLength);
	  i.WriteHtonU16 (0);
  }

  if((m_flags == SEQACK32)||(m_flags == FINSEQACK32))
  {
	  i.WriteHtonU32 (m_dataAck32.GetValue());
	  i.WriteHtonU32 (m_dataSeq32.GetValue());
	  i.WriteHtonU32 (m_subflowSeq.GetValue());
	  i.WriteHtonU16 (m_dataLength);
	  i.WriteHtonU16 (0);
  }
}

uint8_t
TcpOptionMpDss::GetFlags()
{
	return m_flags;
}

uint32_t
TcpOptionMpDss::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_flags= i.ReadU8();
  if((m_flags == ACK)||(m_flags == FINACK))
  {
	  m_dataAck64 = SequenceNumber64(i.ReadNtohU64());
  }
  if((m_flags == ACK32)||(m_flags == FINACK32))
  {
	  m_dataAck32 = SequenceNumber32(i.ReadNtohU32());
  }
  if((m_flags == SEQ)||(m_flags == FINSEQ))
  {
	  m_dataSeq64 = SequenceNumber64(i.ReadNtohU64());
	  m_subflowSeq = SequenceNumber32(i.ReadNtohU32());
	  m_dataLength = i.ReadNtohU16 ();
  }
  if((m_flags == SEQ32)||(m_flags == FINSEQ32))
  {
	  m_dataSeq32 = SequenceNumber32(i.ReadNtohU32());
	  m_subflowSeq = SequenceNumber32(i.ReadNtohU32());
	  m_dataLength = i.ReadNtohU16 ();

  }
  if((m_flags == SEQACK)||(m_flags == FINSEQACK))
  {
	  m_dataAck64 = SequenceNumber64(i.ReadNtohU64());
	  m_dataSeq64 = SequenceNumber64(i.ReadNtohU64());
	  m_subflowSeq = SequenceNumber32(i.ReadNtohU32());
	  m_dataLength = i.ReadNtohU16 ();
  }

  if((m_flags == SEQACK32)||(m_flags == FINSEQACK32))
  {
	  m_dataAck32 = SequenceNumber32(i.ReadNtohU32());
	  m_dataSeq32 = SequenceNumber32(i.ReadNtohU32());
	  m_subflowSeq = SequenceNumber32(i.ReadNtohU32());
	  m_dataLength = i.ReadNtohU16 ();
  }

  return GetSerializedSize();
}

void
TcpOptionMpDss::SetDataAcknowledgment64 (SequenceNumber64 ack)
{
	m_dataAck64 = ack;
	m_flags |= ACK;
	//std::cout << "flags " << (uint32_t)(m_flags & 0x1F) << std::endl;

}

SequenceNumber64
TcpOptionMpDss::GetDataAcknowledgment64 (void)
{
	return m_dataAck64;
}

void
TcpOptionMpDss::SetDataAcknowledgment32 (SequenceNumber32 ack)
{
	m_dataAck32 = ack;
	m_flags |= ACK32;
	//std::cout << "flags " << (uint32_t)(m_flags & 0x1F) << std::endl;


}

SequenceNumber32
TcpOptionMpDss::GetDataAcknowledgment32 (void)
{
	return m_dataAck32;
}

void
TcpOptionMpDss::SetDataSequence64 (SequenceNumber64 seq)
{
	m_dataSeq64 = seq;
	m_flags |= SEQ;
	//std::cout << "flags " << (uint32_t)(m_flags & 0x1F) << std::endl;


}

SequenceNumber64
TcpOptionMpDss::GetDataSequence64(void)
{
	return m_dataSeq64;

}

void
TcpOptionMpDss::SetDataSequence32 (SequenceNumber32 seq)
{
	m_dataSeq32 = seq;
	m_flags |= SEQ32;
	//std::cout << "flags " << (uint32_t)(m_flags & 0x1F) << std::endl;


}

SequenceNumber32
TcpOptionMpDss::GetDataSequence32 (void)
{
	return m_dataSeq32;
}

void
TcpOptionMpDss::SetSubflowSequence (SequenceNumber32 seq)
{
	m_subflowSeq = seq;
}

SequenceNumber32
TcpOptionMpDss::GetSubflowSequence (void)
{
	return m_subflowSeq;
}

void
TcpOptionMpDss::SetDataLength (uint16_t length)
{
	m_dataLength = length;
}

uint16_t
TcpOptionMpDss::GetDataLength (void)
{
	return m_dataLength;
}
void
TcpOptionMpDss::SetFin (void)
{
	m_flags |= 0x10;
;
}

} // namespace ns3
