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

#include "tcp-option-sack.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("TcpOptionSack");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionSack);

TcpOptionSack::TcpOptionSack ()
{
}

TcpOptionSack::~TcpOptionSack ()
{
}

TypeId
TcpOptionSack::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionSack")
    .SetParent<TcpOption> ()
  ;
  return tid;
}

TypeId
TcpOptionSack::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionSack::Print (std::ostream &os)
{
	for (ScoreBoard::iterator i = m_sb.begin (); i != m_sb.end (); ++i)
	    {
			//NS_LOG_LOGIC(i->first << " " << i->second);
			os << i->first << " " << i->second;
	    }

}

uint32_t
TcpOptionSack::GetSerializedSize (void) const
{
  return 2+8*m_sb.size();
}

void
TcpOptionSack::Serialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i.WriteU8 (5); // Kind
  i.WriteU8 (2+8*m_sb.size()); // Length
	for (ScoreBoard::iterator op = m_sb.begin (); op != m_sb.end (); ++op) {
		   i.WriteHtonU32 (op->first.GetValue()); // Local timestamp
		   i.WriteHtonU32 (op->second.GetValue()); // Echo timestamp
	}

} 

uint32_t
TcpOptionSack::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t size = i.ReadU8 ();
  //NS_ASSERT (size == 10);

  for (uint8_t j = 2;j< size;j=j+8)
    {

	  	  SequenceNumber32 sle = SequenceNumber32(i.ReadNtohU32());
	  	  SequenceNumber32 rle = SequenceNumber32(i.ReadNtohU32());
          m_sb.push_back(SackBlock(sle,rle));

    }


  return size;
}

uint8_t
TcpOptionSack::GetKind (void) const
{
  return 5;
}

void
TcpOptionSack::AddSack (SackBlock s)
{
	  m_sb.push_back(s);
}

uint32_t
TcpOptionSack::SackCount (void) const
{
  return m_sb.size ();
}

void
TcpOptionSack::ClearSack (void)
{
  m_sb.clear ();
}

SackBlock
TcpOptionSack::GetSack (int offset)
{
  ScoreBoard::iterator i = m_sb.begin ();
  while (offset--) ++i;
  return *i;
}

} // namespace ns3
