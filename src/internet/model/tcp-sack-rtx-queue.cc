/*
 * tcp-sack-rtx-queue.cc
 *
 *  Created on: Jul 17, 2012
 *      Author: sergi
 */

#include "tcp-sack-rtx-queue.h"
#include <iostream>
#include <algorithm>
#include <string.h>
#include "ns3/packet.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("TcpSackRtxQueue");

namespace ns3 {

TypeId
TcpSackRtxQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSackRtxQueue")
    .SetParent<Object> ()
    .AddConstructor<TcpSackRtxQueue> ()
    /*.AddTraceSource ("UnackSequence",
                     "First unacknowledged sequence number (SND.UNA)",
                     MakeTraceSourceAccessor (&TcpTxBuffer::m_firstByteSeq))*/
  ;
  return tid;
}


TcpSackRtxQueue::TcpSackRtxQueue(SequenceNumber32 n)
{
    //conn = NULL;
    begin = end = n + 1;
}


TcpSackRtxQueue::~TcpSackRtxQueue()
{
    while (!rexmitQueue.empty())
        rexmitQueue.pop_front();
}

void TcpSackRtxQueue::init(SequenceNumber32 seqNum)
{
    begin = seqNum;
    end = seqNum;
}


void TcpSackRtxQueue::info()
{
    RexmitQueue::iterator i = rexmitQueue.begin();
    uint32_t j = 1;
    while (i!=rexmitQueue.end())
    {
    	NS_LOG_LOGIC(j << ". region: [" << i->beginSeqNum << ".." << i->endSeqNum << ") \t sacked=" << i->sacked << "\t rexmitted=" << i->rexmitted);
        i++;
        j++;
    }
}

SequenceNumber32 TcpSackRtxQueue::getBufferStartSeq()
{
    return begin;
}

SequenceNumber32 TcpSackRtxQueue::getBufferEndSeq()
{
    return end;
}

void TcpSackRtxQueue::discardUpTo(SequenceNumber32 seqNum)
{
    NS_LOG_LOGIC(seqNum);

    if (rexmitQueue.empty())
        return;

    NS_ASSERT(seqLE(begin,seqNum) && seqLE(seqNum,end+1));
    begin = seqNum;

    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end()) // discard/delete regions from rexmit queue, which have been acked
    {
        if (seqLess(i->beginSeqNum,begin))
            i = rexmitQueue.erase(i);
        else
            i++;
    }

    // update begin and end of rexmit queue
    if (rexmitQueue.empty())
        begin = end = 0;
    else
    {
        i = rexmitQueue.begin();
        begin = i->beginSeqNum;
        i = rexmitQueue.end();
        end = i->endSeqNum;
    }
}

void TcpSackRtxQueue::enqueueSentData(SequenceNumber32 fromSeqNum, SequenceNumber32 toSeqNum)
{
    bool found = false;

    NS_LOG_LOGIC("rexmitQ: enqueueSentData [" << fromSeqNum << ".." << toSeqNum);

    Region region;
    region.beginSeqNum = fromSeqNum;
    region.endSeqNum = toSeqNum;
    region.sacked = false;
    region.rexmitted = false;

    if (getQueueLength()==0)
    {
        begin = fromSeqNum;
        end = toSeqNum;
        rexmitQueue.push_back(region);
//        tcpEV << "rexmitQ: rexmitQLength=" << getQueueLength() << "\n";
        return;
    }

    if (seqLE(begin,fromSeqNum) && seqLE(toSeqNum,end))
    {
        // Search for region in queue!
        RexmitQueue::iterator i = rexmitQueue.begin();
        while (i!=rexmitQueue.end())
        {
            if (i->beginSeqNum == fromSeqNum && i->endSeqNum == toSeqNum)
            {
                i->rexmitted = true; // set rexmitted bit
                found = true;
            }
            i++;
        }
    }

    if (!found)
    {
        end = toSeqNum;
        rexmitQueue.push_back(region);
    }
//    tcpEV << "rexmitQ: rexmitQLength=" << getQueueLength() << "\n";
}

void TcpSackRtxQueue::setSackedBit(SequenceNumber32 fromSeqNum, SequenceNumber32 toSeqNum)
{
    bool found = false;

    if (seqLE(toSeqNum,end))
    {
        RexmitQueue::iterator i = rexmitQueue.begin();
        while (i!=rexmitQueue.end())
        {
            if (i->beginSeqNum == fromSeqNum && seqGE(toSeqNum, i->endSeqNum)) // Search for LE of region in queue!
            {
                i->sacked = true; // set sacked bit
                found = true;
                i++;
                while (seqGE(toSeqNum, i->endSeqNum) && i!=rexmitQueue.end()) // Search for RE of region in queue!
                {
                    i->sacked = true; // set sacked bit
                    i++;
                }
            }
            else
                i++;
        }
    }

    if (!found)
    	NS_LOG_LOGIC("FAILED to set sacked bit for region: [" << fromSeqNum << ".." << toSeqNum << "). Not found in retransmission queue.");
}

bool TcpSackRtxQueue::getSackedBit(SequenceNumber32 seqNum)
{
    bool found = false;

    if (seqLE(begin,seqNum))
    {
        RexmitQueue::iterator i = rexmitQueue.begin();
        while (i!=rexmitQueue.end())
        {
            if (i->beginSeqNum == seqNum) // Search for region in queue!
            {
                found = i->sacked;
                break;
            }
            i++;
        }
    }
    return found;
}

uint32_t TcpSackRtxQueue::getQueueLength()
{
    return rexmitQueue.size();
}

SequenceNumber32 TcpSackRtxQueue::getHighestSackedSeqNum()
{
	SequenceNumber32 tmp_highest_sacked = SequenceNumber32(0);

    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end())
    {
        if (i->sacked)
            tmp_highest_sacked = i->endSeqNum;
        i++;
    }
    return tmp_highest_sacked;
}

SequenceNumber32 TcpSackRtxQueue::getHighestRexmittedSeqNum()
{
	SequenceNumber32 tmp_highest_rexmitted = SequenceNumber32(0);

    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end())
    {
        if (i->rexmitted)
            tmp_highest_rexmitted = i->endSeqNum;
        i++;
    }
    return tmp_highest_rexmitted;
}

uint32_t TcpSackRtxQueue::checkRexmitQueueForSackedOrRexmittedSegments(SequenceNumber32 fromSeqNum)
{
    uint32_t counter = 0;

    if (fromSeqNum.GetValue()==0 || rexmitQueue.empty() || !(seqLE(begin,fromSeqNum) && seqLE(fromSeqNum,end)))
        return counter;

    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end())
    {
        // search for fromSeqNum (snd_nxt)
        if (i->beginSeqNum == fromSeqNum)
            break;
        else
            i++;
    }

    // search for adjacent sacked/rexmitted regions
    while (i!=rexmitQueue.end())
    {
        if (i->sacked || i->rexmitted)
        {
            counter = counter + (i->endSeqNum - i->beginSeqNum);

            // adjacent regions?
            SequenceNumber32 tmp = i->endSeqNum;
            i++;
            if (i->beginSeqNum != tmp)
                break;
        }
        else
            break;
    }
    return counter;
}

void TcpSackRtxQueue::resetSackedBit()
{
    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end())
    {
        i->sacked = false; // reset sacked bit
        i++;
    }
}

void TcpSackRtxQueue::resetRexmittedBit()
{
    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end())
    {
        i->rexmitted = false; // reset rexmitted bit
        i++;
    }
}

uint32_t TcpSackRtxQueue::getTotalAmountOfSackedBytes()
{
    uint32_t bytes = 0;
    uint32_t counter = 0;

    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end())
    {
        if (i->sacked)
        {
            counter++;
            bytes = bytes + (i->endSeqNum - i->beginSeqNum);
        }
        i++;
    }
    return bytes;
}

uint32_t TcpSackRtxQueue::getAmountOfSackedBytes(SequenceNumber32 seqNum)
{
    uint32_t bytes = 0;
    uint32_t counter = 0;

    if (rexmitQueue.empty() || seqGE(seqNum,end))
        return counter;

    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end() && seqLess(i->beginSeqNum, seqNum)) // search for seqNum
    {
        i++;
        if (i->beginSeqNum == seqNum)
            break;
    }

    NS_ASSERT(seqLE(seqNum,i->beginSeqNum) || seqGE(seqNum,--i->endSeqNum));

    while (i!=rexmitQueue.end())
    {
        if (i->sacked)
        {
            counter++;
            bytes = bytes + (i->endSeqNum - i->beginSeqNum);
        }
        i++;
    }
    return bytes;
}


uint32_t TcpSackRtxQueue::getNumOfDiscontiguousSacks(SequenceNumber32 seqNum)
{
    uint32_t counter = 0;

    if (rexmitQueue.empty() || seqGE(seqNum,end))
        return counter;

    RexmitQueue::iterator i = rexmitQueue.begin();
    while (i!=rexmitQueue.end() && seqLess(i->beginSeqNum, seqNum)) // search for seqNum
    {
        i++;
        if (i->beginSeqNum == seqNum)
            break;
    }

    NS_ASSERT(seqLE(seqNum,i->beginSeqNum) || seqGE(seqNum,--i->endSeqNum));

    // search for discontiguous sacked regions
    while (i!=rexmitQueue.end())
    {
        if (i->sacked)
        {
            counter++;
            SequenceNumber32 tmp = i->endSeqNum;
            i++;
            while (i->sacked && i->beginSeqNum == tmp && i!=rexmitQueue.end()) // adjacent sacked regions?
            {
                tmp = i->endSeqNum;
                i++;
            }
        }
        else
            i++;
    }
    return counter;
}

}
