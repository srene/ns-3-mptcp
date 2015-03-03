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

#ifndef TCPFREEZEMIHUSER_H_
#define TCPFREEZEMIHUSER_H_

#include "ns3/application.h"
#include "mih-callbacks.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

namespace ns3 {
  namespace mih {
  typedef Callback<void, Mac48Address, Mac48Address> AssociateCallback;
	class TcpFreezeMihUser : public SimpleMihUser {
	public:
	    static TypeId GetTypeId (void);
	    TcpFreezeMihUser();
		virtual ~TcpFreezeMihUser();

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
		void SetDownLink(bool down);
		Ptr<PacketSink> m_sink;
		Ptr<TcpSocket> m_tcp;
		void Unfreeze (Mac48Address addr, Mac48Address dst);
		//void Unfreeze ();
	    void SetHandover(AssociateCallback callback);
	protected:
		void Freeze();
		AssociateCallback m_AssociateCallback;
		bool m_down;
	};
  } // namespace mih
} // namespace ns3

#endif /* TCPFREEZEMIHUSER_H_ */
