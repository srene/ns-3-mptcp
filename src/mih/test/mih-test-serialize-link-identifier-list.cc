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

#include <iostream>
#include "ns3/mih-module.h"
#include "ns3/ipv6-address.h"
#include "ns3/mac48-address.h"

using namespace ns3::mih;
using namespace ns3;

int 
main (int argc, char *argv[])
{
  Packet::EnablePrinting ();
  Packet::EnableChecking ();

  MihHeader mihHeader;
  MihHeader mihHeader2;
  Buffer buffer;
  Buffer buffer2;
  uint32_t itemN = 9;
  Mac48Address m_addresses[itemN]; 
  m_addresses[0] = ("00:00:00:00:00:82");
  m_addresses[1] = ("00:00:00:00:00:83");
  m_addresses[2] = ("00:00:00:00:00:84");
  m_addresses[3] = ("00:00:00:00:00:85");
  m_addresses[4] = ("00:00:00:00:00:86");
  m_addresses[5] = ("00:00:00:00:00:87");
  m_addresses[6] = ("00:00:00:00:00:88");
  m_addresses[7] = ("00:00:00:00:00:89");
  m_addresses[8] = ("00:00:00:00:00:90");
  Ipv6Address prefixes[itemN];
  prefixes[0] = Ipv6Address ("2002:1:1::");
  prefixes[1] = Ipv6Address ("2003:1:2::");
  prefixes[2] = Ipv6Address ("2004:1:3::");
  prefixes[3] = Ipv6Address ("2005:1:4::");
  prefixes[4] = Ipv6Address ("2006:1:5::");
  prefixes[5] = Ipv6Address ("2007:1:6::");
  prefixes[6] = Ipv6Address ("2008:1:7::");
  prefixes[7] = Ipv6Address ("2009:1:8::");
  prefixes[8] = Ipv6Address ("2010:1:9::");

  LinkIdentifierList links;
  LinkIdentifierList links2;

  for (uint32_t j = 0; j < itemN; j++)
    {
      links.push_back (Create<LinkIdentifier> (LinkType (LinkType::WIRELESS_802_11),
					       (Ipv6Address::MakeAutoconfiguredAddress(m_addresses[j], prefixes[itemN - j - 1])),
					       (Ipv6Address::MakeAutoconfiguredAddress(m_addresses[itemN - j - 1], prefixes[j]))
					       ));
    }

   links.push_back (Create<LinkIdentifier> ());

  TlvSerialize (buffer2, links);
  TlvMih::PrintBuffer (std::cout, buffer2);
  //  std::cout << links << std::endl;
  TlvDeserialize (buffer2, links2);
  std::cout << links2 << std::endl;

  MihfId mihfid (""),
    mihfid1 ("provide@ns3");
  Buffer mihfidBuffer;
  mihfid.TlvSerialize (mihfidBuffer);
  mihfid1.TlvSerialize (mihfidBuffer);
  MihfId mihfid2, 
    mihfid3;
  mihfid2.TlvDeserialize (mihfidBuffer);
  mihfid2.Print (std::cout);
  mihfid3.TlvDeserialize (mihfidBuffer);
  mihfid3.Print (std::cout);

  RegistrationCode regCode1 (RegistrationCode::REGISTRATION),
    regCode2 (RegistrationCode::RE_REGISTRATION),
    regCode3 (RegistrationCode::REGISTRATION);
  Buffer bufferRegCode;
  regCode1.TlvSerialize (bufferRegCode);
  regCode2.TlvSerialize (bufferRegCode);
  regCode3.TlvSerialize (bufferRegCode);
  for (uint32_t j = 0; j < 3; j++)
    {
      RegistrationCode regCode;
      regCode.TlvDeserialize (bufferRegCode);
      regCode.Print (std::cout);
      std::cout << std::endl;
    }

  Status status1 (Status::AUTHORIZATION_FAILURE),
    status2 (Status::REJECTED);
  uint32_t validLifeTime1 = 6000,
    validLifeTime2 = 8000;
  Buffer bufferStatus;
  // TLV-Serialize;
  status1.TlvSerialize (bufferStatus);
  TlvMih::PrintBuffer (std::cout, bufferStatus);
  TlvMih::SerializeU32 (bufferStatus, validLifeTime1, TLV_VALID_TIME_INTERVAL);
  status2.TlvSerialize (bufferStatus);
  TlvMih::SerializeU32 (bufferStatus, validLifeTime2, TLV_VALID_TIME_INTERVAL);
  TlvMih::PrintBuffer (std::cout, bufferStatus);
  // TLV-Deserialize;
  Status status3, status4;
  uint32_t validLifeTime3, validLifeTime4;
  status3.TlvDeserialize (bufferStatus);
  TlvMih::DeserializeU32 (bufferStatus, validLifeTime3, TLV_VALID_TIME_INTERVAL);
  status4.TlvDeserialize (bufferStatus);
  TlvMih::DeserializeU32 (bufferStatus, validLifeTime4, TLV_VALID_TIME_INTERVAL);
  status3.Print (std::cout);
  status4.Print (std::cout);
  std::cout << "Valid Life Time = " << validLifeTime3 << std::endl;
  std::cout << "Valid Life Time = " << validLifeTime4 << std::endl;

  // NetworkTypeAddressList TLV-Serialization;
  std::cout << "TESTING NETWORK TYPE ADDRESS LIST TLV-SERIALIZATION" << std::endl;
  NetworkTypeAddressList networkTypes;
  NetworkTypeAddressList networkTypes2;

  for (uint32_t j = 0; j < itemN; j++)
    {
      networkTypes.push_back (Create<NetworkTypeAddress> (LinkType (LinkType::WIRELESS_802_11),
                                                          m_addresses[j]));
    }
  Buffer bufferNetworkTypes;
  TlvSerialize (bufferNetworkTypes, networkTypes);
  TlvMih::PrintBuffer (std::cout, bufferNetworkTypes);
  //   std::cout << links << std::endl;
  TlvDeserialize (bufferNetworkTypes, networkTypes2);
  std::cout << networkTypes2 << std::endl;

  // MakeBeforeBreakSupport TLV-Serialization;
  std::cout << "TESTING MBB LIST TLV-SERIALIZATION" << std::endl;
  Buffer mbbBufferList,
    mbbBuffer;
  MakeBeforeBreakSupportList mbbList1, 
    mbbList2;
  mbbList1.push_back (Create<MakeBeforeBreakSupport> (LinkType (LinkType::WIRELESS_802_11), LinkType (LinkType::WIRELESS_802_11), true));
  mbbList1.push_back (Create<MakeBeforeBreakSupport> (LinkType (LinkType::WIRELESS_802_11), LinkType (LinkType::WIRELESS_802_16), false));
  TlvSerialize (mbbBufferList, mbbList1);
  TlvMih::PrintBuffer (std::cout, mbbBufferList);
  TlvDeserialize (mbbBufferList, mbbList2);
  std::cout << mbbList2 << std::endl;

  Ptr<MakeBeforeBreakSupport> mbb = Create<MakeBeforeBreakSupport> (LinkType (LinkType::WIRELESS_802_11), LinkType (LinkType::WIRELESS_802_11), true);
  mbb->TlvSerialize (mbbBuffer);

  mbb = Create<MakeBeforeBreakSupport> (LinkType (LinkType::WIRELESS_802_11), LinkType (LinkType::WIRELESS_802_16), false);
  mbb->TlvSerialize (mbbBuffer);
  TlvMih::PrintBuffer (std::cout, mbbBuffer);
  MakeBeforeBreakSupport mbs, mbs2;
  mbs.TlvDeserialize (mbbBuffer);
  std::cout << mbs << std::endl;
  mbs2.TlvDeserialize (mbbBuffer);
  std::cout << mbs2 << std::endl;

  std::cout << "TESTING EMPTY MBB LIST TLV-SERIALIZATION" << std::endl;
  Buffer emptyBufferList;
  MakeBeforeBreakSupportList emptyMbbList1,
    emptyMbbList2;
  TlvSerialize (emptyBufferList, emptyMbbList1);
  TlvMih::PrintBuffer (std::cout, emptyBufferList);
  TlvDeserialize (emptyBufferList, emptyMbbList2);
  std::cout << emptyMbbList2 << std::endl;

  std::cout << "TESTING LINK PARAMETER REPORT TLV-SERIALIZATION" << std::endl;

  Threshold threshold (10,
                       Threshold::ABOVE_THRESHOLD);
  Ptr<LinkParameterType> parameter80211 =
    Create<LinkParameter80211> (LinkParameter80211::BEACON_CHANNEL_RSSI);
  Ptr<ParameterValue> parameterValue =
    Create<LinkParameterValue> (20);
  Ptr<LinkParameter> parameter = 
    Create<LinkParameter> (parameter80211,
                           parameterValue);
  
  LinkParameterReport parameterReport1 (threshold,
                                        parameter);

  LinkParameterReport parameterReport (Threshold (20, Threshold::BELOW_THRESHOLD),
                                       Create<LinkParameter> (Create<LinkParameter80211> (LinkParameter80211::MULTICAST_PACKET_LOSS_RATE),
                                                              Create<LinkParameterValue> (40)));
  LinkParameterReportList reports, reports2;
  reports.push_back (Create<LinkParameterReport> (parameterReport1));
  reports.push_back (Create<LinkParameterReport> (parameterReport));
  Buffer parameterReportBuffer;
  for (uint32_t j = 0; j < 10; j++)
    {
      TlvSerialize (parameterReportBuffer, reports);
    }
  TlvMih::PrintBuffer (std::cout, parameterReportBuffer);
  for (uint32_t i = 0; i < 9; i++)
    {
      TlvDeserialize (parameterReportBuffer, reports2);
      TlvMih::PrintBuffer (std::cout, parameterReportBuffer);
      for (uint32_t j= 0; j < reports2.size (); j++)
        {
          reports2[j]->Print (std::cout);
          std::cout << std::endl;
        }
    }
  TlvMih::PrintBuffer (std::cout, parameterReportBuffer);
  std::cout << std::endl;
  return 0;
}
