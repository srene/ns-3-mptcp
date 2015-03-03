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
.AddAttribute ("SndBufSize",
	               "TcpSocket maximum transmit buffer size (bytes)",
	               UintegerValue (2097152), // 256k
	               MakeUintegerAccessor (&MpRoundRobin::GetSndBufSize,
	                                     &MpRoundRobin::SetSndBufSize),
	               MakeUintegerChecker<uint32_t> ())
.AddAttribute ("c1", "Capacity channel 1",
			UintegerValue (150000),
			MakeUintegerAccessor (&MpRoundRobin::c1),
			MakeUintegerChecker<uint32_t> ())
.AddAttribute ("c2", "Capacity channel 2",
			UintegerValue (75000),
			MakeUintegerAccessor (&MpRoundRobin::c2),
			MakeUintegerChecker<uint32_t> ())
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
nextSubFlow(0),
data1(0),
data2(0)
{
	// TODO Auto-generated constructor stub

}

MpRoundRobin::~MpRoundRobin() {
	// TODO Auto-generated destructor stub
}

void
MpRoundRobin::SetSndBufSize (uint32_t size)
{
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
MpRoundRobin::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}
/*int
MpRoundRobin::Add(Ptr<Packet> packet,uint32_t subflow)
{

}*/
int
MpRoundRobin::Add(Ptr<Packet> p,uint32_t subflow)
{
	NS_LOG_FUNCTION(this << p->GetSize() << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size());

	  if (!m_txBuffer.Add (p))
	  { // TxBuffer overflow, send failed
		 //m_socket->SetError(Socket::ERROR_MSGSIZE);
		 NS_LOG_FUNCTION("Buffer full");
		 return -1;
	  }
	  m_sizeTxMap.insert(SizePair(m_txBuffer.TailSequence()-p->GetSize(),p->GetSize()));

	  t = Simulator::Now()-t;
	  data+= p->GetSize();
	  if(t.GetSeconds()>0){
		  m_landaRate = data / t.GetSeconds();
		  data = 0;
		  double alpha = 0.9;
		  /*double sample_landa = m_landaRate;
		  m_landaRate = (alpha * m_lastLanda) + ((1 - alpha) * ((sample_landa + m_lastSampleLanda) / 2));
		  m_lastSampleLanda = sample_landa;
		  m_lastLanda = sample_landa;*/

		  m_landaRate = (alpha * m_lastLanda) + ((1 - alpha) * m_landaRate);
		  //m_lastSampleLanda = sample_landa;
		  m_lastLanda = m_landaRate;
	  }
	  t = Simulator::Now();

  return 1;

}


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
	   NS_LOG_LOGIC ("New ack of " << seq);
	   m_txBuffer.DiscardUpTo(seq-1400);
	   return 1;
	}
    return 0;

}

Ptr<Packet>
MpRoundRobin::GetPacket(std::vector<Ptr<TcpSocketBase> > sockets)
{
	NS_LOG_FUNCTION(this << GetLanda(0)*8 << GetLanda(1)*8 << sockets.size() << m_lastUsedsFlowIdx);


	std::map<SequenceNumber64,uint32_t>::iterator it;
	it=m_sizeTxMap.find(m_nextTxSequence);
	if(it!=m_sizeTxMap.end()){
		nextSubFlow = (m_lastUsedsFlowIdx + 1) % sockets.size();
	    m_lastUsedsFlowIdx = nextSubFlow;
	    NS_LOG_LOGIC("Get " << it->second << " " << m_nextTxSequence);
		return m_txBuffer.CopyFromSequence (it->second, m_nextTxSequence);
	}


    switch(nextSubFlow)
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

	return Create<Packet>();

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
MpRoundRobin::SetNextSequence(SequenceNumber64 seq)
{
	m_nextTxSequence = seq;
}

int
//MpRoundRobin::GetSubflowToUse(SequenceNumber64 seq, std::vector<Ptr<TcpSocketBase> > sockets,uint32_t lastUsedsFlowIdx)
MpRoundRobin::GetSubflowToUse()
{
	NS_LOG_FUNCTION(this<<nextSubFlow);


    return nextSubFlow;
}


void
MpRoundRobin::Update(std::vector<Ptr<TcpSocketBase> > sockets)
{

}
} /* namespace ns3 */