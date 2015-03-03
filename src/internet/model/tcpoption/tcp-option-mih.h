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

#ifndef TCP_OPTION_MIH_H
#define TCP_OPTION_MIH_H

#include "tcp-option.h"
#include "ns3/sequence-number.h"
#include "ns3/mac48-address.h"

namespace ns3 {

typedef std::pair<SequenceNumber32, SequenceNumber32> SackBlock;
typedef std::list<SackBlock> ScoreBoard;


class TcpOptionMih : public TcpOption
{
public:
  TcpOptionMih ();
  virtual ~TcpOptionMih ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream &os) ;
  virtual void Serialize (Buffer::Iterator start);
  virtual uint32_t Deserialize (Buffer::Iterator start);

  virtual uint8_t GetKind (void) const;
  virtual uint32_t GetSerializedSize (void) const;

  uint8_t GetDirection();
  uint32_t GetDelay();
  uint32_t GetThroughput();
  uint32_t GetClients();
  Mac48Address GetMacAddress();
  void SetHandover(uint8_t direct, uint32_t delay, uint32_t n, uint32_t throughput,Mac48Address addr);

protected:
  uint32_t m_clients;
  uint32_t m_throughput;
  uint32_t m_delay;
  Mac48Address m_address;
  uint8_t m_direct;

};

} // namespace ns3

#endif /* TCP_OPTION_MIH_H */
