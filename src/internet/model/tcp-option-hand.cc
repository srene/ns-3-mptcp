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

#include "tcp-option-hand.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("TcpOptionHand");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpOptionHand);

TcpOptionHand::TcpOptionHand ()
{
}

TcpOptionHand::~TcpOptionHand ()
{
}

TypeId
TcpOptionHand::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOptionHand")
    .SetParent<TcpOption> ()
  ;
  return tid;
}

TypeId
TcpOptionHand::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
TcpOptionHand::Print (std::ostream &os)
{


}

uint32_t
TcpOptionHand::GetSerializedSize (void) const
{
  return 8;
}

void
TcpOptionHand::Serialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  i.WriteU8 (32); // Kind
  i.WriteU8 (8); // Length
  uint8_t addr[6];
  m_address.CopyTo(addr);
  for(int j=0;j<6;j++){
 	  i.WriteU8(addr[j]);
  }
} 

uint32_t
TcpOptionHand::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t size = i.ReadU8 ();
  NS_ASSERT (size == 8);
  uint8_t addr[6];
  for(int j=0;j<6;j++){
	  addr[j] = i.ReadU8();
  }
  m_address.CopyFrom(addr);
  return 8;
}

uint8_t
TcpOptionHand::GetKind (void) const
{
  return 32;
}

void
TcpOptionHand::SetMacAddress (Mac48Address mac)
{
	m_address = mac;
}

Mac48Address
TcpOptionHand::GetMacAddress (void)
{
	return m_address;
}
} // namespace ns3
