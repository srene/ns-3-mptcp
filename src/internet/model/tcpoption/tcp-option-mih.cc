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

#include "tcp-option-mih.h"
#include "ns3/log.h"


NS_LOG_COMPONENT_DEFINE ("TcpOptionMih");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionMih);

TcpOptionMih::TcpOptionMih ()
{
}

TcpOptionMih::~TcpOptionMih ()
{
}

TypeId
TcpOptionMih::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionMih")
    .SetParent<TcpOption> ()
  ;
  return tid;
}

TypeId
TcpOptionMih::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionMih::Print (std::ostream &os)
{
	//for (ScoreBoard::iterator i = m_sb.begin (); i != m_sb.end (); ++i)
	//    {
			//NS_LOG_LOGIC(i->first << " " << i->second);
	//		os << i->first << " " << i->second;
	//    }

}

uint32_t
TcpOptionMih::GetSerializedSize (void) const
{
  return 21;
}

void
TcpOptionMih::Serialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i.WriteU8 (31); // Kind
  i.WriteU8 (21); // Length
  i.WriteU8 (m_direct);
  i.WriteHtonU32 (m_clients); // Local timestamp
  i.WriteHtonU32 (m_throughput); // Echo timestamp
  i.WriteHtonU32 (m_delay); // Echo timestamp
  uint8_t addr[6];
  m_address.CopyTo(addr);
  for(int j=0;j<6;j++){
 	  i.WriteU8(addr[j]);
  }
} 

uint32_t
TcpOptionMih::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t addr[6];
  uint8_t size = i.ReadU8 ();
  NS_ASSERT (size == 21);
  m_direct = i.ReadU8();
  m_clients = i.ReadNtohU32 ();
  m_throughput = i.ReadNtohU32 ();
  m_delay = i.ReadNtohU32 ();
  for(int j=0;j<6;j++){
	  addr[j] = i.ReadU8();
  }
  m_address.CopyFrom(addr);
  return size;
}

uint8_t
TcpOptionMih::GetKind (void) const
{
  return 31;
}

uint8_t
TcpOptionMih::GetDirection (void)
{
	return m_direct;
}

uint32_t
TcpOptionMih::GetClients ()
{
	return m_clients;
}

uint32_t
TcpOptionMih::GetThroughput ()
{
	return m_throughput;
}

uint32_t
TcpOptionMih::GetDelay ()
{
	return m_delay;
}

Mac48Address
TcpOptionMih::GetMacAddress ()
{
	return m_address;
}

void
TcpOptionMih::SetHandover (uint8_t direct, uint32_t delay, uint32_t n, uint32_t throughput, Mac48Address addr)
{
	m_delay = delay;
	m_throughput = throughput;
	m_clients = n;
	m_address = addr;
	m_direct = direct;
}
} // namespace ns3
