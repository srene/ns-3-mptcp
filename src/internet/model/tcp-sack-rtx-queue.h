/*
 * tcp-sack-rtx-queue.h
 *
 *  Created on: Jul 17, 2012
 *      Author: sergi
 */

#ifndef TCP_SACK_RX_QUEUE_H_
#define TCP_SACK_RX_QUEUE_H_

#include <map>
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/sequence-number.h"
#include "ns3/ptr.h"
#include "ns3/tcp-header.h"

namespace ns3 {

/** @name Comparing sequence numbers */
//@{
inline bool seqLess(SequenceNumber32 a, SequenceNumber32 b) {return a.GetValue()!=b.GetValue() && b.GetValue()-a.GetValue()<(1UL<<31);}
inline bool seqLE(SequenceNumber32 a, SequenceNumber32 b) {return b.GetValue()-a.GetValue()<(1UL<<31);}
inline bool seqGreater(SequenceNumber32 a, SequenceNumber32 b) {return a.GetValue()!=b.GetValue() && a.GetValue()-b.GetValue()<(1UL<<31);}
inline bool seqGE(SequenceNumber32 a, SequenceNumber32 b) {return a.GetValue()-b.GetValue()<(1UL<<31);}
//@}

class TcpSackRtxQueue : public Object {
public:
	static TypeId GetTypeId (void);
    struct Region
    {
		SequenceNumber32 beginSeqNum;
		SequenceNumber32 endSeqNum;
        bool sacked;      // indicates whether region has already been sacked by data receiver
        bool rexmitted;   // indicates whether region has already been retransmitted by data sender
    };
    typedef std::list<Region> RexmitQueue;
    RexmitQueue rexmitQueue;

    SequenceNumber32 begin;  // 1st sequence number stored
    SequenceNumber32 end;    // last sequence number stored +1

	TcpSackRtxQueue(SequenceNumber32 n = SequenceNumber32(0));
	virtual ~TcpSackRtxQueue();
    /**
     * Set the connection that owns this queue.
     */
    //virtual void setConnection(TCPConnection *_conn)  {conn = _conn;}

    /**
     * Initialize the object. The startSeq parameter tells what sequence number the first
     * byte of app data should get. This is usually ISS+1 because SYN consumes
     * one byte in the sequence number space.
     *
     * init() may be called more than once; every call flushes the existing contents
     * of the queue.
     */
    virtual void init(SequenceNumber32 seqNum);

    /**
     * Prints the current rexmitQueue status for debug purposes.
     */
    virtual void info();

    /**
     * Returns the sequence number of the first byte stored in the buffer.
     */
    virtual SequenceNumber32 getBufferStartSeq();

    /**
     * Returns the sequence number of the last byte stored in the buffer plus one.
     * (The first byte of the next send operation would get this sequence number.)
     */
    virtual SequenceNumber32 getBufferEndSeq();

    /**
     * Tells the queue that bytes up to (but NOT including) seqNum have been
     * transmitted and ACKed, so they can be removed from the queue.
     */
    virtual void discardUpTo(SequenceNumber32 seqNum);

    /**
     * Inserts sent data to the rexmit queue.
     */
    virtual void enqueueSentData(SequenceNumber32 fromSeqNum, SequenceNumber32 toSeqNum);

    /**
     * Called when data sender received selective acknowledgments.
     * Tells the queue which bytes have been transmitted and SACKed,
     * so they can be skipped if retransmitting segments as long as
     * REXMIT timer did not expired.
     */
    virtual void setSackedBit(SequenceNumber32 fromSeqNum, SequenceNumber32 toSeqNum);

    /**
     * Returns SackedBit value of seqNum.
     */
    virtual bool getSackedBit(SequenceNumber32 seqNum);

    /**
     * Returns the number of blocks currently buffered in queue.
     */
    virtual uint32_t getQueueLength();

    /**
     * Returns the highest sequence number sacked by data receiver.
     */
    virtual SequenceNumber32 getHighestSackedSeqNum();

    /**
     * Returns the highest sequence number rexmitted by data sender.
     */
    virtual SequenceNumber32 getHighestRexmittedSeqNum();

    /**
     * Checks rexmit queue for sacked of rexmitted segments and returns a certain offset
     * (contiguous sacked or rexmitted region) to forward snd->nxt.
     * It is called before retransmitting data.
     */
    virtual uint32_t checkRexmitQueueForSackedOrRexmittedSegments(SequenceNumber32 fromSeq);

    /**
     * Called when REXMIT timer expired.
     * Resets sacked bit of all segments in rexmit queue.
     */
    virtual void resetSackedBit();

    /**
     * Called when REXMIT timer expired.
     * Resets rexmitted bit of all segments in rexmit queue.
     */
    virtual void resetRexmittedBit();

    /**
     * Returns total amount of sacked bytes. Corresponds to update() function from RFC 3517.
     */
    virtual uint32_t getTotalAmountOfSackedBytes();

    /**
     * Returns amount of sacked bytes above seqNum.
     */
    virtual uint32_t getAmountOfSackedBytes(SequenceNumber32 seqNum);

    /**
     * Returns the number of discontiguous sacked regions (SACKed sequences) above seqNum.
     */
    virtual uint32_t getNumOfDiscontiguousSacks(SequenceNumber32 seqNum);
};

} //namepsace ns3

#endif /* TCP_SACK_RX_QUEUE_H_ */
