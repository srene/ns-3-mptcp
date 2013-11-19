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

#ifndef TCP_OPTION_HAND_H
#define TCP_OPTION_HAND_H

#include "tcp-option.h"
#include "ns3/sequence-number.h"
#include "ns3/mac48-address.h"

namespace ns3 {


/**
 * Defines the TCP option of kind 5 (Selective Acknowledgement) as in RFC2018
 */

class TcpOptionHand : public TcpOption
{
public:
  TcpOptionHand ();
  virtual ~TcpOptionHand ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream &os) ;
  virtual void Serialize (Buffer::Iterator start);
  virtual uint32_t Deserialize (Buffer::Iterator start);

  virtual uint8_t GetKind (void) const;
  virtual uint32_t GetSerializedSize (void) const;

  Mac48Address GetMacAddress();
  void SetMacAddress (Mac48Address mac);

protected:
  Mac48Address m_address;
};


} // namespace ns3

#endif /* TCP_OPTION_HAND_H */
