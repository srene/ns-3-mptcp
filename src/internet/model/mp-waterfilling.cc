/*
 * mp-scheduler.cc
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#include "mp-waterfilling.h"
#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"


NS_LOG_COMPONENT_DEFINE ("MpWaterfilling");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpWaterfilling);

TypeId
MpWaterfilling::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpWaterfilling")
.SetParent<MpScheduler> ()
.AddConstructor<MpWaterfilling> ()
/*.AddAttribute ("SndBufSize",
	               "TcpSocket maximum transmit buffer size (bytes)",
	               UintegerValue (2097152), // 256k
	               MakeUintegerAccessor (&MpWaterfilling::GetSndBufSize,
	                                     &MpWaterfilling::SetSndBufSize),
	               MakeUintegerChecker<uint32_t> ())*/
;
  return tid;
}

MpWaterfilling::MpWaterfilling()
: m_landaRate(0),
  m_lastSampleLanda(0),
  m_lastLanda(0),
  data(0)
{
	// TODO Auto-generated constructor stub

}

MpWaterfilling::~MpWaterfilling() {
	// TODO Auto-generated destructor stub
}

/*void
MpWaterfilling::SetSndBufSize (uint32_t size)
{
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
MpWaterfilling::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}*/
int
MpWaterfilling::Add(Ptr<Packet> p,uint32_t subflow)
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
MpWaterfilling::Discard(SequenceNumber64 seq)
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
	   NS_LOG_LOGIC ("New ack of " << seq << " " << m_txBuffer.Size());
	   MpScheduler::Discard(seq);
	   return 1;
	}
    return 0;

}

bool
MpWaterfilling::GetPacket(std::vector<Ptr<TcpSocketBase> > sockets)
{

	Ptr<Packet> packet;
	std::map<SequenceNumber64,uint32_t>::iterator it;
	it=m_sizeTxMap.find(m_nextTxSequence);



	//NS_LOG_LOGIC("Sockets size " << sockets.size());

	std::map <Ptr<TcpSocketBase>,double> times;
	/*for(std::vector<Ptr<TcpSocketBase> >::iterator sit = sockets.begin(); sit != sockets.end(); ++sit)
	{
		times.insert(std::pair <Ptr<TcpSocketBase>,double>(*sit,(double)Assigned(*sit)/(*sit)->GetCurrentBw() + (*sit)->GetLastRtt().GetSeconds()));
	}*/
	times.insert(std::pair <Ptr<TcpSocketBase>,double>(sockets[0],Assigned(sockets[0])/525679 + sockets[0]->GetLastRtt().GetSeconds()));
	times.insert(std::pair <Ptr<TcpSocketBase>,double>(sockets[0],Assigned(sockets[1])/162500 + sockets[1]->GetLastRtt().GetSeconds()));

	//If packet exists

	//NS_LOG_LOGIC("T1 " << t1_pending << " T2 " << t2_pending);

	Ptr<TcpSocketBase> tcp;
	uint32_t nextSubflow = -1;
	if (it!=m_sizeTxMap.end()){
		//NS_LOG_LOGIC("SizeMap");

		for(uint32_t i = 0; i<sockets.size();i++)
		{
			NS_LOG_LOGIC("Sockets " << sockets[i]->AvailableWindow() << " " << m_mtu << " "  << sockets.size() << " " << i << " " << (sockets[i]->AvailableWindow()>=m_mtu) << " " << (i<sockets.size()));
			if(sockets[i]->GetLastRtt().GetSeconds()==0||sockets[i]->GetCurrentBw()==0)
			{
			//	NS_LOG_LOGIC("Sockets2 " << m_lastUsedsFlowIdx);
				nextSubflow = (m_lastUsedsFlowIdx + 1) % sockets.size();
			//	NS_LOG_LOGIC("Sockets2 " << m_lastUsedsFlowIdx);
				break;

			}
			//NS_LOG_LOGIC("Window " << i << " " << sockets[i]->AvailableWindow());
			if(sockets[i]->AvailableWindow()>=m_mtu)
			{
				NS_LOG_LOGIC("Size " << Assigned(sockets[0]) << " BW " << sockets[0]->GetCurrentBw() << " time1 " <<  (double)Assigned(sockets[0])/sockets[0]->GetCurrentBw() << " " << times.find(sockets[0])->second << " rtt " << sockets[0]->GetLastRtt().GetSeconds() << " win " << sockets[0]->AvailableWindow());
				NS_LOG_LOGIC("Size " << Assigned(sockets[1]) << " BW " << sockets[1]->GetCurrentBw() << " time1 " <<  (double)Assigned(sockets[1])/sockets[1]->GetCurrentBw() << " " << times.find(sockets[1])->second << " rtt " << sockets[1]->GetLastRtt().GetSeconds() << " win " << sockets[1]->AvailableWindow());

				tcp = sockets[i];
				//NS_LOG_LOGIC("Sockets");
				NS_LOG_LOGIC("Sockets3");
				double t = times.find(sockets[i])->second;
				//Send traffic equaling latencies
				bool found=true;
				for(std::map <Ptr<TcpSocketBase>,double>::iterator sit = times.begin(); sit != times.end(); ++sit){
					NS_LOG_LOGIC("Sockets delay "<< sit->first << " " << t << " " << sit->second << " " << sit->first->AvailableWindow());
					if(t>sit->second){
						NS_LOG_LOGIC("found1");
						found=false;
						break;
					}
				}
				if(found){
					nextSubflow = i;
					NS_LOG_LOGIC("found2");
					break;
				}

			}


		}
		m_lastUsedsFlowIdx = nextSubflow;
		//Get packet to send
		if(m_lastUsedsFlowIdx!=-1){
			NS_LOG_LOGIC("Socket " << tcp << " sent packet");
		    MapSequence(sockets[m_lastUsedsFlowIdx]);
		    m_nextTxSequence+=it->second;
			return true;
		} else {
			NS_LOG_LOGIC("Socket " << tcp << " not sent packet");

			return false;

		}
    //If there is no packet to send, we create an empty packet
    } else {
    	return false;
    }

    return false;
}






} /* namespace ns3 */
