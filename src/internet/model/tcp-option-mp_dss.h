/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
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
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#ifndef TCP_OPTION_MPDSS_H
#define TCP_OPTION_MPDSS_H

#include "tcp-option-mptcp.h"
#include "ns3/sequence-number.h"


namespace ns3 {

/**
 * Defines the TCP option of kind 0 (end of option list) as in RFC793
 */

class TcpOptionMpDss : public TcpSubOption
{
public:
  TcpOptionMpDss ();
  virtual ~TcpOptionMpDss ();

  enum Flags
    {
      SEQ = 0x0C,
      SEQ32 = 0x04,
      ACK = 0x03,
      ACK32 = 0x01,
      SEQACK = 0x0F,
      SEQACK32 = 0x05,
      FINSEQ = 0x01C,
      FINSEQ32 = 0x014,
      FINACK = 0x013,
      FINACK32 = 0x011,
      FINSEQACK = 0x1F,
      FINSEQACK32 = 0x15
    };

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream &os);
  virtual void Serialize (Buffer::Iterator start);
  virtual uint32_t Deserialize (Buffer::Iterator start);

  void SetDataAcknowledgment64 (SequenceNumber64 ack);
  SequenceNumber64 GetDataAcknowledgment64 (void);
  void SetDataAcknowledgment32 (SequenceNumber32 ack);
  SequenceNumber32 GetDataAcknowledgment32 (void);
  void SetDataSequence64 (SequenceNumber64 seq);
  SequenceNumber64 GetDataSequence64(void);
  void SetDataSequence32 (SequenceNumber32 seq);
  SequenceNumber32 GetDataSequence32 (void);
  void SetSubflowSequence (SequenceNumber32 seq);
  SequenceNumber32 GetSubflowSequence (void);
  uint8_t GetFlags();
  void SetFin (void);
  uint16_t GetDataLength (void);
  void SetDataLength (uint16_t length);
  uint8_t GetSubType (void) const;
  uint32_t GetSerializedSize (void) const;
protected:

  uint8_t m_flags;
  SequenceNumber64 m_dataAck64;
  SequenceNumber32 m_dataAck32;
  SequenceNumber64 m_dataSeq64;
  SequenceNumber32 m_dataSeq32;
  SequenceNumber32 m_subflowSeq;

  uint16_t	m_dataLength;
  uint16_t  m_checksum;

  uint32_t m_size;

};

} // namespace ns3

#endif /* TCP_OPTION_MPTCP_H */
