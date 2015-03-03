/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */            
/*
 * Copyright (c) 2008 IT-SUDPARIS
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
 * Author: Providence SALUMU M. <Providence.Salumu_Munga@it-sudparis.eu>
 */

#ifndef   	MIH_LINK_STATES_RESPONSE_H
#define   	MIH_LINK_STATES_RESPONSE_H

#include <vector>
#include "ns3/ptr.h"
#include "ns3/ref-count-base.h"
#include "mih-choice-helper.h"
#include "mih-tlv.h"

namespace ns3 {
  namespace mih {
    class LinkStatesResponse : public RefCountBase, public TlvMih {
    public:
      TLV_TYPE_HELPER_HEADER (LinkStatesResponse);
      CHOICE_HELPER_PURE_VIRTUAL_HEADER (LinkStatesResponse);
      virtual uint32_t GetTlvSerializedSize (void) const;
      virtual void Print (std::ostream &os) const;
      virtual void TlvSerialize (Buffer &buffer) const;
      virtual uint32_t TlvDeserialize (Buffer &buffer);
    protected:
    };
    typedef std::vector<Ptr<LinkStatesResponse> > LinkStatesResponseList;
    typedef LinkStatesResponseList::iterator LinkStatesResponseListI;
    uint32_t GetTlvSerializedSize (const LinkStatesResponseList &linkStatesResponseList);
    void TlvSerialize (Buffer &buffer, const LinkStatesResponseList &linkStatesResponseList);
    uint32_t TlvDeserialize (Buffer &buffer, LinkStatesResponseList &linkStatesResponseList);
  } // namespace mih
} // namespace ns3

#endif 	    /* !MIH_LINK_STATES_RESPONSE_H */
