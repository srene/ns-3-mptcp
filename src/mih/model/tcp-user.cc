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

#include "ns3/log.h"
#include "mih-function.h"
#include "simple-mih-user.h"
#include "ns3/mih-link-parameter-type.h"
#include "ns3/mih-link-parameter-qos.h"
#include "ns3/mih-link-parameter-value.h"
#include "ns3/mih-link-parameter-qos.h"
#include "ns3/mih-link-status-response.h"
#include "tcp-user.h"

NS_LOG_COMPONENT_DEFINE ("TcpUser");

namespace ns3 {
  namespace mih {

    //NS_OBJECT_ENSURE_REGISTERED (TcpUser);
    TypeId
    TcpUser::GetTypeId (void)
  	{
    static TypeId tid = TypeId ("ns3::TcpUser")
      .SetParent<SimpleMihUser> ()
      .AddConstructor<TcpUser> ()
      ;
      return tid;
    }
    TcpUser::TcpUser() {
		NS_LOG_FUNCTION_NOARGS ();
		m_down = true;
	}

    TcpUser::~TcpUser() {
		NS_LOG_FUNCTION_NOARGS ();
	}

	void
	TcpUser::SetTcp(Ptr<TcpSocket> tcp)
	{
		m_tcp = tcp;
	}

	void
	TcpUser::SetSink(Ptr<PacketSink> sink)
	{
		m_sink = sink;
	}

	void
	TcpUser::SetDownLink(bool down)
	{
		m_down = down;
	}

    void
    TcpUser::RecvLinkHandoverImminent (MihfId sourceIdentifier,
					     LinkIdentifier oldLinkIdentifier,
					     LinkIdentifier newLinkIdentifier,
					     Address oldAR,
					     Address newAR)
    {
      NS_LOG_FUNCTION (this);
      //Freeze();
    }

    void
    TcpUser::RecvLinkHandoverComplete (MihfId sourceIdentifier,
					     LinkIdentifier oldLinkIdentifier,
					     LinkIdentifier newLinkIdentifier,
					     Address oldAR,
					     Address newAR,
					     Status status)
    {
      NS_LOG_FUNCTION (this);
      NS_LOG_INFO ("Source Mihf Identifier = " << sourceIdentifier << ", from Old Link Identifier = " << oldLinkIdentifier <<  ", to New Link Identifier = " << newLinkIdentifier <<", from Old AR = " << oldAR << ", to New AR = " << newAR << ", Status = " << status);
      Unfreeze(Mac48Address::ConvertFrom(newLinkIdentifier.GetDeviceLinkAddress()),Mac48Address::ConvertFrom(newAR));
      //Simulator::Schedule (Seconds(1.5),&TcpUser::Unfreeze,this,Mac48Address::ConvertFrom(newLinkIdentifier.GetDeviceLinkAddress()),Mac48Address::ConvertFrom(newAR));
    }

    void TcpUser::Freeze ()
    {
        NS_LOG_FUNCTION (this);
          if(m_down){
    		  std::list<Ptr<Socket> > tcpList = m_sink->GetAcceptedSockets();
    		  for (std::list<Ptr<Socket> >::iterator i = tcpList.begin (); i != tcpList.end (); i++)
    		  {
    			  NS_LOG_LOGIC ("getSocket flow at time " <<  Simulator::Now ().GetSeconds ());
    			  Ptr<TcpSocket> tcp = (*i)->GetObject<TcpSocket>();
    			  tcp->SetPersist(true);
    		  }

          }else{
        	  //m_tcp->Freeze();
			  //tcp->SendEmptyPacket(TcpHeader::ACK);
		  }

    }

    void TcpUser::SetHandover(AssociateCallback callback)
    {
    	m_AssociateCallback = callback;
    }

    /*void TcpUser:: Unfreeze (void)
    {
          NS_LOG_FUNCTION (this);
    	  std::list<Ptr<Socket> > tcpList = m_sink->GetAcceptedSockets();
    	  for (std::list<Ptr<Socket> >::iterator i = tcpList.begin (); i != tcpList.end (); i++)
    	  {
    		  NS_LOG_LOGIC ("getSocket flow at time " <<  Simulator::Now ().GetSeconds ());
    		  Ptr<TcpSocket> tcp = (*i)->GetObject<TcpSocket>();
    		  tcp->SetPersist(false);
    		  tcp->TripleAck();

    	  }

    }*/

    void TcpUser:: Unfreeze (Mac48Address addr, Mac48Address dst)
    {
    	  m_AssociateCallback(addr,dst);
          NS_LOG_FUNCTION (this);



    }

  } // namespace mih
} // namespace ns3

