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
 * Author: Sergi Reñé <sergi.rene@entel.upc.edu>
 */

#ifndef TCPMIHUSER_H_
#define TCPMIHUSER_H_

#include "ns3/application.h"
#include "mih-callbacks.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

namespace ns3 {
  namespace mih {
  typedef Callback<void, Mac48Address, Mac48Address> AssociateCallback;
  //typedef Callback<void, Mac48Address> DeAssociateCallback;


	class TcpMihUser : public SimpleMihUser {
	public:
	    static TypeId GetTypeId (void);
	    TcpMihUser();
		virtual ~TcpMihUser();
		void SetDownLink(bool down);
	    virtual void LinkGetParameters (MihfId destMihf, LinkIdentifier destLinkIdentifier);
		virtual void RecvLinkHandoverImminent (MihfId sourceIdentifier,
							 LinkIdentifier oldLinkIdentifier,
							 LinkIdentifier newLinkIdentifier,
							 Address oldAR,
							 Address newAR);
		virtual void RecvLinkHandoverComplete (MihfId sourceIdentifier,
							 LinkIdentifier oldLinkIdentifier,
							 LinkIdentifier newLinkIdentifier,
							 Address oldAR,
							 Address newAR,
							 Status status);
		void SetTcp(Ptr<TcpSocket> tcp);
		void SetSink(Ptr<PacketSink> sink);
		Ptr<PacketSink> m_sink;
		Ptr<TcpSocket> m_tcp;
	    //void SetHandover(AssociateCallback callback, DeAssociateCallback deAssociateCallback);
	    void SetHandover(AssociateCallback callback);
	protected:
		void Freeze(Mac48Address mac);
		void Unfreeze (Mac48Address addr, Mac48Address dst);
	    void GetParameter (uint32_t delay, uint32_t n, uint32_t throughput);
		void ConfirmLinkGetParameters (MihfId sourceIdentifier,
								 Status status,
								 DeviceStatesResponseList deviceStates,
								 LinkStatusResponseList linkStatusResponseList);
		AssociateCallback m_AssociateCallback;
		//DeAssociateCallback m_deAssociateCallback;
		bool m_down;
		Mac48Address m_apAddress;
	    TracedCallback<Ptr<TcpSocket>,Mac48Address> m_postHandover;
	    TracedCallback<Ptr<TcpSocket>,uint32_t,uint32_t,uint32_t,Mac48Address> m_linkParameterTrace;


	};
  } // namespace mih
} // namespace ns3

#endif /* TCPMIHUSER_H_ */
