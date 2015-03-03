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

#include "tcp-option-mp_fclose.h"
#include "ns3/log.h"


NS_LOG_COMPONENT_DEFINE ("TcpOptionMpFClose");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionMpFClose);

TcpOptionMpFClose::TcpOptionMpFClose ()
{
}

TcpOptionMpFClose::~TcpOptionMpFClose ()
{
}

TypeId
TcpOptionMpFClose::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMpFClose")
    .SetParent<TcpOptionMptcp> ()
  ;
  return tid;
}

TypeId
TcpOptionMpFClose::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionMpFClose::Print (std::ostream &os)
{
	//for (ScoreBoard::iterator i = m_sb.begin (); i != m_sb.end (); ++i)
	//    {
			//NS_LOG_LOGIC(i->first << " " << i->second);
	//		os << i->first << " " << i->second;
	//    }

}

uint32_t
TcpOptionMpFClose::GetSerializedSize (void) const
{
  return m_length;
}

/*void
TcpOptionMpFClose::Serialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i.WriteU8 (30); // Kind
  i.WriteU8 (m_length); // Length
  i.WriteU8 (m_subKind);
  i.WriteHtonU32 (m_clients); // Local timestamp
  i.WriteHtonU32 (m_throughput); // Echo timestamp
  i.WriteHtonU32 (m_delay); // Echo timestamp
  uint8_t addr[6];
  m_address.CopyTo(addr);
  for(int j=0;j<6;j++){
 	  i.WriteU8(addr[j]);
  }
} */

/*uint32_t
TcpOptionMpFClose::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  //uint8_t addr[6];
  uint8_t size = i.ReadU8 ();
  NS_ASSERT (size == m_length);
  m_subKind = i.ReadU8();
  m_clients = i.ReadNtohU32 ();
  m_throughput = i.ReadNtohU32 ();
  m_delay = i.ReadNtohU32 ();
  for(int j=0;j<6;j++){
	  addr[j] = i.ReadU8();
  }
  m_address.CopyFrom(addr);
  return size;
}*/

uint8_t
TcpOptionMpFClose::GetKind (void) const
{
  return 30;
}

} // namespace ns3
