/*
 * mp-scheduler.cc
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#include "mp-wroundrobin.h"
#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"


NS_LOG_COMPONENT_DEFINE ("MpWeightedRoundRobin");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpWeightedRoundRobin);

TypeId
MpWeightedRoundRobin::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpWeightedRoundRobin")
.SetParent<MpScheduler> ()
.AddConstructor<MpWeightedRoundRobin> ()
.AddAttribute ("w1","Weight for subflow1",
               UintegerValue (1),
               MakeUintegerAccessor (&MpWeightedRoundRobin::w1),
               MakeUintegerChecker<uint32_t> ())
.AddAttribute ("w2","Weight for subflow2",
			  UintegerValue (1),
			  MakeUintegerAccessor (&MpWeightedRoundRobin::w2),
			  MakeUintegerChecker<uint32_t> ())
;
  return tid;
}

MpWeightedRoundRobin::MpWeightedRoundRobin()
:s1(0),s2(0)
{
	// TODO Auto-generated constructor stub

}

MpWeightedRoundRobin::~MpWeightedRoundRobin() {
	// TODO Auto-generated destructor stub
}

/*void
MpWeightedRoundRobin::SetSndBufSize (uint32_t size)
{
	  NS_LOG_FUNCTION(this<<size);
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
MpWeightedRoundRobin::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}*/


int
MpWeightedRoundRobin::Discard(SequenceNumber64 seq)
{
	//NS_LOG_FUNCTION(this);
	NS_LOG_FUNCTION(this << seq << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size());

    if (seq <= m_txBuffer.HeadSequence ())
	{ // Case 1: Old ACK, ignored.
	   NS_LOG_LOGIC ("Ignored ack of " << seq);
	   return -1;
	}
	else if (seq > m_txBuffer.HeadSequence ())
	{ // Case 3: New ACK, reset m_dupAckCount and update m_txBuffer
	   //NS_LOG_LOGIC ("New ack of " << seq);
	   m_txBuffer.DiscardUpTo(seq);
	   NS_LOG_LOGIC ("New ack of " << seq << " " <<m_txBuffer.Size());
	   MpScheduler::Discard(seq);
	   return 1;
	}
    return 0;

}

bool
MpWeightedRoundRobin::GetPacket(std::vector<Ptr<TcpSocketBase> > sockets)
{
	NS_LOG_FUNCTION(this << sockets.size() << sockets[0]->GetRtt()->GetCurrentEstimate().GetSeconds() << sockets[1]->GetRtt()->GetCurrentEstimate().GetSeconds() << sockets.size());

	std::map<SequenceNumber64,uint32_t>::iterator it;
	it=m_sizeTxMap.find(m_nextTxSequence);
	if(it!=m_sizeTxMap.end()){
		if(s1<w1){
			m_lastUsedsFlowIdx = 0;
			if(s1<w1)s1++;
			if(s1==w1)s2=0;
		} else if (s2<w2){
			m_lastUsedsFlowIdx = 1;
			if(s2<w2)s2++;
			if(s2==w2)s1=0;
		}
		NS_LOG_LOGIC(m_lastUsedsFlowIdx);
		//m_lastUsedsFlowIdx = (m_lastUsedsFlowIdx + 1) % sockets.size();
	    NS_LOG_LOGIC("Get " << it->second << " " << m_nextTxSequence << " " << sockets[m_lastUsedsFlowIdx]);
	    MapSequence(sockets[m_lastUsedsFlowIdx]);
	    m_nextTxSequence+=it->second;
		return true;
		//return m_txBuffer.CopyFromSequence (it->second, m_nextTxSequence);
	} else {
		return false;
	}



    return false;
}


} /* namespace ns3 */
