/*
 * mp-scheduler.cc
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#include "mp-waterfilling2.h"
#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"


NS_LOG_COMPONENT_DEFINE ("MpWaterfilling2");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpWaterfilling2);

TypeId
MpWaterfilling2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpWaterfilling2")
.SetParent<MpScheduler> ()
.AddConstructor<MpWaterfilling2> ()
/*.AddAttribute ("SndBufSize",
	               "TcpSocket maximum transmit buffer size (bytes)",
	               UintegerValue (2097152), // 256k
	               MakeUintegerAccessor (&MpWaterfilling2::GetSndBufSize,
	                                     &MpWaterfilling2::SetSndBufSize),
	               MakeUintegerChecker<uint32_t> ())*/
;
  return tid;
}

MpWaterfilling2::MpWaterfilling2()
: m_landaRate(0),
  m_lastSampleLanda(0),
  m_lastLanda(0),
  data(0)
{
	// TODO Auto-generated constructor stub

}

MpWaterfilling2::~MpWaterfilling2() {
	// TODO Auto-generated destructor stub
}

/*void
MpWaterfilling2::SetSndBufSize (uint32_t size)
{
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
MpWaterfilling2::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}*/
int
MpWaterfilling2::Add(Ptr<Packet> p,uint32_t subflow)
{
	NS_LOG_FUNCTION(this << p->GetSize() << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size());

	  //if(subflow==0)return 0;
	  if(!sent){
		  t = Simulator::Now();
		  sent = true;
	  }

	  if (!m_txBuffer.Add (p))
	  { // TxBuffer overflow, send failed
		 NS_LOG_FUNCTION("Buffer full");
		 return -1;
	  }
	  m_timeMap1.insert(TimePair(m_txBuffer.TailSequence()-p->GetSize(),Simulator::Now()));

	  data+= p->GetSize();
	  if(Simulator::Now()-t1>0){
		  m_landaRate = data / (Simulator::Now().GetSeconds()-t1.GetSeconds());
		  data = 0;
		  double alpha = 0.6;
		  double sample_landa = m_landaRate;
		  m_landaRate = (alpha * m_lastLanda) + ((1 - alpha) * ((sample_landa + m_lastSampleLanda) / 2));
		  m_lastSampleLanda = sample_landa;
		  m_lastLanda = sample_landa;
		  t1 = Simulator::Now();

	  }
		NS_LOG_LOGIC(this << " " << data << " " << m_landaRate<< " " << t1.GetSeconds());


	 m_sizeTxMap.insert(SizePair(m_txBuffer.TailSequence()-p->GetSize(),p->GetSize()));


  return 1;

}


int
MpWaterfilling2::Discard(SequenceNumber64 seq)
{
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
MpWaterfilling2::GetPacket(std::vector<Ptr<TcpSocketBase> > sockets)
{

	Ptr<Packet> packet;
	std::map<SequenceNumber64,uint32_t>::iterator it;
	it=m_sizeTxMap.find(m_nextTxSequence);

	std::map<SequenceNumber64,Time>::iterator timeIt;
	timeIt=m_timeMap1.find(m_nextTxSequence);
	double time1=0;

	if(timeIt!=m_timeMap1.end()){
		time1 = Simulator::Now().GetSeconds()-timeIt->second.GetSeconds();
		NS_LOG_LOGIC("TimeIte1 "<< m_nextTxSequence<< " "<< time1 <<  " " << ((double)sockets[0]->UnSentData() + m_mtu)/sockets[0]->GetCurrentBw() << " " << time1+((((double)sockets[0]->UnSentData() + m_mtu)/sockets[0]->GetCurrentBw())));
	}

	NS_LOG_LOGIC("Socketsnum " << sockets.size());
	//If packet exists
	NS_LOG_LOGIC("Size " << Assigned(sockets[0]) << " BW " << sockets[0]->GetCurrentBw() << " t1 " << (double)Assigned(sockets[0])/sockets[0]->GetCurrentBw() << " rtt " << sockets[0]->GetLastRtt().GetSeconds());
	NS_LOG_LOGIC("Size " << Assigned(sockets[1]) << " BW " << sockets[1]->GetCurrentBw() << " t2 " << (double)Assigned(sockets[1])/sockets[1]->GetCurrentBw() << " rtt " << sockets[1]->GetLastRtt().GetSeconds());

	//double t1_pending = Assigned(sockets[0])/sockets[0]->GetCurrentBw() + sockets[0]->GetRtt()->GetCurrentEstimate().GetSeconds();
	//double t2_pending = Assigned(sockets[1])/sockets[1]->GetCurrentBw() + sockets[1]->GetRtt()->GetCurrentEstimate().GetSeconds();

	double t1_pending = Assigned(sockets[0])/sockets[0]->GetCurrentBw() + sockets[0]->GetLastRtt().GetSeconds();
	double t2_pending = Assigned(sockets[1])/sockets[1]->GetCurrentBw() + sockets[1]->GetLastRtt().GetSeconds();

	//double t1_pending = (double)Assigned(sockets[0])/525679 + sockets[0]->GetRtt()->GetCurrentEstimate().GetSeconds();
	//double t2_pending = (double)Assigned(sockets[1])/162500 + sockets[1]->GetRtt()->GetCurrentEstimate().GetSeconds();

	double t1 = sockets[0]->GetLastRtt().GetSeconds();
	double t2 = sockets[1]->GetLastRtt().GetSeconds();


	NS_LOG_LOGIC("T1 " << t1_pending << " T2 " << t2_pending);

	NS_LOG_LOGIC("SizeMap1 " << sockets[0]->AvailableWindow() << " " << sockets[1]->AvailableWindow() << " " << m_mtu);

	if (it!=m_sizeTxMap.end()){

		NS_LOG_LOGIC("SizeMap2 " << sockets[0]->AvailableWindow() << " " << sockets[1]->AvailableWindow() << " " << m_mtu);
		if(t1==0||t2==0||sockets[0]->GetCurrentBw()==0||sockets[1]->GetCurrentBw()==0)
		//if((t1==0&&t2==0)||(sockets[0]->GetCurrentBw()==0&&sockets[1]->GetCurrentBw()==0))
		{
			NS_LOG_LOGIC("SizeMap3 " << sockets[0]->AvailableWindow() << " " << sockets[1]->AvailableWindow() << " " << m_mtu);

			/*m_lastUsedsFlowIdx = (m_lastUsedsFlowIdx + 1) % sockets.size();
			if(sockets[m_lastUsedsFlowIdx]->AvailableWindow()<m_mtu)
			{
				m_lastUsedsFlowIdx = (m_lastUsedsFlowIdx + 1) % sockets.size();
				if(sockets[m_lastUsedsFlowIdx]->AvailableWindow()<m_mtu){
					m_lastUsedsFlowIdx=-1;
					return false;
				}
			}*/
			//return false;
			m_lastUsedsFlowIdx = (m_lastUsedsFlowIdx + 1) % sockets.size();

		} else
		{

			//Send traffic equaling latencies
			if(t1_pending<t2_pending){
			//if(t1<t2){
				m_lastUsedsFlowIdx=0;
			}else{
				m_lastUsedsFlowIdx=1;
			}
		}

		//Get packet to send
		if(m_lastUsedsFlowIdx!=-1){
		    MapSequence(sockets[m_lastUsedsFlowIdx]);
		    m_nextTxSequence+=it->second;
			return true;
		} else {
			return false;

		}
    //If there is no packet to send, we create an empty packet
    } else {
    	return false;
    }

    return false;
}






} /* namespace ns3 */
