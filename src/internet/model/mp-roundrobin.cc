/*
 * mp-scheduler.cc
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#include "mp-roundrobin.h"
#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"


NS_LOG_COMPONENT_DEFINE ("MpRoundRobin");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpRoundRobin);

TypeId
MpRoundRobin::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpRoundRobin")
.SetParent<MpScheduler> ()
.AddConstructor<MpRoundRobin> ()
/*.AddAttribute ("SndBufSize",
	               "TcpSocket maximum transmit buffer size (bytes)",
	               UintegerValue (2097152), // 256k
	               MakeUintegerAccessor (&MpRoundRobin::GetSndBufSize,
	                                     &MpRoundRobin::SetSndBufSize),
	               MakeUintegerChecker<uint32_t> ())*/
;
  return tid;
}

MpRoundRobin::MpRoundRobin()
: m_landaRate(0),
m_lastSampleLanda(0),
m_lastLanda(0),
data(0),
m_landa1Rate(0),
m_landa2Rate(0),
m_landa3Rate(0),
data1(0),
data2(0)
{
	// TODO Auto-generated constructor stub

}

MpRoundRobin::~MpRoundRobin() {
	// TODO Auto-generated destructor stub
}

/*void
MpRoundRobin::SetSndBufSize (uint32_t size)
{
	  NS_LOG_FUNCTION(this<<size);
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
MpRoundRobin::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}*/
/*int
MpRoundRobin::Add(Ptr<Packet> packet,uint32_t subflow)
{

}*/



int
MpRoundRobin::Discard(SequenceNumber64 seq)
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
	   NS_LOG_LOGIC ("New ack of " << seq << " " <<m_txBuffer.HeadSequence() << " " << m_txBuffer.Size());
	   m_txBuffer.DiscardUpTo(seq);
	   NS_LOG_LOGIC ("New ack of " << seq << " " <<m_txBuffer.HeadSequence() << " " << m_txBuffer.Size());
	   MpScheduler::Discard(seq);
	   return 1;
	}
    return 0;

}

bool
MpRoundRobin::GetPacket(std::vector<Ptr<TcpSocketBase> > sockets)
{

	NS_LOG_FUNCTION("Socket 0 " << sockets[0] << " socket 1 " << sockets[1]);
	NS_LOG_FUNCTION(this << GetLanda(0)*8 << GetLanda(1)*8 << sockets[0]->GetRtt()->GetCurrentEstimate().GetSeconds() << sockets[1]->GetRtt()->GetCurrentEstimate().GetSeconds() << sockets.size() << m_lastUsedsFlowIdx);

	std::map<SequenceNumber64,uint32_t>::iterator it;
	it=m_sizeTxMap.find(m_nextTxSequence);
	if(it!=m_sizeTxMap.end()){
		m_lastUsedsFlowIdx = (m_lastUsedsFlowIdx + 1) % sockets.size();
	    NS_LOG_LOGIC("Get " << it->second << " " << m_nextTxSequence << " " << sockets[m_lastUsedsFlowIdx]);
	    MapSequence(sockets[m_lastUsedsFlowIdx]);
	    m_nextTxSequence+=it->second;
		return true;
		//return m_txBuffer.CopyFromSequence (it->second, m_nextTxSequence);
	} else {
		return false;
	}

    switch(m_lastUsedsFlowIdx)
    {
    case 0:
    	data1+=it->second;
    	break;

    case 1:
    	data2+=it->second;
    	break;

    default:
    	break;
    }

    return false;
}


double
MpRoundRobin::GetLanda(uint32_t channel)
{
	//NS_LOG_LOGIC("landa11 " << m_landa11Rate << " landa12 " << m_landa12Rate);
	//NS_LOG_LOGIC("landa21 " << m_landa21Rate << " landa22 " << m_landa22Rate);

	double landa;
	double rate = data1+data2;
	double landarate;
	switch(channel)
	{
	case 0:
		landarate = (double)data1/rate;
		landa =  (double)m_landaRate*landarate;
		break;
	case 1:
		landarate = (double)data2/rate;
		landa =  (double)m_landaRate*landarate;
		break;
	default:
		return 0;
		break;

	}

	//NS_LOG_LOGIC("return landa  " << m_landaRate << " " << m_landaRate2 << " " << (double)landa);

	return landa;

}




void
MpRoundRobin::Update(std::vector<Ptr<TcpSocketBase> > sockets)
{

}
} /* namespace ns3 */
