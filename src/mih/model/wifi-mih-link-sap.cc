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
 * GNU General Public License forsimple more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sergi Reñé <sergi.rene@entel.upc.edu>
 */
#define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }

#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/mac48-address.h"
#include "wifi-mih-link-sap.h"
#include "mih-device-information.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-mac-header.h"
#include <string>
#include <fstream>
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/mih-link-parameter-qos.h"
#include "ns3/mih-link-parameter-80211.h"
#include "ns3/mih-link-parameter-value.h"


#include <iostream>

NS_LOG_COMPONENT_DEFINE ("WifiMihLinkSap");

namespace ns3 {
  namespace mih {

    NS_OBJECT_ENSURE_REGISTERED (WifiMihLinkSap);

    TypeId
    WifiMihLinkSap::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::mih::WifiMihLinkSap")
	.SetParent<MihLinkSap> ()
	.AddConstructor<WifiMihLinkSap> ()
	.AddAttribute ("MihfId", "Mih Function Identifier",
		       MihfIdValue (MihfId ("default-WifiMihLinkSap@ns3")),
		       MakeMihfIdAccessor (&WifiMihLinkSap::m_mihfId),
		       MakeMihfIdChecker ())
   .AddAttribute ("LinkHandoverImminentThreshold",
				  "The energy of a received signal should be higher than "
				  "this threshold (dbm) to send a link handover imminent.",
				  DoubleValue (-83.0),
				  MakeDoubleAccessor (&WifiMihLinkSap::SetLIThreshold,
									  &WifiMihLinkSap::GetLIThreshold),
				  MakeDoubleChecker<double> ())
   .AddAttribute ("CcaMode1Threshold",
				  "The energy of a received signal should be higher than "
				  "this threshold (dbm) to  send a link handover complete",
				  DoubleValue (-85.0),
				  MakeDoubleAccessor (&WifiMihLinkSap::SetLCThreshold,
									  &WifiMihLinkSap::GetLCThreshold),
		                      MakeDoubleChecker<double> ())
    .AddTraceSource ("StaConnected",
					 "The TCP connection's congestion window",
					 MakeTraceSourceAccessor (&WifiMihLinkSap::m_nSta))
	.AddTraceSource ("ThroughputDown",
					 "The TCP connection's congestion window",
					 MakeTraceSourceAccessor (&WifiMihLinkSap::m_apThroughputDown))
	.AddTraceSource ("ThroughputUp",
					 "The TCP connection's congestion window",
					 MakeTraceSourceAccessor (&WifiMihLinkSap::m_apThroughputUp))
    .AddTraceSource ("MacDelay",
				     "The TCP connection's congestion window",
				     MakeTraceSourceAccessor (&WifiMihLinkSap::m_macDelay))
    .AddTraceSource ("Rate",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&WifiMihLinkSap::m_rate))
	;
      return tid;
    }
    WifiMihLinkSap::WifiMihLinkSap (void) :
      m_linkIdentifier ()
    {
      NS_LOG_FUNCTION (this);
      m_handover = false;
      m_apThroughputWatchDown.set_alpha(0.1);
      m_apThroughputWatchDown.time_.set_alpha(1);
      m_apThroughputWatchDown.size_.set_alpha(1);
      m_apThroughputWatchUp.set_alpha(0.1);
      m_apThroughputWatchUp.time_.set_alpha(1);
      m_apThroughputWatchUp.size_.set_alpha(1);
      m_macDelayWatch.set_alpha(0.5);
      m_signal = -200;
    }
    WifiMihLinkSap::~WifiMihLinkSap (void)
    {
      NS_LOG_FUNCTION_NOARGS ();
    }
    void
    WifiMihLinkSap::SetLIThreshold(double threshold)
    {
    	m_liThresholdW = threshold;
    }
    void
    WifiMihLinkSap::SetLCThreshold(double threshold)
    {
    	m_lcThresholdW = threshold;
    }
    double
    WifiMihLinkSap::GetLIThreshold(void) const
    {
    	return m_liThresholdW;
    }
    double
    WifiMihLinkSap::GetLCThreshold(void) const
    {
    	return m_lcThresholdW;
    }
    void
    WifiMihLinkSap::DoDispose (void)
    {
      NS_LOG_FUNCTION (this);
      if (m_nextEventId.IsRunning ())
	{
	  Simulator::Cancel (m_nextEventId);
	}
      MihLinkSap::DoDispose ();
    }
    LinkType
    WifiMihLinkSap::GetLinkType (void)
    {
      NS_LOG_FUNCTION (this);
      return m_linkIdentifier.GetType ();
    }
    void
    WifiMihLinkSap::SetLinkType (LinkType linkType)
    {
      NS_LOG_FUNCTION (this);
      m_linkIdentifier.SetType (linkType);
    }
    Address
    WifiMihLinkSap::GetLinkAddress (void)
    {
      NS_LOG_FUNCTION (this);
      return m_linkIdentifier.GetDeviceLinkAddress ();
    }
    void
    WifiMihLinkSap::SetLinkAddress (Address addr)
    {
      NS_LOG_FUNCTION (this);
      m_linkIdentifier.SetDeviceLinkAddress (addr);
    }
    LinkIdentifier
    WifiMihLinkSap::GetLinkIdentifier (void)
    {
      NS_LOG_FUNCTION (this);
      return m_linkIdentifier;
    }
    void
    WifiMihLinkSap::SetLinkIdentifier (LinkIdentifier linkIdentifier)
    {
      NS_LOG_FUNCTION (this);
      m_linkIdentifier = linkIdentifier;
    }
    void
    WifiMihLinkSap::SetWifiNetDevice (Ptr<NetDevice> netDevice)
    {
      NS_LOG_FUNCTION (this);
      m_wifiNetDevice = netDevice->GetObject<WifiNetDevice> ();

      if (!m_wifiNetDevice->GetMac()->TraceConnectWithoutContext ("MacTx", MakeCallback (&WifiMihLinkSap::DevTxTrace,this)))
      {
        	  NS_LOG_INFO("Trace fail Mac Tx");
       }
      if (!m_wifiNetDevice->GetMac()->TraceConnectWithoutContext ("MacPromiscRx", MakeCallback (&WifiMihLinkSap::DevRxTrace,this)))
            {
              	  NS_LOG_INFO("Trace fail Mac Rx");
             }
      /*if (!m_wifiNetDevice->GetPhy()->TraceConnectWithoutContext ("PhyRxEnd", MakeCallback (&WifiMihLinkSap::TriggerLinkHOImminent,this)))
      {
        	  NS_LOG_INFO("Trace fail Phy Rx");
       }*/

      if (!m_wifiNetDevice->GetPhy ()->TraceConnectWithoutContext ("MonitorSnifferRx", MakeCallback (&WifiMihLinkSap::SniffRxEvent,this)))
      {
        	  NS_LOG_INFO("Trace fail Phy Sniffer");
      }

      if (!m_wifiNetDevice->GetPhy()->TraceConnectWithoutContext ("PhyTxBegin", MakeCallback (&WifiMihLinkSap::PhyTxTrace,this)))
      {
        	  NS_LOG_INFO("Trace fail Phy Tx");
       }

      if (!m_wifiNetDevice->GetMac()->TraceConnectWithoutContext ("Assoc", MakeCallback (&WifiMihLinkSap::AssocTrace,this)))
      {
        	  NS_LOG_INFO("Trace fail Assoc");
       }

      if (!m_wifiNetDevice->GetMac()->TraceConnectWithoutContext ("DeAssoc", MakeCallback (&WifiMihLinkSap::DeAssocTrace,this)))
      {
        	  NS_LOG_INFO("Trace fail DeAssoc");
       }





    }
    void
    WifiMihLinkSap::RxDrop (Ptr<const Packet> p)
    {
      NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
    }
    Address
    WifiMihLinkSap::GetPoAAddress (void)
    {
      NS_LOG_FUNCTION (this);
      return m_linkIdentifier.GetPoALinkAddress ();
    }
    void
    WifiMihLinkSap::SetPoAAddress (Address addr)
    {
      NS_LOG_FUNCTION (this);
      m_linkIdentifier.SetPoALinkAddress (addr);
    }
    LinkCapabilityDiscoverConfirm
    WifiMihLinkSap::CapabilityDiscover (void)
    {
      NS_LOG_FUNCTION (this);
      return LinkCapabilityDiscoverConfirm (Status::SUCCESS,
					    (EventList::LINK_DETECTED |
					     EventList::LINK_UP |
					     EventList::LINK_DOWN |
					     EventList::LINK_PARAMETERS_REPORT),
					    (MihCommandList::LINK_GET_PARAMETERS |
					     MihCommandList::LINK_CONFIGURE_THRESHOLDS));
    }
    LinkGetParametersConfirm
    WifiMihLinkSap::GetParameters (LinkParameterTypeList linkParametersRequest,
				     LinkStatesRequest linkStatesRequest,
				     LinkDescriptorsRequest descriptors)
    {
      NS_LOG_FUNCTION (this);

      LinkGetParametersConfirm linkGetParametersConfirm;
	  linkGetParametersConfirm = LinkGetParametersConfirm (Status::SUCCESS);
		//std::cout << "ParameterList = " << linkParametersRequest.size() << std::endl;
		for (LinkParameterTypeList::iterator it = linkParametersRequest.begin(); it!=linkParametersRequest.end(); ++it)
		  {
			Ptr<LinkParameterQoS> qosType = DynamicCast<LinkParameterQoS> (*it);
			Ptr<LinkParameter80211> wifiType = DynamicCast<LinkParameter80211> (*it);

			if(qosType){
				switch(qosType->GetParameterCode()){
					case (LinkParameterQoS::AVERAGE_PACKET_TRANSFER_DELAY):
					{
						Ptr<ParameterValue> value = Create<LinkParameterValue> (m_macDelayWatch.average());
						Ptr<LinkParameter> param = Create<LinkParameter>(qosType,value);
						linkGetParametersConfirm.AddLinkParameterItem(param);
						break;

					}
					default :
					{
						linkGetParametersConfirm = LinkGetParametersConfirm (Status::REJECTED);
						break;
					}
				}
			} else if (wifiType){
				switch(wifiType->GetParameterCode()){
					case (LinkParameter80211::CONNECTED_STA):
					{
						Ptr<ParameterValue> value = Create<LinkParameterValue> (m_wifiNetDevice->GetMac()->GetStaNumberConnected());
						Ptr<LinkParameter> param = Create<LinkParameter>(wifiType,value);
						linkGetParametersConfirm.AddLinkParameterItem(param);
						break;

					}
					case (LinkParameter80211::THROUGHPUT_DOWN):
					{
						Ptr<ParameterValue> value = Create<LinkParameterValue> (m_apThroughputWatchDown.average());
						Ptr<LinkParameter> param = Create<LinkParameter>(wifiType,value);
	    				//NS_LOG_INFO("Get parameters at " << Simulator::Now().GetMicroSeconds() << " with bps " << m_apThroughput << " " << m_apThroughputWatch.average() << " " << value->GetValue());
						linkGetParametersConfirm.AddLinkParameterItem(param);
						break;

					}
					case (LinkParameter80211::THROUGHPUT_UP):
					{
						Ptr<ParameterValue> value = Create<LinkParameterValue> (m_apThroughputWatchUp.average());
						Ptr<LinkParameter> param = Create<LinkParameter>(wifiType,value);
	    				//NS_LOG_INFO("Get parameters at " << Simulator::Now().GetMicroSeconds() << " with bps " << m_apThroughput << " " << m_apThroughputWatch.average() << " " << value->GetValue());
						linkGetParametersConfirm.AddLinkParameterItem(param);
						break;

					}
					default :
					{
						linkGetParametersConfirm = LinkGetParametersConfirm (Status::REJECTED);
						break;
					}
				}
			}

		  }
      return linkGetParametersConfirm;
    }
    LinkConfigureThresholdsConfirm
    WifiMihLinkSap::ConfigureThresholds (LinkConfigurationParameterList configureParameters)
    {
      NS_LOG_FUNCTION (this);
      //      NS_ASSERT (m_powerUp);
      return LinkConfigureThresholdsConfirm (Status::UNSPECIFIED_FAILURE);
    }
    EventId
    WifiMihLinkSap::Action (LinkAction action,
			      uint64_t executionDelay,
			      Address poaLinkAddress,
			      LinkActionConfirmCallback actionConfirmCb)
    {

          if (action.GetType () != LinkAction::LINK_POWER_UP)
            {
              return EventId ();
            }

      return Simulator::Schedule (MilliSeconds (executionDelay),
                                  &WifiMihLinkSap::DoAction,
                                  this,
                                  action,
                                  poaLinkAddress,
                                  actionConfirmCb);
    }
    void
    WifiMihLinkSap::DoAction (LinkAction action,
                                Address poaLinkAddress,
                                LinkActionConfirmCallback actionConfirmCb)
    {
      NS_LOG_FUNCTION (this);
      if (actionConfirmCb.IsNull ())
        {
          return;
        }
      if (action.GetType () == LinkAction::LINK_POWER_DOWN)
        {
          //Stop ();
          actionConfirmCb (Status (Status::SUCCESS),
                           ScanResponseList (),
                           LinkActionResponse::INCAPABLE,
                           GetLinkIdentifier ());
          return;

        }
      actionConfirmCb (Status (Status::SUCCESS),
                       ScanResponseList (),
                       LinkActionResponse::INCAPABLE,
                       GetLinkIdentifier ());
    }
    Ptr<DeviceStatesResponse>
    WifiMihLinkSap::GetDeviceStates (void)
    {
      NS_LOG_FUNCTION (this);
      Ptr<DeviceStatesResponse> dev = Create<DeviceInformation> (m_mihfId.PeekString());
      dev->SetLinkIdentifier(m_linkIdentifier);
      return dev;
    }

    void
    WifiMihLinkSap::TriggerLinkDetected (void)
    {
      NS_LOG_FUNCTION (this);
      LinkDetected (m_mihfId,
		    LinkDetectedInformationList ());
      //ScheduleNextTrigger ();
    }
    void
    WifiMihLinkSap::TriggerLinkUp (void)
    {
      NS_LOG_FUNCTION (this);
      LinkUp (m_mihfId,
	      GetLinkIdentifier (),
	      Mac48Address ("33:33:00:00:00:10"),
	      Mac48Address ("33:33:00:00:00:20"),
	      true,
	      MobilityManagementSupport (MobilityManagementSupport::MOBILE_IPV4_RFC3344));
      //ScheduleNextTrigger ();
    }
    void
    WifiMihLinkSap::TriggerLinkDown (void)
    {
      NS_LOG_FUNCTION (this);
      LinkDown (m_mihfId,
		GetLinkIdentifier (),
		Mac48Address ("33:33:00:00:00:10"),
		LinkDownReason (LinkDownReason::EXPLICIT_DISCONNECT));
      //ScheduleNextTrigger ();
    }

    void
    WifiMihLinkSap::DevTxTrace (Ptr<const Packet> p)
    {
    //    NS_LOG_FUNCTION (this);

    	  Ptr<Packet> copy = p->Copy ();
    	  WifiMacHeader wih;
    	  Ipv4Header iph;
    	  TcpHeader tcph;
    	  LlcSnapHeader llh;
    	  //std::cout << " TX p: " << Simulator::Now ().GetSeconds () << " " << *p << std::endl;
    	  copy->RemoveHeader (llh);
    	  if(llh.GetType()==Ipv4L3Protocol::PROT_NUMBER){
    		  copy->RemoveHeader (iph);
    		  if(iph.GetProtocol()==TcpL4Protocol::PROT_NUMBER){
    			  copy->RemoveHeader (tcph);
    		//	  NS_LOG_INFO("Packet transmitted at MAC " << Simulator::Now().GetMicroSeconds() << " with delay " << copy->GetSize());
    			  if(copy->GetSize()>0){
        			  m_apThroughputWatchDown.update(copy->GetSize()*8,Simulator::Now().GetMicroSeconds());
        			  m_apThroughputDown = m_apThroughputWatchDown.average();
    				  m_macDelayWatch.set_current(Simulator::Now().GetMicroSeconds());
    		//		  NS_LOG_INFO("Packet transmitted at MAC " << Simulator::Now().GetMicroSeconds() << " with delay " << copy->GetSize()<< " " <<(uint32_t)m_macDelayWatch.current());
    			  }


    		  }

    	  }

    }

    void
    WifiMihLinkSap::DevRxTrace (Ptr<const Packet> p, uint16_t packetType)
    {
      //  NS_LOG_FUNCTION (this);

    	  Ptr<Packet> copy;
    	  copy = p->Copy ();
    	  Ipv4Header iph;
    	  TcpHeader tcph;
    	  if(packetType==Ipv4L3Protocol::PROT_NUMBER){
    		  copy->RemoveHeader (iph);
    		  if(iph.GetProtocol()==TcpL4Protocol::PROT_NUMBER){
    			  copy->RemoveHeader (tcph);
    		//	  NS_LOG_INFO("Packet transmitted at MAC " << Simulator::Now().GetMicroSeconds() << " with delay " << copy->GetSize());
    			  if(copy->GetSize()>0)
    			  {
        			  m_apThroughputWatchUp.update(copy->GetSize()*8,Simulator::Now().GetMicroSeconds());
        			  m_apThroughputUp = m_apThroughputWatchUp.average();
    		//		  NS_LOG_INFO("Packet reveived at " << Simulator::Now().GetMicroSeconds() << " with bytes " << copy->GetSize() << " " << m_apThroughputWatchUp.average());
    			  }
    		  }
    	  }
    }

    void
    WifiMihLinkSap::PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, enum WifiPreamble preamble)
    {
          std::cout << "PHYRXOK mode=" << mode << " snr=" << snr << " " << *packet << std::endl;

    }
    void
    WifiMihLinkSap::PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double snr)
    {
          std::cout << "PHYRXERROR snr=" << snr << " " << *packet << std::endl;
    }


    void
    WifiMihLinkSap::PhyTxTrace (Ptr<const Packet> packet, WifiMode mode)
    {
//    	  NS_LOG_FUNCTION (this);
    	  Ptr<Packet> copy = packet->Copy ();
    	  WifiMacHeader wih;
    	  Ipv4Header iph;
    	  LlcSnapHeader llh;
    	  TcpHeader tcph;
    	  copy->RemoveHeader (wih);
  //  	  std::cout << " PHYTX p: " << Simulator::Now ().GetSeconds () << " " << *packet << " " << llh.GetType() << std::endl;
    	  if(wih.GetType()==WIFI_MAC_DATA){
        	  copy->RemoveHeader (llh);
    		  if(llh.GetType()==Ipv4L3Protocol::PROT_NUMBER){
    			  copy->RemoveHeader (iph);
    			  if(iph.GetProtocol()==TcpL4Protocol::PROT_NUMBER){
					  copy->RemoveHeader (tcph);
					  //std::cout << "PHYTX p: " << Simulator::Now().GetMicroSeconds() << " " << mode.GetDataRate() << std::endl;
	//				  NS_LOG_LOGIC("PHYTX p: " << Simulator::Now().GetMicroSeconds() << " " << (uint32_t)m_macDelayWatch.current());
					  m_macDelayWatch.update(Simulator::Now().GetMicroSeconds() - m_macDelayWatch.current());
					  m_macDelay = m_macDelayWatch.average();
					  //NS_LOG_LOGIC("PHYTX p: " << Simulator::Now().GetMicroSeconds() << " " << m_macDelayWatch.current() << " " <<  m_macDelayWatch.average() << " " << m_macDelay);
    			  }
    		  }
    	  }


    }

    void
    WifiMihLinkSap::AssocTrace (Mac48Address addr, Mac48Address mac)
    {
		  NS_LOG_INFO(" AssocTrace p: " << Simulator::Now().GetMicroSeconds() << " " << addr);
		  LinkHandoverComplete (m_mihfId,
				  GetLinkIdentifier (),
				  GetLinkIdentifier (),
				  m_wifiNetDevice->GetMac()->GetBssid(),
				  m_wifiNetDevice->GetMac()->GetBssid(),
		          Status::SUCCESS);
    }
    void
    WifiMihLinkSap::SetNode (Ptr<Node> node){
    	m_node = node;
    }
    void
    WifiMihLinkSap::DeAssocTrace (Mac48Address addr)
    {
		  NS_LOG_INFO(" DeAssocTrace p: " << Simulator::Now().GetMicroSeconds() << " " << addr);
    }
    void
    WifiMihLinkSap::TriggerLinkHOImminent (Ptr<const Packet> packet)
    {
    	  NS_LOG_FUNCTION (this << *packet );

    	  Ptr<Packet> copy = packet->Copy ();

    	  WifiMacHeader wih;
    	  copy->RemoveHeader(wih);

    	  /*if (wih.IsBeacon ()){
			  MgtBeaconHeader beacon;
			  copy->RemoveHeader (beacon);
			  //uint8_t buffer[6];

			  NS_LOG_INFO("BEACON RECEIVED p: " << Simulator::Now().GetMicroSeconds() << " " << wih.GetAddr2() << " " << m_wifiNetDevice->GetMac()->GetBssid() << " " << addr);
			  if (wih.GetAddr2()!= m_wifiNetDevice->GetMac()->GetBssid() && wih.GetAddr2()!= addr && wih.GetAddr2()!= addr2 && m_wifiNetDevice->GetMac()->GetBssid()!="00:00:00:00:00:00")
			  {*/
				  NS_LOG_INFO("BEACON RECEIVED p: " << addr << addr2);
				  addr = wih.GetAddr2();
				  addr2 = m_wifiNetDevice->GetMac()->GetBssid();
				  //wih.GetAddr2().CopyTo(buffer);
				  //addr.CopyFrom (buffer);
				  NS_LOG_INFO("BEACON RECEIVED p: " << addr);
				  LinkHandoverImminent (m_mihfId,
						  	  	  	    GetLinkIdentifier (),
						  	  	  	    GetLinkIdentifier (),
						  	  	  	    m_wifiNetDevice->GetMac()->GetBssid(),
				                        wih.GetAddr2());
		//	  }
    	//  }


    }
    void
    WifiMihLinkSap::PhyStateTrace (std::string context, Time start, Time duration, enum WifiPhy::State state)
    {
          std::cout << " state=" << state << " start=" << start << " duration=" << duration << std::endl;

    }


    /*void
    WifiMihLinkSap::Run (void)
    {
      NS_LOG_FUNCTION (this);
      //m_powerUp = true;
      //ScheduleNextTrigger ();
    }
    void
    WifiMihLinkSap::Stop (void)
    {
      NS_LOG_FUNCTION (this);
      //m_powerUp = false;
      if (m_nextEventId.IsRunning ())
      {
    	  Simulator::Cancel (m_nextEventId);
      }
    }*/
    void
    WifiMihLinkSap::SetTcp(Ptr<TcpSocket> tcp)
    {
    	m_tcp = tcp;
    }
    void
    WifiMihLinkSap::SetMih(bool mih)
    {
    	m_mih = mih;
    }
    void
    WifiMihLinkSap::SniffRxEvent (Ptr<const Packet> packet, uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate, bool isShortPreamble, double signalDbm, double noiseDbm)
    {
    	  Ptr<Packet> copy = packet->Copy ();

    	  WifiMacHeader wih;
    	  copy->RemoveHeader(wih);
  	  //NS_LOG_FUNCTION (this << *packet );

  	  //NS_LOG_FUNCTION ("signal " << m_signal << signalDbm);
  	  if (wih.IsData()&& !wih.GetAddr1().IsBroadcast()){
  		  //NS_LOG_INFO(" Sniff phy rx p: "  << Simulator::Now ().GetSeconds () << " " << wih.GetType() << " " << signalDbm << " " << noiseDbm << " " << channelNumber << " " << channelFreqMhz << " " << rate);
  		  m_rate(m_tcp,(double)rate / 2);
  	  }
	  if (wih.IsBeacon ()){
		  if(m_signal>signalDbm&&signalDbm<m_liThresholdW&&!m_handover){
			  NS_LOG_FUNCTION ("HANDOVER INMINENT");
			m_handover=true;
			TriggerLinkHOImminent(packet);
		  } else if(m_signal>signalDbm&&signalDbm<m_lcThresholdW){
			  NS_LOG_FUNCTION ("HANDOVER COMPLETE");
			m_handover=false;
			if(m_mih){
				if(channelNumber==1){
					m_wifiNetDevice->GetPhy()->SetChannelNumber(6);
				}else if(channelNumber==6){
					m_wifiNetDevice->GetPhy()->SetChannelNumber(1);
				}
			}
			else{
				m_wifiNetDevice->GetPhy()->SetChannelNumber((channelNumber+1)%11);
			}

			m_wifiNetDevice->GetMac()->GetObject<StaWifiMac>()->SetState(StaWifiMac::BEACON_MISSED);
			m_wifiNetDevice->GetMac()->GetObject<StaWifiMac>()->StartActiveAssociation();
			m_signal=-200;
			return;
		  }
		  m_signal = signalDbm;

	  }

    }

  } // namespace mih
} // namespace ns3
