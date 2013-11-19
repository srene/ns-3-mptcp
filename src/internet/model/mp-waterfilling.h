/*
 * mp-scheduler.h
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#ifndef MP_WATERFILLING_H_
#define MP_WATERFILLING_H_

#include <stdint.h>
#include <queue>
#include "mp-scheduler.h"
#include "ns3/traced-callback.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"

namespace ns3 {
//class Packet;

class MpWaterfilling : public MpScheduler {

public:
    static TypeId GetTypeId (void);
    MpWaterfilling();
	MpWaterfilling(SequenceNumber64 nextTxSequence);

	virtual ~MpWaterfilling();
	//virtual int Add(Ptr<Packet> packet);
	virtual int Add(Ptr<Packet> packet,uint32_t subflow);
	virtual int Discard(SequenceNumber64 seq);
	virtual Ptr<Packet> GetPacket(std::vector<Ptr<TcpSocketBase> > sockets);
	virtual void SetNextSequence(SequenceNumber64 seq);
	//virtual int GetSubflowToUse(SequenceNumber64 seq, std::vector<Ptr<TcpSocketBase> > sockets,uint32_t lastUsedsFlowIdx);
	virtual int GetSubflowToUse();
	void SetSndBufSize (uint32_t size);
	uint32_t GetSndBufSize (void) const;
	double GetLanda(uint32_t channel);
	void SetMtu(uint32_t mtu);
private:
	virtual void  Update(std::vector<Ptr<TcpSocketBase> > sockets);

	uint32_t c1;
	uint32_t c2;
	uint32_t c3;

	  double m_landaRate;
	  double m_lastSampleLanda;
	  double m_lastLanda;
	  Time t;
	  uint32_t data;

	  uint32_t m_landa1Rate;
	  uint32_t m_landa2Rate;
	  uint32_t m_landa3Rate;

	  uint32_t data1;
	  uint32_t data2;

	  uint32_t nextSubFlow;
	  bool sent;

	  bool update;

	  uint32_t m_lastUsedsFlowIdx;
};

} /* namespace ns3 */
#endif /* MP_WATERFILLING_H_ */
