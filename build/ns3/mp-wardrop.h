/*
 * mp-wardrop.h
 *
 *  Created on: Aug 20, 2013
 *      Author: sergi
 */

#ifndef MP_WARDROP_H_
#define MP_WARDROP_H_

#include <stdint.h>
#include <queue>
#include "ns3/mp-scheduler.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"

namespace ns3 {

typedef std::map<SequenceNumber64,uint8_t> SeqFlowMap;
typedef std::pair<SequenceNumber64,uint8_t> SeqFlowPair;
typedef std::map<SequenceNumber64,uint8_t>::iterator SeqIte;

typedef std::map<uint32_t,std::vector<uint32_t> > SubflowToUse;
typedef std::map<uint32_t,std::vector<uint32_t> >::iterator SubflowIte;
typedef std::pair<uint32_t,std::vector<uint32_t> > SubflowPair;

typedef std::pair<SequenceNumber64,SequenceNumber64> SeqPair;
typedef std::map<SequenceNumber64,SequenceNumber64> SeqMap;
typedef std::pair<SequenceNumber64,uint32_t> SizePair;
typedef std::map<SequenceNumber64,uint32_t> SizeMap;
class MpWardrop : public MpScheduler {
public:
    static TypeId GetTypeId (void);
	MpWardrop();
	virtual ~MpWardrop();
	//virtual int Add(Ptr<Packet> packet);
	virtual int Add(Ptr<Packet> packet,uint32_t subflow);
	virtual int Discard(SequenceNumber64 seq);
	virtual Ptr<Packet> GetPacket(std::vector<Ptr<TcpSocketBase> > sockets);
	virtual void SetNextSequence(SequenceNumber64 seq);
	//virtual int GetSubflowToUse(SequenceNumber64 seq, std::vector<Ptr<TcpSocketBase> > sockets,uint32_t lastUsedsFlowIdx);
	virtual int GetSubflowToUse();
	void SetSndBufSize (uint32_t size);
	uint32_t GetSndBufSize (void) const;
	void SetSndBufSize1 (uint32_t size);
	uint32_t GetSndBufSize1 (void) const;
	void SetSndBufSize2 (uint32_t size);
	uint32_t GetSndBufSize2 (void) const;
	void SetMtu(uint32_t mtu);
private:
	virtual void  Update(std::vector<Ptr<TcpSocketBase> > sockets);
	double GetLanda(uint32_t flow, uint32_t channel);
	//uint32_t GetSubflowToUse(uint8_t subflow, uint32_t lastUsedsFlowIdx);
	uint32_t m_limit;
	uint32_t m_limit2;
	uint32_t m_mtu;

	uint32_t c1;
	uint32_t c2;


	double m_alpha;
	double m_beta;

	SeqFlowMap seqMap;
	  Time t0;
	  double m_landaRate;
	  double m_lastLanda;
	  uint32_t data;

	  Time t1;
	  double m_landaRate1;
	  double m_lastLanda1;
	  uint32_t data1;

	  Time t2;
	  double m_landaRate2;
	  double m_lastLanda2;
	  uint32_t data2;

	  double m_landaRate11;
	  Time t;
	  uint32_t data11;
	  double m_landaRate12;
	  uint32_t data12;
	  double m_landaRate21;
	  uint32_t data21;
	  double m_landaRate22;
	  uint32_t data22;

	  uint32_t m_landa11Rate;
	  uint32_t m_landa12Rate;
	  uint32_t m_landa21Rate;
	  uint32_t m_landa22Rate;

	  std::map<uint32_t,double> landaMap2;


	  //uint32_t m_lastUsedsFlowId1;
	  uint32_t m_lastUsedsFlowIdx2;

	 // SubflowToUse m_wsubflowsToUse;
	  bool drop;

      TcpTxBuffer64  m_txBuffer2;       //< Tx buffer
      TcpTxBuffer64  m_txBuffer1;       //< Tx buffer

      SeqMap m_seqTxMap1;
      SizeMap m_sizeTxMap1;

      SeqMap m_seqTxMap2;
      SizeMap m_sizeTxMap2;

      SequenceNumber64 m_nextTxSequence,m_nextTxSequence1,m_nextTxSequence2;

      TracedValue<uint32_t> m_buffer;
      TracedValue<uint32_t> m_buffer1;
      TracedValue<uint32_t> m_buffer2;

      double m_landa21;
      double m_landa22;

      bool sent;

      int nextSubFlow;

	  bool update;

};

}

#endif /* MP_WARDROP_H_ */
