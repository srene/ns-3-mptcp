/*
 * mp-scheduler.h
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#ifndef MP_ROUNDROBIN_H_
#define MP_ROUNDROBIN_H_

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

class MpRoundRobin : public MpScheduler {

public:
    static TypeId GetTypeId (void);
    MpRoundRobin();
	MpRoundRobin(SequenceNumber64 nextTxSequence);

	virtual ~MpRoundRobin();
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

	  double m_landaRate;
	  double m_lastSampleLanda;
	  double m_lastLanda;
	  Time t;
	  uint32_t data;

	  uint32_t m_mtu;

	  uint32_t nextSubFlow;

	  uint32_t data1;
	  uint32_t data2;
};

} /* namespace ns3 */
#endif /* MP_ROUNDROBIN_H_ */
