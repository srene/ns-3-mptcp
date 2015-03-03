/*
 * mp-scheduler.h
 *
 *  Created on: Aug 20, 2013
 *      Author: sergi
 */

#ifndef MP_SCHEDULER_H_
#define MP_SCHEDULER_H_

#include "ns3/tcp-socket-base.h"
#include "ns3/object.h"
#include "tcp-tx-buffer64.h"

namespace ns3 {
typedef std::map<Ptr<TcpSocketBase>,uint32_t> SocketPacketMap;
typedef std::pair<Ptr<TcpSocketBase>,uint32_t> SocketPacketPair;
typedef std::map<Ptr<TcpSocketBase>,uint32_t>::iterator SocketPacketIte;
typedef std::pair<SequenceNumber64,uint32_t> SizePair;
typedef std::map<SequenceNumber64,uint32_t> SizeMap;
//typedef std::pair<Ptr<TcpSocketBase>,SequenceNumber32> SeqPair;
//typedef std::pair<SequenceNumber64,std::vector<SeqPair> > SubflowPair;
//typedef std::map<SequenceNumber64,std::vector<SeqPair> > SubflowMap;
typedef std::pair<SequenceNumber64,SequenceNumber32> SeqPair;
typedef std::pair<Ptr<TcpSocketBase>,std::vector<SeqPair> > SubflowPair;
typedef std::map<Ptr<TcpSocketBase>,std::vector<SeqPair> > SubflowMap;
typedef std::pair<Ptr<TcpSocketBase>,uint32_t> BuffSizePair;
typedef std::map<Ptr<TcpSocketBase>,uint32_t> BuffSizeMap;
typedef std::pair<Ptr<TcpSocketBase>,SequenceNumber32> FirstSeqPair;
typedef std::map<Ptr<TcpSocketBase>,SequenceNumber32> FirstSeqMap;


class MpScheduler : public Object {
public:
    static TypeId GetTypeId (void);
	MpScheduler();
	virtual ~MpScheduler();
	uint32_t Available (void) const;
	uint32_t Size(Ptr<TcpSocketBase> tcpSocketBase);
	uint32_t Assigned(Ptr<TcpSocketBase> tcpSocketBase);
	uint32_t SizeFromSequence(Ptr<TcpSocketBase> tcpSocketBase,const SequenceNumber32& seq);
	void SetHeadSequence(Ptr<TcpSocketBase> tcpSocketBase,const SequenceNumber32& seq);
	SequenceNumber32 HeadSequence(Ptr<TcpSocketBase> tcpSocketBase);
	SequenceNumber32 TailSequence(Ptr<TcpSocketBase> tcpSocketBase);
	Ptr<Packet> CopyFromSequence (Ptr<TcpSocketBase> tcpSocketBase, uint32_t numBytes, const SequenceNumber32& seq);
	void DiscardUpTo(Ptr<TcpSocketBase> sock,SequenceNumber32 seq);
	virtual int Discard(SequenceNumber64 seq);
	virtual int Add(Ptr<Packet> packet,uint32_t subflow);
	virtual bool GetPacket(std::vector<Ptr<TcpSocketBase> > sockets) =  0;
	virtual void SetNextSequence(SequenceNumber64 seq);
	virtual int GetSubflowToUse();
	virtual void SetSndBufSize (uint32_t size);
	virtual uint32_t GetSndBufSize (void) const;
	uint32_t GetBufSize (void) const;
	SequenceNumber64 GetNextSequence (Ptr<TcpSocketBase> tcpSocketBase, const SequenceNumber32& seq);
	void SendBackup(Ptr<TcpSocketBase> tcpSocketBase);

protected:

	void MapSequence(Ptr<TcpSocketBase> tcpSocketBase);
	void UpdateAssigned(Ptr<TcpSocketBase> tcpSocketBase);
	  Time m_rtt;
	  bool m_IsCount;
	  EventId  m_bwEstimateEvent;
	  std::map<double,uint32_t> bweMap;
	  std::pair<double,uint32_t> bwePair;
	  std::map<uint32_t,double> lambdaMap;
	  std::pair<uint32_t,double> lambdaPair;
	  SizeMap m_sizeTxMap;
      TcpTxBuffer64  m_txBuffer;       //< Tx buffer
      SequenceNumber64 m_nextTxSequence; //< Next seqnum to be sent (SND.NXT), ReTx pushes it back

      int m_lastUsedsFlowIdx;

      SubflowMap m_assignedSubflow;
      SubflowMap m_sentSubflow;

      FirstSeqMap m_seqMap;
      BuffSizeMap m_bufSizeMap;
      BuffSizeMap m_assignedSizeMap;
      FirstSeqMap m_asSeqMap;
	  uint32_t m_mtu;

};

} /* namespace ns3 */
#endif /* MP_SCHEDULER_H_ */
