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

#ifndef TCP_SUBOPTION_H
#define TCP_SUBOPTION_H

#include <stdint.h>
#include "ns3/object.h"
#include "ns3/buffer.h"

namespace ns3 {

/**
 * Base class for all kinds of TCP options
 */

typedef enum{
	MPTCP_SUB_CAPABLE,
	MPTCP_SUB_JOIN,
	MPTCP_SUB_DSS,
	MPTCP_SUB_ADD_ADDR,
	MPTCP_SUB_FAIL,
	MPTCP_SUB_PRIO,
	MPTCP_SUB_RM_ADDR,
	MPTCP_SUB_FCLOSE
} Subtypes_t;
class TcpSubOption : public Object
{
public:
  TcpSubOption ();
  virtual ~TcpSubOption ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream &os) = 0;
  virtual void Serialize (Buffer::Iterator start) = 0;
  virtual uint32_t Deserialize (Buffer::Iterator start) = 0;

  virtual uint8_t GetSubType (void) const = 0; // Get the `kind' (as in RFC793) of this option
  virtual uint32_t GetSerializedSize (void) const = 0; // Get the total length of this option, >= 1
  //virtual uint8_t GetLength (void) const = 0;
  static Ptr<TcpSubOption> CreateSubOption (uint8_t subtype); // Factory method for all options
protected:
};

} // namespace ns3

#endif /* TCP_OPTION */
