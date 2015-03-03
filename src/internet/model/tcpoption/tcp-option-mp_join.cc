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

#include "tcp-option-mp_join.h"
#include "ns3/log.h"


NS_LOG_COMPONENT_DEFINE ("TcpOptionMpJoin");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpJoin);

TcpOptionMpJoin::TcpOptionMpJoin ()
{
}

TcpOptionMpJoin::~TcpOptionMpJoin ()
{
}

TypeId
TcpOptionMpJoin::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpJoin")
    .SetParent<TcpOptionMptcp> ()
  ;
  return tid;
}

TypeId
TcpOptionMpJoin::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionMpJoin::Print (std::ostream &os)
{
	//for (ScoreBoard::iterator i = m_sb.begin (); i != m_sb.end (); ++i)
	//    {
			//NS_LOG_LOGIC(i->first << " " << i->second);
	//		os << i->first << " " << i->second;
	//    }

}

void
TcpOptionMpJoin::Serialize (Buffer::Iterator start)
{
  /*Buffer::Iterator i = start;
  i.WriteU8 (30); // Kind
  i.WriteU8 (m_length); // Length
  i.WriteU8 (m_addressId);
  i.WriteHtonU32 (m_recvToken); // Local timestamp
  i.WriteHtonU32 (m_senderRandNumber); // Echo timestamp*/

}

uint32_t
TcpOptionMpJoin::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  //uint8_t addr[6];
  uint8_t size = i.ReadU8 ();
  /*NS_ASSERT (size == m_length);
  m_subtype = i.ReadU8();
  m_addressId = i.ReadU8();
  m_recvToken = i.ReadNtohU32 ();
  m_senderRandNumber = i.ReadNtohU32 ();*/

  return size;
}

void
TcpOptionMpJoin::SetAddressId (uint8_t addressId)
{
  m_addressId = addressId;
}

void
TcpOptionMpJoin::SetRecvToken (uint32_t recvToken)
{
  m_recvToken = recvToken;
}

void
TcpOptionMpJoin::SetSenderRandNumber (uint32_t senderRandNumber)
{
  m_senderRandNumber = senderRandNumber;
}

} // namespace ns3
