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

#ifndef   	WIFI_MIH_LINK_SAP_H
#define   	WIFI_MIH_LINK_SAP_H

#include "ns3/nstime.h"
#include "ns3/random-variable.h"
#include "ns3/event-id.h"
#include "mihf-id.h"
#include "mih-link-sap.h"
#include "ns3/wifi-net-device.h"
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
#include "ns3/stat-watch.h"
#include "ns3/throughput-watch.h"


namespace ns3 {
  namespace mih {
    class WifiMihLinkSap : public MihLinkSap {
    public:
      static TypeId GetTypeId (void);
      WifiMihLinkSap (void);
      virtual ~WifiMihLinkSap (void);
      virtual LinkType GetLinkType (void);
      void SetLinkType (LinkType linkType);
      virtual Address GetLinkAddress (void);
      void SetLinkAddress (Address addr);
      virtual LinkIdentifier GetLinkIdentifier (void);
      virtual void SetLinkIdentifier (LinkIdentifier linkIdentifier);
      void SetWifiNetDevice (Ptr<NetDevice> netDevice);
      virtual Address GetPoAAddress (void);
      virtual void SetPoAAddress (Address addr);
      
      virtual LinkCapabilityDiscoverConfirm CapabilityDiscover (void);
      virtual LinkGetParametersConfirm GetParameters (LinkParameterTypeList linkParametersRequest, 
					      LinkStatesRequest linkStatesRequest, 
					      LinkDescriptorsRequest descriptors);
      virtual LinkConfigureThresholdsConfirm ConfigureThresholds (LinkConfigurationParameterList configureParameters);
      virtual EventId Action (LinkAction action, 
                              uint64_t executionDelay, 
                              Address poaLinkAddress,
                              LinkActionConfirmCallback actionConfirmCb);

      //void Run (void);
      void SetNode(Ptr<Node>);
      //void Stop (void);
      void SetLIThreshold(double threshold);
      void SetLCThreshold(double threshold);
      void SetTcp(Ptr<TcpSocket> tcp);
      double GetLIThreshold(void) const;
      double GetLCThreshold(void) const;
      void SetMih(bool mih);

    protected:
      typedef void (WifiMihLinkSap::*ActionTrigger) (void);
      void TriggerLinkDetected (void);
      void TriggerLinkUp (void);
      void TriggerLinkDown (void);
      void DevTxTrace (Ptr<const Packet> p);
      void DevRxTrace (Ptr<const Packet> p, uint16_t packetType);
      void PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, enum WifiPreamble preamble);
      void PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double snr);
      void PhyTxTrace (Ptr<const Packet> packet,WifiMode mode);
      void AssocTrace (Mac48Address addr, Mac48Address mac);
      void DeAssocTrace (Mac48Address addr);
      void TriggerLinkHOImminent (Ptr<const Packet> packet);
      void PhyStateTrace (std::string context, Time start, Time duration, enum WifiPhy::State state);
      void RxDrop (Ptr<const Packet> p);
      void DoAction (LinkAction action,
                     Address poaLinkAddress,
                     LinkActionConfirmCallback actionConfirmCb);
      void SniffRxEvent (Ptr<const Packet> packet, uint16_t channelFreqMhz, uint16_t channelNumber, uint32_t rate, bool isShortPreamble, double signalDbm, double noiseDbm);
      virtual void DoDispose (void);
      virtual Ptr<DeviceStatesResponse> GetDeviceStates (void);
      LinkIdentifier m_linkIdentifier;
      Ptr<WifiNetDevice> m_wifiNetDevice;
      EventId m_nextEventId;
      MihfId m_mihfId;
  	  StatWatch m_macDelayWatch;
  	  ThroughputWatch m_apThroughputWatchDown;
  	  Average<double> m_throughDown;
  	  ThroughputWatch m_apThroughputWatchUp;
  	  Average<double> m_throughUp;
	  Mac48Address addr,addr2;
	  TracedValue<uint32_t>  m_nSta;
	  TracedValue<uint32_t>  m_macDelay;
	  TracedValue<uint32_t>  m_apThroughputDown;
	  TracedValue<uint32_t>  m_apThroughputUp;
	  bool m_handover;
	  bool m_mih;
	  double m_signal;
	  Ptr<Node> m_node;
      //static bool g_verbose;
	  double   m_liThresholdW;
	  double   m_lcThresholdW;
	  TracedCallback<Ptr<TcpSocket>,double>  m_rate;
	  Ptr<TcpSocket> m_tcp;
    };
  } // namespace mih
} // namespace ns3

#endif 	    /* !WIFI_MIH_LINK_SAP_H */
