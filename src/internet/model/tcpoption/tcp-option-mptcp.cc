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

#include "tcp-option-mptcp.h"
#include "ns3/log.h"


NS_LOG_COMPONENT_DEFINE ("TcpOptionMptcp");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionMptcp);

TcpOptionMptcp::TcpOptionMptcp ():m_version(1)
{
}

TcpOptionMptcp::~TcpOptionMptcp ()
{
}

TypeId
TcpOptionMptcp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMptcp")
    .SetParent<TcpOption> ()
  ;
  return tid;
}

TypeId
TcpOptionMptcp::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


void
TcpOptionMptcp::Print (std::ostream &os)
{
	//for (ScoreBoard::iterator i = m_sb.begin (); i != m_sb.end (); ++i)
	//    {
			//NS_LOG_LOGIC(i->first << " " << i->second);
	//		os << i->first << " " << i->second;
	//    }

}

void
TcpOptionMptcp::Serialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i.WriteU8 (30); // Kind
  i.WriteU8 (m_length); // Length
  m_subOption->Serialize(i);
  //i.WriteHtonU32 (m_recvToken); // Local timestamp
  //i.WriteHtonU32 (m_senderRandNumber); // Echo timestamp

}

void
TcpOptionMptcp::AddSubOption(Ptr<TcpSubOption> subOption)
{
	m_subOption = subOption;
	m_length = m_subOption->GetSerializedSize();
}
uint32_t
TcpOptionMptcp::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  //uint8_t addr[6];
  uint8_t size = i.ReadU8 ();
  //NS_LOG_FUNCTION("size " << (uint32_t) size);
  //NS_ASSERT (size == 2);
  uint8_t field = i.ReadU8 ();
 // NS_LOG_FUNCTION("field " << (uint32_t) field);

  m_version = field & 0x0F;
  uint8_t m_subtype = field>>4;  //m_addressId = i.ReadU8();
  NS_LOG_LOGIC("Create suboption kind " << (uint32_t)m_subtype);
  m_subOption = TcpSubOption::CreateSubOption (m_subtype);
  m_subOption->Deserialize (i);
  return size;
}
uint8_t
TcpOptionMptcp::GetKind (void) const
{
  return 30;
}

uint32_t
TcpOptionMptcp::GetSerializedSize (void) const
{
  return m_subOption->GetSerializedSize();
}

uint8_t
TcpOptionMptcp::GetSubtype (void) const
{
  return m_subOption->GetSubType();
}
Ptr<TcpSubOption>
TcpOptionMptcp::GetSubOption(void)
{
  return m_subOption;
}


} // namespace ns3
