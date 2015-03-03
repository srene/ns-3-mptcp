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
  LinkIdentifier linkIdentifier;
  LinkIdentifier newLinkIdentifier;
  Mac48Address m_addresses[2]; 
  m_addresses[0] = ("00:00:00:00:00:82");
  m_addresses[1] = ("00:00:00:00:00:88");
  Ipv6Address prefix0 ("2002:1:1::");
  Ipv6Address prefix1 ("3303:1:1::");
  Ptr<Packet> p;
  const uint8_t *packetData;
  uint32_t packetPayloadSize;

  //   Fill the buffer;
  linkIdentifier = LinkIdentifier (LinkType (LinkType::WIRELESS_802_11),
                                   (Ipv6Address::MakeAutoconfiguredAddress(m_addresses[0], prefix0)),
                                   (Ipv6Address::MakeAutoconfiguredAddress(m_addresses[1], prefix1)));
  std::cout << "TLV-Serializing the LinkIdentifier ..." << std::endl;
  std::cout << linkIdentifier << std::endl;
  linkIdentifier.TlvSerialize (buffer);
  //   PrintTlvBuffer (std::cout, buffer);
  //  Create the packet;
  std::cout << "Creating one empty packet ..." << std::endl;
  std::cout << "Filling LinkIdentifier in the packet payload ..." << std::endl;
  p = Create<Packet> (buffer.PeekData (), buffer.GetSize ());
  std::cout << "Filling MihHeader at start of the packet ..." << std::endl;
  mihHeader.SetVersion (MihHeader::VERSION_ONE);
  mihHeader.SetServiceId (MihHeader::COMMAND);
  mihHeader.SetOpCode (MihHeader::REQUEST);
  mihHeader.SetActionId (MihHeader::MIH_NET_HO_COMMIT);
  mihHeader.SetTransactionId (125);
  mihHeader.SetPayloadLength (buffer.GetSize ());
  p->AddHeader (mihHeader);
  std::cout << "Printing the created packet ..." << std::endl;
  p->Print (std::cout);
  std::cout << std::endl;
  std::cout << "Removing MihHeader from packet ..." << std::endl;
  p->RemoveHeader (mihHeader2);
  std::cout << "Printing the created packet ..." << std::endl;
  p->Print (std::cout);
  std::cout << std::endl;
  std::cout << "Recovering LinkIdentifier from the packet payload ..." << std::endl;
  packetPayloadSize = p->GetSize ();
  packetData = p->PeekData ();
  buffer2.AddAtStart (packetPayloadSize);
  Buffer::Iterator i = buffer2.Begin ();
  for (uint32_t j = 0; j < packetPayloadSize; j++, packetData++)
    {
      i.WriteU8 (*packetData);
    }
  newLinkIdentifier.TlvDeserialize (buffer2);
  std::cout << newLinkIdentifier << std::endl;
  std::cout << "Serialization is working fine! Now ready to transmit MIH packets troughout the network ..." << std::endl;
  return 0;
}

