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

#include "tcp-option-mp_cap.h"
#include "ns3/log.h"


NS_LOG_COMPONENT_DEFINE ("TcpOptionMpCapable");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpCapable);

TcpOptionMpCapable::TcpOptionMpCapable ()
 : m_version(1),
   m_flags (0),
   m_senderKey (0),
   m_recvKey (0)

{
}

TcpOptionMpCapable::~TcpOptionMpCapable ()
{
}

TypeId
TcpOptionMpCapable::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpCapable")
    .SetParent<TcpSubOption> ()
  ;
  return tid;
}

TypeId
TcpOptionMpCapable::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionMpCapable::Print (std::ostream &os)
{
	//for (ScoreBoard::iterator i = m_sb.begin (); i != m_sb.end (); ++i)
	//    {
			//NS_LOG_LOGIC(i->first << " " << i->second);
	//		os << i->first << " " << i->second;
	//    }

}

uint32_t
TcpOptionMpCapable::GetSerializedSize (void) const
{
  if(m_recvKey!=0)  return 20;
  else return 12;
}

void
TcpOptionMpCapable::Serialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t kind = 0;
  i.WriteU8 (kind << 4 | m_version); //reserved bits are all zero
  i.WriteU8(m_flags);
  i.WriteHtonU64 (m_senderKey); // Local timestamp
  if(m_recvKey!=0)i.WriteHtonU64 (m_recvKey); // Echo timestamp

}

/*uint8_t
TcpOptionMpCapable::GetLength() const
{
	return 20;
}*/

uint32_t
TcpOptionMpCapable::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t size;
  m_flags= i.ReadU8();
  m_senderKey = i.ReadNtohU64 ();
  //NS_LOG_FUNCTION("size " << i.GetSize());
  if(i.GetSize()==40){
	  m_recvKey = i.ReadNtohU64 ();
	  size=20;
  } else if (i.GetSize()==32){
	  size=12;
  }
  return size;
}

uint8_t
TcpOptionMpCapable::GetSubType (void) const
{
	return 0;
}

void
TcpOptionMpCapable::SetSenderKey(uint64_t key)
{
	m_senderKey = key;
}

void
TcpOptionMpCapable::SetReceiverKey(uint64_t key)
{
	m_recvKey = key;
}

uint64_t
TcpOptionMpCapable::GetSenderKey(void)
{
	return m_senderKey;
}

uint64_t
TcpOptionMpCapable::GetReceiverKey(void)
{
	return m_recvKey;
}

} // namespace ns3
