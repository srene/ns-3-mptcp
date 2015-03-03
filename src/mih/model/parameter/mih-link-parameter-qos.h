/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012
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
 * Author: Sergi Reñé <sergi.rene@entel.upc.edu>
 */

#ifndef MIHLINKPARAMETERQOS_H_
#define MIHLINKPARAMETERQOS_H_


#include <stdint.h>
#include "ns3/mih-choice-helper.h"
#include "mih-link-parameter-type.h"

namespace ns3 {
  namespace mih {
    class LinkParameterQoS : public LinkParameterType {
    public:
      enum Type {
        INVALID = 0xff,
		MAXIMUM_NUMBER_COS = 0,
		MINIMUM_PACKET_TRANSFER_DELAY = 1,
		AVERAGE_PACKET_TRANSFER_DELAY = 2,
		PACKET_TRANSFER_DELAY_JITTER = 3,
		PACKET_TRANSFER_LOSS_RATE = 4,
      };
      LinkParameterQoS (uint8_t paramterCode = LinkParameterQoS::INVALID);
      LinkParameterQoS (LinkParameterQoS const &o);
      uint8_t GetParameterCode (void);
      CHOICE_HELPER_HEADER (LinkParameterQoS, LinkParameterType);
      virtual uint32_t GetTlvSerializedSize (void) const;
      virtual void Print (std::ostream &os) const;
      virtual void TlvSerialize (Buffer &buffer) const;
      virtual uint32_t TlvDeserialize (Buffer &buffer);
      TLV_TYPE_HELPER_HEADER (LinkParameterQoS);
    protected:
      uint8_t m_parameterCode;
    };
  } // namespace mih
} // namespace ns3


#endif /* MIHLINKPARAMETERQOS_H_ */
