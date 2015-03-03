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

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/address.h"
#include "mih-tlv.h"

NS_LOG_COMPONENT_DEFINE ("Tlv-mih");

namespace ns3 {
  namespace mih {
    TlvMih::~TlvMih (void) {
    }
    uint8_t
    TlvMih::RegisterTlvType (void) {
      static uint8_t typ = 0;
      typ++;
      return typ;
    }
    uint32_t 
    TlvMih::ComputeLengthOfLengthField (uint32_t payloadLength)
    {
      uint32_t lengthOfLengthField = 0;
      
      if (payloadLength <= 128)
        {
          lengthOfLengthField = 1;
        }
      else if (payloadLength <= (1 << 8))
        {
          lengthOfLengthField = 2;
        }
      else if (payloadLength <= (1 << 16))
        {
          lengthOfLengthField = 3;
        }
      else if (payloadLength <= (1 << 24))
        {
          lengthOfLengthField = 4;
        }
      else
        {
          NS_ASSERT (payloadLength <= (1 << 24));
        }
      return lengthOfLengthField;
    }
    void
    TlvMih::WritePayloadLengthField (Buffer::Iterator &i, uint32_t payloadLength)
    {
      if (payloadLength <= 128)
        {
          i.WriteU8 (payloadLength);
        }
      else if (payloadLength <= (1 << 8))
        {
          i.WriteU8 (128 + 1);
          i.WriteU8 (payloadLength - 128);
        }
      else if (payloadLength <= (1 << 16))
        {
          i.WriteU8 (128 + 2);
          i.WriteU8 (((payloadLength - 128) >> 8) && 0xff);
          i.WriteU8 ((payloadLength - 128) && 0xff);
        }
      else if (payloadLength <= (1 << 24))
        {
          i.WriteU8 (128 + 3);
          i.WriteU8 (((payloadLength - 128) >> 16) && 0xff);
          i.WriteU8 (((payloadLength - 128) >> 8) && 0xff);
          i.WriteU8 ((payloadLength - 128) && 0xff);
        }
      else
        {
          NS_ASSERT (payloadLength <= (1 << 24));
        }
    }
    uint32_t 
    TlvMih::ReadPayloadLengthField (Buffer::Iterator &i, uint32_t roomSize, uint32_t &payloadLengthValue)
    {
      uint32_t bytesRead = 0;
      uint32_t lengthOfLengthField = 0;
      
      if (roomSize <= 128)
        {
          payloadLengthValue = roomSize;
        }
      else // MSB is 1
        {
          lengthOfLengthField = roomSize - 128;
          for (uint32_t j = 0; j < lengthOfLengthField; j++)
            {
              payloadLengthValue <<= 8;
              payloadLengthValue += i.ReadU8 ();
              bytesRead++;
            }
          payloadLengthValue += 128;
        }
      return bytesRead;
    }
    void 
    TlvMih::PrintBuffer (std::ostream &os, 
                      const Buffer &buffer)
    {
      uint32_t size = buffer.GetSize ();
      Buffer::Iterator i = buffer.Begin ();
      os << "Buffer data = [\n\t";
      uint32_t col = 1;
      for (uint32_t j = 0; j < size; j++, col++)
        {
          uint8_t byteRead = i.ReadU8 ();
          os << "0x" << std::hex << (int) byteRead << " ";
          if (col % 10 == 0)
            {
              os << std::endl << "\t";
            }
        }
      os << "\n]" << std::endl;
    }
    // TLV methods for Address class;
    uint32_t
    TlvMih::GetSerializedSize (Address address)
    {
      return 1 + //Type
        1 + //Length
        address.GetSerializedSize (); //Value;
    }
    void
    TlvMih::Serialize (Buffer &buffer, 
                    Address address, 
                    uint8_t tlvTypeValue)
    {
      uint32_t roomSize = GetSerializedSize (address);
      uint32_t formerBufferSize = buffer.GetSize ();
      buffer.AddAtEnd (roomSize);
      Buffer::Iterator i = buffer.Begin ();
      i.Next (formerBufferSize);
      i.WriteU8 (tlvTypeValue);
      i.WriteU8 (address.GetSerializedSize ());
      i.WriteU8 (address.m_type);
      i.WriteU8 (address.m_len);
      const uint8_t *data = address.m_data;
      for (uint32_t j = 0; j < address.m_len; j++, data++)
        {
          i.WriteU8 (*data);
        }
    }
    uint32_t 
    TlvMih::Deserialize (Buffer &buffer, Address &address, uint8_t tlvTypeValue)
    {
      Buffer::Iterator i = buffer.Begin ();
      NS_ASSERT_MSG (i.ReadU8 () == tlvTypeValue, "Address TLV type values mismatch!");
      i.Next (1);
      address.m_type = i.ReadU8 ();
      address.m_len = i.ReadU8 ();
      uint8_t *data = address.m_data;
      for (uint32_t j = 0; j < address.m_len; j++, data++)
        {
          *data = i.ReadU8 ();
        }
      buffer.RemoveAtStart (1 + 1 + 1 + 1 + address.m_len);
      return 1 + 1 + 1 + 1 + address.m_len;
    }
    // TLV for bool values;
    uint32_t 
    TlvMih::GetSerializedSizeBool (void)
    {
      return TlvMih::GetSerializedSizeU8 ();
    }
    void 
    TlvMih::SerializeBool (Buffer &buffer, 
                        bool element, 
                        uint8_t tlvTypeValue)
    {
      element ? TlvMih::SerializeU8 (buffer, 1, tlvTypeValue) :
        TlvMih::SerializeU8 (buffer, 0, tlvTypeValue);
    }
    uint32_t
    TlvMih::DeserializeBool (Buffer &buffer, 
                          bool &element, 
                          uint8_t tlvTypeValue)
    {
      uint8_t res;
      uint32_t size;
      size = TlvMih::DeserializeU8 (buffer, res, tlvTypeValue);
      if (res == 0)
        {
          element = false;
        }
      else if (res == 1)
        {
          element = true;
        }
      else
        {
          NS_ASSERT (res == 0 || res == 1);
        }
      return size;
      
    }    
    // TLV for uint8_t values;
    uint32_t 
    TlvMih::GetSerializedSizeU8 (void)
    {
      return 1 + 1 + 1;
    }
    void
    TlvMih::SerializeU8 (Buffer &buffer, 
                      uint8_t element, 
                      uint8_t tlvTypeValue) 
    {
      uint32_t roomSize = TlvMih::GetSerializedSizeU8();
      uint32_t formerBufferSize = buffer.GetSize ();
      buffer.AddAtEnd (roomSize);
      Buffer::Iterator i = buffer.Begin ();
      i.Next (formerBufferSize);
      i.WriteU8 (tlvTypeValue);
      i.WriteU8 (1);
      i.WriteU8 (element);
    }
    uint32_t 
    TlvMih::DeserializeU8 (Buffer &buffer,
                        uint8_t &element,
                        uint8_t tlvTypeValue)
    {
      Buffer::Iterator i = buffer.Begin ();
      uint8_t vl = i.ReadU8 ();
      NS_LOG_INFO ("TlvTypeValue = " << (int) tlvTypeValue << " i.ReadU8 () = " << (int) vl);
      NS_ASSERT (vl == tlvTypeValue);
      i.Next ();
      element = i.ReadU8 ();
      buffer.RemoveAtStart (1 + 1 + 1);
      return 1 + 1 + 1;
    }
    // TLV for uint16_t values;
    uint32_t
    TlvMih::GetSerializedSizeU16 (void)
    {
      return 1 + 1 + 2;
    }
    void 
    TlvMih::SerializeU16 (Buffer &buffer,
                       uint16_t element, 
                       uint8_t tlvTypeValue)
    {
      uint32_t roomSize = TlvMih::GetSerializedSizeU16 ();
      uint32_t formerBufferSize = buffer.GetSize ();
      buffer.AddAtEnd (roomSize);
      Buffer::Iterator i = buffer.Begin ();
      i.Next (formerBufferSize);
      i.WriteU8 (tlvTypeValue);
      i.WriteU8 (2);
      i.WriteU16 (element);
      
    }
    uint32_t 
    TlvMih::DeserializeU16 (Buffer &buffer, 
                         uint16_t &element, 
                         uint8_t tlvTypeValue)
    {
      Buffer::Iterator i = buffer.Begin ();
      uint8_t vl = i.ReadU8 ();
      NS_LOG_INFO ("TlvTypeValue = " << (int) tlvTypeValue << " i.ReadU8 () = " << (int) vl);
      NS_ASSERT (vl == tlvTypeValue);
      i.Next ();
      element = i.ReadU16 ();
      buffer.RemoveAtStart (1 + 1 + 2);
      return 1 + 1 + 2;
      
    }
    // TLV for uint32_t values;
    uint32_t 
    TlvMih::GetSerializedSizeU32 (void)
    {
      return 1 + 1 + 4;      
    }
    void 
    TlvMih::SerializeU32 (Buffer &buffer, 
                       uint32_t element, 
                       uint8_t tlvTypeValue)
    {
      uint32_t roomSize = TlvMih::GetSerializedSizeU32 ();
      uint32_t formerBufferSize = buffer.GetSize ();
      buffer.AddAtEnd (roomSize);
      Buffer::Iterator i = buffer.Begin ();
      i.Next (formerBufferSize);
      i.WriteU8 (tlvTypeValue);
      i.WriteU8 (4);
      i.WriteU32 (element);
    }
    uint32_t 
    TlvMih::DeserializeU32 (Buffer &buffer,
                         uint32_t &element,
                         uint8_t tlvTypeValue)
    {
      Buffer::Iterator i = buffer.Begin ();
      NS_ASSERT (i.ReadU8 () == tlvTypeValue);
      i.Next ();
      element = i.ReadU32 ();
      buffer.RemoveAtStart (1 + 1 + 4);
      return 1 + 1 + 4;      
    }
    // TLV for uint64_t values;
    uint32_t
    TlvMih::GetSerializedSizeU64 (void)
    {
      return 1 + 1 + 8;
    }
    void 
    TlvMih::SerializeU64 (Buffer &buffer,
                       uint64_t element, 
                       uint8_t tlvTypeValue)
    {
      uint32_t roomSize = TlvMih::GetSerializedSizeU64 ();
      uint32_t formerBufferSize = buffer.GetSize ();
      buffer.AddAtEnd (roomSize);
      Buffer::Iterator i = buffer.Begin ();
      i.Next (formerBufferSize);
      i.WriteU8 (tlvTypeValue);
      i.WriteU8 (8);
      i.WriteU64 (element);
    }
    uint32_t 
    TlvMih::DeserializeU64 (Buffer &buffer, 
                         uint64_t &element,
                         uint8_t tlvTypeValue)
    {
      Buffer::Iterator i = buffer.Begin ();
      NS_ASSERT (i.ReadU8 () == tlvTypeValue);
      i.Next ();
      element = i.ReadU64 ();
      buffer.RemoveAtStart (1 + 1 + 8);
      return 1 + 1 + 8;
    }
    // TLV for string values;
    uint32_t
    TlvMih::GetSerializedSizeString (std::string element) 
    {
      uint32_t payloadLength = 0;
      uint32_t lengthOfLengthField = 0;
      
      payloadLength = element.length ();
      NS_ASSERT (payloadLength <= 253);
      lengthOfLengthField = TlvMih::ComputeLengthOfLengthField (payloadLength);
      return 1 + lengthOfLengthField + payloadLength;      
    }
    void 
    TlvMih::SerializeString (Buffer &buffer,
                          std::string element, 
                          uint8_t tlvTypeValue)
    {
      uint32_t formerBufferSize = buffer.GetSize ();
      uint32_t lengthOfLengthField = 0;
      uint32_t payloadLength = 0;
      
      payloadLength = element.length ();
      NS_ASSERT (payloadLength <= 253);
      lengthOfLengthField = TlvMih::ComputeLengthOfLengthField (payloadLength);
      buffer.AddAtEnd (1 + lengthOfLengthField + payloadLength);       
      Buffer::Iterator i = buffer.Begin ();
      i.Next (formerBufferSize);
      i.WriteU8 (tlvTypeValue); // Type field;
      TlvMih::WritePayloadLengthField (i, payloadLength); // Length field;
      for (uint32_t j = 0; j < element.length (); j++)
        {
          i.WriteU8 (element[j]);
        }
    }
    uint32_t 
    TlvMih::DeserializeString (Buffer &buffer,
                            std::string &element, 
                            uint8_t tlvTypeValue)
    {
      uint32_t totalBytesRead = 0;
      uint32_t roomSize;
      uint32_t payloadLengthValue = 0;
      Buffer::Iterator i = buffer.Begin ();
      NS_ASSERT (i.ReadU8 () == tlvTypeValue);
      totalBytesRead++;
      roomSize = i.ReadU8 ();
      totalBytesRead++;
      
      totalBytesRead += TlvMih::ReadPayloadLengthField (i, roomSize, payloadLengthValue);

      element = "";
      for (uint32_t j = 0; j < payloadLengthValue; j++)
        {
          element += i.ReadU8 ();
          totalBytesRead++;
        }
      buffer.RemoveAtStart (totalBytesRead);
      return totalBytesRead;
    }
  } // namespace mih
} // namespace ns3
