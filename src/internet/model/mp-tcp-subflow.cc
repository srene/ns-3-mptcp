#include <stdint.h>
#include <iostream>
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "mp-tcp-subflow.h"
#include <stdlib.h>
#include <queue>
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include <ns3/object-base.h>
#include "ns3/simulator.h"
#include "time.h"
#include "ns3/packet.h"

NS_LOG_COMPONENT_DEFINE ("MpTcpTypeDefs");
namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpTcpSubFlow);

DSNMapping::DSNMapping ()
{
    subflowIndex     = 255;
    acknowledgement  = 0;
    dataSeqNumber    = 0;
    dataLevelLength  = 0;
    subflowSeqNumber = 0;
    dupAckCount      = 0;
    packet           = 0;
    //original         = true;
    retransmited     = false;
    tsval = Simulator::Now ().GetMilliSeconds (); // set the current time as a TimesTamp
}

DSNMapping::DSNMapping (uint8_t sFlowIdx, uint64_t dSeqNum, uint16_t dLvlLen, uint32_t sflowSeqNum, uint32_t ack, Ptr<Packet> pkt)
{
    subflowIndex     = sFlowIdx;
    dataSeqNumber    = dSeqNum;
    dataLevelLength  = dLvlLen;
    subflowSeqNumber = sflowSeqNum;
    acknowledgement  = ack;
    dupAckCount      = 0;
    packet = new uint8_t[dLvlLen];
    uint8_t *buf = new uint8_t[dLvlLen];
    pkt->CopyData(buf, dLvlLen);

    retransmited     = false;
    tsval = Simulator::Now ().GetMilliSeconds (); // set the current time as a TimesTamp
    //original         = true;
}
/*
DSNMapping::DSNMapping (const DSNMapping &res)
{
    subflowIndex     = res.subflowIndex;
    acknowledgement  = res.acknowledgement;
    dataSeqNumber    = res.dataSeqNumber;
    dataLevelLength  = res.dataLevelLength;
    subflowSeqNumber = res.subflowSeqNumber;
    dupAckCount      = res.dupAckCount;
    packet           = res.packet;
    original         = false;
}
*/
DSNMapping::~DSNMapping()
{
    /*
    if ( original == false )
        return;
        */
    dataSeqNumber    = 0;
    dataLevelLength  = 0;
    subflowSeqNumber = 0;
    dupAckCount      = 0;
    if( packet != 0 )
        delete[] packet;
}

bool
DSNMapping::operator < (const DSNMapping& rhs) const
{
	return this->dataSeqNumber < rhs.dataSeqNumber;
}

/*DataBuffer::DataBuffer ()
{
    bufMaxSize    = 0;
}

DataBuffer::DataBuffer (uint32_t size)
{
    bufMaxSize    = size;
}

DataBuffer::~DataBuffer ()
{
    bufMaxSize    = 0;
}

uint32_t
DataBuffer::Add (uint8_t* buf, uint32_t size)
{
    // read data from buf and insert it into the DataBuffer instance
    NS_LOG_FUNCTION (this << (int) size << (int) (bufMaxSize - (uint32_t) buffer.size()) );
    uint32_t toWrite = std::min(size, (bufMaxSize - (uint32_t) buffer.size()));

    if(buffer.empty() == true)
    {
        NS_LOG_INFO("DataBuffer::Add -> buffer is empty !");
    }else
        NS_LOG_INFO("DataBuffer::Add -> buffer was not empty !");

    uint32_t qty = 0;

    while( qty < toWrite )
    {
        buffer.push( buf[ qty ] );
        qty++;
    }
    NS_LOG_INFO("DataBuffer::Add -> amount of data = "<< qty);
    NS_LOG_INFO("DataBuffer::Add -> freeSpace Size = "<< (bufMaxSize - (uint32_t) buffer.size()) );
    return qty;
}

uint32_t
DataBuffer::Retrieve (uint8_t* buf, uint32_t size)
{
    NS_LOG_FUNCTION (this << (int) size << (int)  (bufMaxSize - (uint32_t) buffer.size()) );
    uint32_t quantity = std::min(size, (uint32_t) buffer.size());
    if( quantity == 0)
    {
        NS_LOG_INFO("DataBuffer::Retrieve -> No data to read from buffer reception !");
        return 0;
    }

    for(uint32_t i = 0; i < quantity; i++)
    {
        buf[i] = buffer.front();
        buffer.pop();
    }

    NS_LOG_INFO("DataBuffer::Retrieve -> freeSpaceSize == "<< bufMaxSize - (uint32_t) buffer.size() );
    return quantity;
}

Ptr<Packet>
DataBuffer::CreatePacket (uint32_t size)
{
    NS_LOG_FUNCTION (this << (int) size << (int) ( bufMaxSize - (uint32_t) buffer.size()) );
    uint32_t quantity = std::min(size, (uint32_t) buffer.size());
    if( quantity == 0 )
    {
        NS_LOG_INFO("DataBuffer::CreatePacket -> No data ready for sending !");
        return 0;
    }

    uint8_t *ptrBuffer = new uint8_t [quantity];
    for( uint32_t i = 0; i < quantity; i++)
    {
        ptrBuffer [i] = buffer.front();
        buffer.pop();
    }

    Ptr<Packet> pkt = Create<Packet> (ptrBuffer, quantity);
    delete[] ptrBuffer;

    NS_LOG_INFO("DataBuffer::CreatePacket -> freeSpaceSize == "<< bufMaxSize - (uint32_t) buffer.size() );
    return pkt;
}

uint32_t
DataBuffer::ReadPacket (Ptr<Packet> pkt, uint32_t dataLen)
{
    NS_LOG_FUNCTION (this << (int) (bufMaxSize - (uint32_t) buffer.size()) );

    uint32_t toWrite = std::min(dataLen, ( bufMaxSize - (uint32_t) buffer.size()) );

    if(buffer.empty() == true)
    {
        NS_LOG_INFO("DataBuffer::ReadPacket -> buffer is empty !");
    }else
        NS_LOG_INFO("DataBuffer::ReadPacket -> buffer was not empty !");

    uint8_t *ptrBuffer = new uint8_t [toWrite];
    pkt->CopyData (ptrBuffer, toWrite);

    for(uint32_t i =0; i < toWrite; i++)
        buffer.push( ptrBuffer[i] );
    delete[] ptrBuffer;

    NS_LOG_INFO("DataBuffer::ReadPacket -> data   readed == "<< toWrite );
    NS_LOG_INFO("DataBuffer::ReadPacket -> freeSpaceSize == "<< bufMaxSize - (uint32_t) buffer.size() );
    return toWrite;
}

uint32_t
DataBuffer::PendingData ()
{
    return ( (uint32_t) buffer.size() );
}

uint32_t
DataBuffer::FreeSpaceSize ()
{
    return (bufMaxSize - (uint32_t) buffer.size());
}

bool
DataBuffer::Empty ()
{
    return buffer.empty(); // ( freeSpaceSize == bufMaxSize );
}

bool
DataBuffer::Full ()
{
    return (bufMaxSize == (uint32_t) buffer.size());//( freeSpaceSize == 0 );
}*/


TypeId
MpTcpSubFlow::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::MpTcpSubFlow")
        .SetParent (Object::GetTypeId ())
        //.AddConstructor<MpTcpSubFlow> ()
        .AddTraceSource ("CongestionWindow",
                         "The congestion control window to trace.",
                         MakeTraceSourceAccessor (&MpTcpSubFlow::cwnd))
        ;
      return tid;
}

MpTcpSubFlow::MpTcpSubFlow()
    : routeId (0), state (CLOSED), phase (Slow_Start),
      sAddr (Ipv4Address::GetZero ()), sPort (0),
      dAddr (Ipv4Address::GetZero ()), dPort (0),
      oif (0), mapDSN (0), lastMeasuredRtt (Seconds (0.0))
{
    connected   = false;
    TxSeqNumber = rand() % 1000;
    RxSeqNumber = 0;
    bandwidth   = 0;
    cwnd        = 1;                  // congestion window is initialized to one segment
    scwnd       = 0;
    ssthresh    = 65535;              // initial value for a TCP connexion
    maxSeqNb    = TxSeqNumber - 1;    // thus we suppose that SYN & ACK segments has been acked correctly, for subflow n° 0
    highestAck  = 0;
    rtt = new RttMeanDeviation ();
    rtt->Gain(0.1); // 1.0

    // variables used for simulating drops
    LostThreshold     = 0.0;
    CanDrop           = true;
    PktCount          = 0;
    MaxPktCount       = rand() % 100 + 50;
    DropedPktCount    = 0;
    MaxDropedPktCount = 1;

    // variables used for reordering simulation
    savedCWND         = 0.0;
    savedSSThresh     = 0;
    SpuriousRecovery  = false;
    recover           = 0;
    ackCount          = 0;
    ReTxSeqNumber     = 0;
    nbRecvAck         = -1;
}

MpTcpSubFlow::MpTcpSubFlow(uint32_t TxSeqNb)
    : routeId (0), state (CLOSED), phase (Slow_Start),
      sAddr (Ipv4Address::GetZero ()), sPort (0),
      dAddr (Ipv4Address::GetZero ()), dPort (0),
      oif (0), mapDSN (0), lastMeasuredRtt (Seconds (0.0))
{
    connected   = false;
    TxSeqNumber = TxSeqNb;
    RxSeqNumber = 0;
    bandwidth   = 0;
    cwnd        = 1;                   // congestion window is initialized to one segment
    scwnd       = 0;
    ssthresh    = 65535;               // initial value for a TCP connexion
    maxSeqNb    = TxSeqNumber - 1;     // the subflow is created after receiving 'SYN ACK' segment
    highestAck  = 0;
    rtt = new RttMeanDeviation ();
    rtt->Gain(0.1); //1.0

    // variables used for simulating drops
    LostThreshold     = 0.0;
    CanDrop           = true;
    PktCount          = 0;
    MaxPktCount       = rand() % 100 + 100;
    DropedPktCount    = 0;
    MaxDropedPktCount = 1;

    // variables used for reordering simulation
    savedCWND         = 0.0;
    savedSSThresh     = 0;
    SpuriousRecovery  = false;
    recover           = 0;
    ackCount          = 0;
    ReTxSeqNumber     = 0;
}

MpTcpSubFlow::~MpTcpSubFlow()
{
    routeId     = 0;
    sAddr       = Ipv4Address::GetZero ();
    oif         = 0;
    state       = CLOSED;
    bandwidth   = 0;
    cwnd        = 1;
    maxSeqNb    = 0;
    highestAck  = 0;
    for(list<DSNMapping *>::iterator it = mapDSN.begin(); it != mapDSN.end(); ++it)
    {
        DSNMapping * ptrDSN = *it;
        delete ptrDSN;
    }
    mapDSN.clear();
}

void
MpTcpSubFlow::StartTracing (string traced)
{
    NS_LOG_INFO ("MpTcpSubFlow -> starting tracing of: "<< traced);
    TraceConnectWithoutContext (traced, MakeCallback (&MpTcpSubFlow::CwndTracer, this)); //"CongestionWindow"
}

void
MpTcpSubFlow::CwndTracer (double oldval, double newval)
{
    NS_LOG_WARN ("Subflow "<< routeId <<": Moving cwnd from " << oldval << " to " << newval);
}

void
MpTcpSubFlow::AddDSNMapping(uint8_t sFlowIdx, uint64_t dSeqNum, uint16_t dLvlLen, uint32_t sflowSeqNum, uint32_t ack, Ptr<Packet> pkt)
{
    NS_LOG_FUNCTION_NOARGS();
    mapDSN.push_back ( new DSNMapping(sFlowIdx, dSeqNum, dLvlLen, sflowSeqNum, ack, pkt) );
}

void
MpTcpSubFlow::updateRTT (SequenceNumber32 ack, Time current)
{
    NS_LOG_FUNCTION( this << ack << current );
    rtt->AckSeq ( ack );
    NS_LOG_INFO ("MpTcpSubFlow::updateRTT -> time from last RTT measure = " << (current - lastMeasuredRtt).GetSeconds() );
/*
    rtt->Measurement ( current - lastMeasuredRtt );
    lastMeasuredRtt = current;
    measuredRTT.insert(measuredRTT.end(), rtt->Estimate().GetSeconds ());
*/
    measuredRTT.insert(measuredRTT.end(), rtt->GetCurrentEstimate().GetSeconds ());
    NS_LOG_INFO ("MpTcpSubFlow::updateRTT -> estimated RTT = " << (rtt->GetCurrentEstimate().GetSeconds ()) );
}

DSNMapping *
MpTcpSubFlow::GetunAckPkt (uint32_t awnd)
{
    NS_LOG_FUNCTION_NOARGS();
    DSNMapping * ptrDSN = 0;

    for (list<DSNMapping *>::iterator it = mapDSN.begin(); it != mapDSN.end(); ++it)
    {
        DSNMapping * ptr = *it;
        NS_LOG_ERROR ("Subflow ("<<(int) routeId<<") Subflow Seq N° = " << ptr->subflowSeqNumber);
        if ( (ptr->subflowSeqNumber == highestAck + 1) || (ptr->subflowSeqNumber == highestAck + 2) )
        {
            // we added 2, for the case in wich the fst pkt of a subsequent subflow is lost, because the highest ack is the one included in 'SYN | ACK' which is 2 less than the current TxSeq
            NS_LOG_INFO ("MpTcpSubFlow::GetunAckPkt -> packet to retransmit found: sFlowSeqNum = " << ptr->subflowSeqNumber);
            /*
            if ( awnd < ptr->dataLevelLength )
            {
                DSNMapping * fstPtr = new DSNMapping(ptr->subflowIndex, ptr->dataSeqNumber, (uint16_t)awnd, ptr->subflowSeqNumber, ptr->acknowledgement, new Packet(ptr->packet, awnd));
                DSNMapping * sndPtr = new DSNMapping(ptr->subflowIndex, ptr->dataSeqNumber + (uint64_t)awnd, ptr->dataLevelLength - (uint16_t) awnd, ptr->subflowSeqNumber + awnd, ptr->acknowledgement, new Packet(ptr->packet + awnd,(uint32_t)ptr->dataLevelLength - awnd));
                delete ptr;
                ptr = fstPtr;
                mapDSN.insert( mapDSN.end(), sndPtr);
            }
            */
            ptrDSN = ptr;
            //mapDSN.erase (it);
            break;
        }
    }
    return ptrDSN;
}

MpTcpAddressInfo::MpTcpAddressInfo()
    : addrID (0), ipv4Addr (Ipv4Address::GetZero ()), mask (Ipv4Mask::GetZero())
{
}

MpTcpAddressInfo::~MpTcpAddressInfo()
{
    addrID = 0;
    ipv4Addr = Ipv4Address::GetZero ();
}

} // namespace ns3
