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


#include "mih-link-parameter-qos.h"

namespace ns3 {
  namespace mih {
    LinkParameterQoS::LinkParameterQoS (uint8_t paramterCode) :
	  m_parameterCode (paramterCode)
    {
      SimulationSingleton<TlvObjectFactory<LinkParameterType> >::Get ()->AddTlvTypeConstructor (TLV_LINK_PARAMETER_QOS,
                                                                                                MakeCallback (&LinkParameterQoS::Create));
    }
    LinkParameterQoS::LinkParameterQoS (LinkParameterQoS const &o) :
	  m_parameterCode (o.m_parameterCode)
    {}
    uint8_t
    LinkParameterQoS::GetParameterCode (void)
    {
      return m_parameterCode;
    }
    CHOICE_HELPER_IMPLEM (LinkParameterQoS, LinkParameterType)
    TLV_TYPE_HELPER_IMPLEM (LinkParameterQoS, TLV_LINK_PARAMETER_QOS)
    uint32_t
    LinkParameterQoS::GetTlvSerializedSize (void) const
    {
      return TlvMih::GetSerializedSizeU8 ();
    }
    void
    LinkParameterQoS::Print (std::ostream &os) const
    {
      std::string codestr = " (INVALID) ";
      switch (m_parameterCode)
        {
        case LinkParameterQoS::MAXIMUM_NUMBER_COS :
          {
            codestr = " (MAXIMUM_NUMBER_COS) ";
            break;
          }
        case LinkParameterQoS::MINIMUM_PACKET_TRANSFER_DELAY :
          {
            codestr = " (MINIMUM_PACKET_TRANSFER_DELAY) ";
            break;
          }
        case LinkParameterQoS::AVERAGE_PACKET_TRANSFER_DELAY :
          {
            codestr = " (AVERAGE_PACKET_TRANSFER_DELAY) ";
            break;
          }
        case LinkParameterQoS::PACKET_TRANSFER_DELAY_JITTER :
          {
            codestr = " (PACKET_TRANSFER_DELAY_JITTER) ";
            break;
          }
        case LinkParameterQoS::PACKET_TRANSFER_LOSS_RATE :
          {
            codestr = " (PACKET_TRANSFER_LOSS_RATE) ";
            break;
          }
        default:
          {
            codestr = " (INVALID) ";
            break;
          }
        }
      os << "Link Parameter QoS = "
         << std::hex << (int)m_parameterCode << codestr;
    }
    void
    LinkParameterQoS::TlvSerialize (Buffer &buffer) const
    {
      TlvMih::SerializeU8 (buffer, m_parameterCode, TLV_LINK_PARAMETER_QOS);
    }
    uint32_t
    LinkParameterQoS::TlvDeserialize (Buffer &buffer)
    {
      return TlvMih::DeserializeU8 (buffer, m_parameterCode, TLV_LINK_PARAMETER_QOS);
    }
  } // namespace mih
} // namespace ns3
