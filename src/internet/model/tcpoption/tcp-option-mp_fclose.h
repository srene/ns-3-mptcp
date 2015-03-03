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

#ifndef TCP_OPTION_MPFCLOSE_H
#define TCP_OPTION_MPFCLOSE_H

#include "tcp-option-mptcp.h"

namespace ns3 {

/**
 * Defines the TCP option of kind 0 (end of option list) as in RFC793
 */

class TcpOptionMpFClose : public TcpOptionMptcp
{
public:
  TcpOptionMpFClose ();
  virtual ~TcpOptionMpFClose ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream &os);
  virtual void Serialize (Buffer::Iterator start) = 0;
  virtual uint32_t Deserialize (Buffer::Iterator start) = 0;

  virtual uint8_t GetKind (void) const;
  virtual uint32_t GetSerializedSize (void) const;
protected:
  uint8_t m_length;
  uint8_t m_subKind;

};

} // namespace ns3

#endif /* TCP_OPTION_MPTCP_H */