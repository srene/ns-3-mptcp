/*
 * mp-scheduler.h
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#ifndef MP_WATERFILLING2_H_
#define MP_WATERFILLING2_H_

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

typedef std::pair<SequenceNumber64,Time> TimePair;
typedef std::map<SequenceNumber64,Time> TimeMap;

class MpWaterfilling2 : public MpScheduler {

public:
    static TypeId GetTypeId (void);
    MpWaterfilling2();
	MpWaterfilling2(SequenceNumber64 nextTxSequence);

	virtual ~MpWaterfilling2();
	virtual int Add(Ptr<Packet> packet,uint32_t subflow);
	virtual int Discard(SequenceNumber64 seq);
	virtual bool GetPacket(std::vector<Ptr<TcpSocketBase> > sockets);
	//void SetSndBufSize (uint32_t size);
	//uint32_t GetSndBufSize (void) const;
private:


	  double m_landaRate;
	  double m_lastSampleLanda;
	  double m_lastLanda;
	  Time t1;
	  Time t;
	  uint32_t data;
	  bool sent;
	  bool update;
      TimeMap m_timeMap1,m_timeMap2;

};

} /* namespace ns3 */
#endif /* MP_WATERFILLING_H_ */
