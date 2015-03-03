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
#include "ns3/mih-link-parameter-80211.h"
#include "tcpmih-mih-user.h"

NS_LOG_COMPONENT_DEFINE ("TcpMihUser");

namespace ns3 {
  namespace mih {

    //NS_OBJECT_ENSURE_REGISTERED (TcpFreezeMihUser);
    TypeId
    TcpMihUser::GetTypeId (void)
  	{
    static TypeId tid = TypeId ("ns3::TcpMihUser")
      .SetParent<SimpleMihUser> ()
      .AddConstructor<TcpMihUser> ()
      .AddTraceSource ("linkParameter",
                        "ConfirmGetParameters trace",
                        MakeTraceSourceAccessor (&TcpMihUser::m_linkParameterTrace))
      .AddTraceSource ("postHandover",
                        "Post Handover",
                        MakeTraceSourceAccessor (&TcpMihUser::m_postHandover))
      ;
      return tid;
    }
	TcpMihUser::TcpMihUser() {
		NS_LOG_FUNCTION_NOARGS ();
		m_down = true;
		//this->TraceConnectWithoutContext("linkParameter", MakeCallback (&TcpMihUser::GetParameter,this));
	}

	void
	TcpMihUser::SetDownLink(bool down)
	{
		m_down = down;
	}
    void
    TcpMihUser::LinkGetParameters (MihfId destMihf, LinkIdentifier destLinkIdentifier)
    {
  	  NS_LOG_FUNCTION (this);

    Ptr<MihFunction> mihf = 0;
    mihf = GetObject<MihFunction> ();
    NS_ASSERT (mihf != 0);

    LinkIdentifierList links;
    links.push_back (Create<LinkIdentifier> (destLinkIdentifier));
    LinkStatusRequest linkStatusRequest = LinkStatusRequest();
    Ptr<LinkParameterQoS> linkParameter = Create<LinkParameterQoS>(LinkParameterQoS::AVERAGE_PACKET_TRANSFER_DELAY);
    Ptr<LinkParameter80211> linkParameter2 = Create<LinkParameter80211>(LinkParameter80211::CONNECTED_STA);
    Ptr<LinkParameter80211> linkParameter3;
    if(m_down)
        linkParameter3 = Create<LinkParameter80211>(LinkParameter80211::THROUGHPUT_DOWN);
    else
        linkParameter3 = Create<LinkParameter80211>(LinkParameter80211::THROUGHPUT_UP);

    linkStatusRequest.AddLinkParameterTypeItem(linkParameter);
    linkStatusRequest.AddLinkParameterTypeItem(linkParameter2);
    linkStatusRequest.AddLinkParameterTypeItem(linkParameter3);
    mihf->LinkGetParameters (destMihf,
			       DeviceStatesRequest (static_cast<uint16_t> (DeviceStatesRequest::DEVICE_INFO)),
			       links,
			       linkStatusRequest,
			       MakeCallback (&SimpleMihUser::ConfirmLinkGetParameters, this)
			       );
    }
	void
	TcpMihUser::ConfirmLinkGetParameters (MihfId sourceIdentifier,
						 Status status,
						 DeviceStatesResponseList deviceStates,
						 LinkStatusResponseList linkStatusResponseList)
	{
	  NS_LOG_FUNCTION (this);
	  NS_LOG_INFO ("Source Mihf Identifier = " << sourceIdentifier << ", Status = " << status << ", Response List = " << status.GetType());
	  uint32_t delay;
	  uint32_t n;
	  uint32_t throughputUp;
	  uint32_t throughputDown;
	  Mac48Address poa;
	  for (DeviceStatesResponseListI i = deviceStates.begin (); i != deviceStates.end(); i++){
		  Ptr<DeviceStatesResponse>  device = *i;
		  poa = Mac48Address::ConvertFrom(device->GetLinkIdentifier().GetPoALinkAddress());
		  NS_LOG_INFO("Device response " << device->GetLinkIdentifier());
	  }
	  for (LinkStatusResponseI i = linkStatusResponseList.begin (); i != linkStatusResponseList.end (); i++){
		  Ptr<LinkStatusResponse> response = *i;
		  for(LinkParameterListI j = response->GetLinkParameterListBegin(); j != response->GetLinkParameterListEnd (); j++){
			  Ptr<LinkParameter> linkParameter = *j;
			  Ptr<LinkParameterQoS> delaylinkParameterType = DynamicCast<LinkParameterQoS>(linkParameter->GetLinkParameterType());
			  Ptr<LinkParameter80211> nlinkParameterType = DynamicCast<LinkParameter80211>(linkParameter->GetLinkParameterType());
			  Ptr<LinkParameterValue> parameterValue = DynamicCast<LinkParameterValue>(linkParameter->GetParameterValue());
			  if(delaylinkParameterType && delaylinkParameterType->GetParameterCode()==LinkParameterQoS::AVERAGE_PACKET_TRANSFER_DELAY)
			  {
				  delay = parameterValue->GetValue();
			  } else if(nlinkParameterType && nlinkParameterType->GetParameterCode()==LinkParameter80211::CONNECTED_STA){
				  n = parameterValue->GetValue();
			  } else if(nlinkParameterType && nlinkParameterType->GetParameterCode()==LinkParameter80211::THROUGHPUT_DOWN){
				  throughputDown = parameterValue->GetValue();
			  } else if(nlinkParameterType && nlinkParameterType->GetParameterCode()==LinkParameter80211::THROUGHPUT_UP){
				  throughputUp = parameterValue->GetValue();
			  }
		 }

	  }
	  if(m_down){
		  //m_linkParameterTrace (delay,n,throughputDown,poa);
		  //GetParameter(delay, n, throughputDown);
		  std::list<Ptr<Socket> > tcpList = m_sink->GetAcceptedSockets();
		  for (std::list<Ptr<Socket> >::iterator i = tcpList.begin (); i != tcpList.end (); i++)
		  {
			  Ptr<TcpSocket> tcp = (*i)->GetObject<TcpSocket>();
			  NS_LOG_LOGIC ("getSocket flow at time " <<  Simulator::Now ().GetSeconds () << " tcp " << tcp->GetPort());
			  tcp->SetPersist(false);
			  tcp->SetHandover(0,delay,n,throughputDown,m_apAddress);
			  //m_linkParameterTrace (tcp,delay,n,throughputDown,poa);

		  }
		  //m_linkParameterTrace (m_sink,delay,n,throughputDown,poa);

	  } else {
	      //m_linkParameterTrace (m_tcp,delay,n,throughputUp,poa);
		  GetParameter (delay, n, throughputUp);
		  m_tcp->SetHandover(1,delay,n,throughputUp,m_apAddress);

	  }

	}

	TcpMihUser::~TcpMihUser() {
		NS_LOG_FUNCTION_NOARGS ();
	}

	void
	TcpMihUser::SetTcp(Ptr<TcpSocket> tcp)
	{
		m_tcp = tcp;
	}

	void
	TcpMihUser::SetSink(Ptr<PacketSink> sink)
	{
		m_sink = sink;
	}

    void
    TcpMihUser::RecvLinkHandoverImminent (MihfId sourceIdentifier,
					     LinkIdentifier oldLinkIdentifier,
					     LinkIdentifier newLinkIdentifier,
					     Address oldAR,
					     Address newAR)
    {
      NS_LOG_FUNCTION (this << sourceIdentifier << oldLinkIdentifier << newLinkIdentifier << oldAR << newAR);
      if(!m_down)m_postHandover(m_tcp,Mac48Address::ConvertFrom(newAR));
      Freeze(Mac48Address::ConvertFrom(newAR));
    }

    void
    TcpMihUser::RecvLinkHandoverComplete (MihfId sourceIdentifier,
					     LinkIdentifier oldLinkIdentifier,
					     LinkIdentifier newLinkIdentifier,
					     Address oldAR,
					     Address newAR,
					     Status status)
    {
      NS_LOG_FUNCTION (this);
      //NS_LOG_INFO ("Source Mihf Identifier = " << sourceIdentifier << ", from Old Link Identifier = " << oldLinkIdentifier <<  ", to New Link Identifier = " << newLinkIdentifier <<", from Old AR = " << oldAR << ", to New AR = " << newAR << ", Status = " << status);
      Unfreeze(Mac48Address::ConvertFrom(newLinkIdentifier.GetDeviceLinkAddress()),Mac48Address::ConvertFrom(newAR));
    }

    void TcpMihUser::Freeze (Mac48Address mac)
    {
        NS_LOG_FUNCTION (this);
        if(m_down){
  		  std::list<Ptr<Socket> > tcpList = m_sink->GetAcceptedSockets();
  		  for (std::list<Ptr<Socket> >::iterator i = tcpList.begin (); i != tcpList.end (); i++)
  		  {
  			  Ptr<TcpSocket> tcp = (*i)->GetObject<TcpSocket>();
			  NS_LOG_LOGIC ("getSocket flow at time " <<  Simulator::Now ().GetSeconds () << " tcp " << tcp->GetPort());
  			  tcp->SetPersist(true);
  			  tcp->SetHandOption(true,mac);
  			  //tcp->SendEmptyPacket(TcpHeader::ACK);
  		  }
        }else{
        	m_tcp->SetHandOption(true,mac);
        	m_tcp->Freeze();
        }

    }

    void TcpMihUser::SetHandover(AssociateCallback callback)
    //void TcpMihUser::SetHandover(AssociateCallback callback, DeAssociateCallback deAssociateCallback)
    {
    	m_AssociateCallback = callback;
    	//m_deAssociateCallback = deAssociateCallback;
    }


    void TcpMihUser:: Unfreeze (Mac48Address addr, Mac48Address dst)
    {
		if(m_down){
		  std::list<Ptr<Socket> > tcpList = m_sink->GetAcceptedSockets();
		  for (std::list<Ptr<Socket> >::iterator i = tcpList.begin (); i != tcpList.end (); i++)
		  {
			  Ptr<TcpSocket> tcp = (*i)->GetObject<TcpSocket>();
			  NS_LOG_LOGIC ("getSocket flow at time " <<  Simulator::Now ().GetSeconds () << " tcp " << tcp->GetPort());
			  tcp->SetPersist(false);
			  //tcp->SendEmptyPacket(TcpHeader::ACK);
		  }
		} else {
			m_tcp->Unfreeze();
		}
  		  m_AssociateCallback(addr,dst);
    	  m_apAddress = dst;
          NS_LOG_FUNCTION (this);

    }

    void TcpMihUser::GetParameter (uint32_t delay, uint32_t n, uint32_t throughput)
    {
          NS_LOG_FUNCTION (this << delay << n << throughput);
          /*if(m_down){

			  std::list<Ptr<Socket> > tcpList = m_sink->GetAcceptedSockets();
			  for (std::list<Ptr<Socket> >::iterator i = tcpList.begin (); i != tcpList.end (); i++)
			  {
				  NS_LOG_LOGIC ("getSocket flow at time " <<  Simulator::Now ().GetSeconds ());
				  Ptr<TcpSocket> tcp = (*i)->GetObject<TcpSocket>();
				  tcp->SetPersist(false);
				  tcp->SetHandover(delay,n,throughput,m_apAddress);

			  }
          }else{*/
          if(!m_down){
             m_tcp->Handover(delay,0,throughput);
          }


    }



  } // namespace mih
} // namespace ns3

