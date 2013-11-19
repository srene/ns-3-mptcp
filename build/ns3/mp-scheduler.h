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

class MpScheduler : public Object {
public:
    static TypeId GetTypeId (void);
	MpScheduler();
	virtual ~MpScheduler();

	//virtual int Add(Ptr<Packet> packet) = 0;
	virtual int Add(Ptr<Packet> packet,uint32_t subflow) = 0;
	virtual int Discard(SequenceNumber64 seq) = 0;
	virtual Ptr<Packet> GetPacket(std::vector<Ptr<TcpSocketBase> > sockets) =  0;
	virtual void SetNextSequence(SequenceNumber64 seq) = 0;
	//virtual int GetSubflowToUse(SequenceNumber64 seq, std::vector<Ptr<TcpSocketBase> > sockets,uint32_t lastUsedsFlowIdx) = 0;
	virtual int GetSubflowToUse() = 0;
	virtual void SetMtu(uint32_t mtu) = 0;
protected:

	  Time m_rtt;
	  bool m_IsCount;
	  EventId  m_bwEstimateEvent;
	  //std::vector<double> bwe;
	  std::map<double,uint32_t> bweMap;
	  std::pair<double,uint32_t> bwePair;
	  std::map<uint32_t,double> landaMap;
	  std::pair<uint32_t,double> landaPair;
	  //std::vector<uint32_t> m_subflowsToUse;
	  //std::vector<double> m_landa;
	  SizeMap m_sizeTxMap;
      TcpTxBuffer64  m_txBuffer;       //< Tx buffer
      SequenceNumber64 m_nextTxSequence; //< Next seqnum to be sent (SND.NXT), ReTx pushes it back

      uint32_t m_lastUsedsFlowIdx;
};

} /* namespace ns3 */
#endif /* MP_SCHEDULER_H_ */
