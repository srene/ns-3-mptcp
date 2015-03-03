/*
 * mp-scheduler.h
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#ifndef MP_WROUNDROBIN_H_
#define MP_WROUNDROBIN_H_

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

class MpWeightedRoundRobin : public MpScheduler {

public:
    static TypeId GetTypeId (void);
    MpWeightedRoundRobin();
	MpWeightedRoundRobin(SequenceNumber64 nextTxSequence);

	virtual ~MpWeightedRoundRobin();
	virtual int Discard(SequenceNumber64 seq);
	virtual bool GetPacket(std::vector<Ptr<TcpSocketBase> > sockets);
	//void SetSndBufSize (uint32_t size);
	//uint32_t GetSndBufSize (void) const;

private:
	uint32_t w1,w2;
	uint32_t s1,s2;

};

} /* namespace ns3 */
#endif /* MP_ROUNDROBIN_H_ */
