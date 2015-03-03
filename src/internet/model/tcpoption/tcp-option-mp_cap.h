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

#ifndef TCP_OPTION_MPCAP_H
#define TCP_OPTION_MPCAP_H

#include "tcp-suboption.h"

namespace ns3 {

/**
 * Defines the TCP option of kind 0 (end of option list) as in RFC793
 */

class TcpOptionMpCapable : public TcpSubOption
{
public:
  TcpOptionMpCapable ();
  virtual ~TcpOptionMpCapable ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream &os);
  virtual void Serialize (Buffer::Iterator start);
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint8_t GetSubType (void) const ; // Get the `kind' (as in RFC793) of this option
  virtual uint32_t GetSerializedSize (void) const;

  void SetSenderKey(uint64_t key);
  void SetReceiverKey(uint64_t key);
  uint64_t GetSenderKey(void);
  uint64_t GetReceiverKey(void);

protected:
  uint8_t m_version;
  uint8_t m_flags;
  uint64_t m_senderKey;
  uint64_t m_recvKey;

};

} // namespace ns3

#endif /* TCP_OPTION_MPTCP_H */
