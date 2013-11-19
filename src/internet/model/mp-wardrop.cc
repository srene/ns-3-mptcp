/*
 * mp-wardrop.cc
 *
 *  Created on: Aug 20, 2013
 *      Author: sergi
 */

#include "mp-wardrop.h"
#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"

NS_LOG_COMPONENT_DEFINE ("MpWardrop");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpWardrop);

TypeId
MpWardrop::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpWardrop")
.SetParent<MpScheduler> ()
.AddConstructor<MpWardrop> ()
.AddAttribute ("SndBufSize",
	               "TcpSocket maximum transmit buffer size (bytes)",
	               UintegerValue (262144), // 256k
	               MakeUintegerAccessor (&MpWardrop::GetSndBufSize,
	                                     &MpWardrop::SetSndBufSize),
	               MakeUintegerChecker<uint32_t> ())
.AddAttribute ("SndBufSize1",
			      "TcpSocket maximum transmit buffer size (bytes)",
			      UintegerValue (262144), // 256k
	               MakeUintegerAccessor (&MpWardrop::GetSndBufSize1,
	                                     &MpWardrop::SetSndBufSize1),
			      MakeUintegerChecker<uint32_t> ())
.AddAttribute ("SndBufSize2",
				   "TcpSocket maximum transmit buffer size (bytes)",
				   UintegerValue (262144), // 256k
	               MakeUintegerAccessor (&MpWardrop::GetSndBufSize2,
	                                     &MpWardrop::SetSndBufSize2),
				   MakeUintegerChecker<uint32_t> ())
.AddAttribute ("Limit1", "Flow one time limit in ms",
                UintegerValue (100),
                MakeUintegerAccessor (&MpWardrop::m_limit),
                MakeUintegerChecker<uint32_t> ())
.AddAttribute ("Limit2", "Flow one time limit in ms",
				UintegerValue (200),
				MakeUintegerAccessor (&MpWardrop::m_limit2),
				MakeUintegerChecker<uint32_t> ())
.AddAttribute ("Mtu", "Flow one time limit in ms",
				UintegerValue (1400),
				MakeUintegerAccessor (&MpWardrop::m_mtu),
				MakeUintegerChecker<uint32_t> ())
.AddAttribute ("c1", "Capacity channel 1",
				//UintegerValue (57750),
				UintegerValue(150000),
				MakeUintegerAccessor (&MpWardrop::c1),
				MakeUintegerChecker<uint32_t> ())
.AddAttribute ("c2", "Capacity channel 2",
				//UintegerValue (57750),
				UintegerValue(75000),
				MakeUintegerAccessor (&MpWardrop::c2),
				MakeUintegerChecker<uint32_t> ())
.AddTraceSource ("Buffer",
				"Buffer occupancy",
				MakeTraceSourceAccessor (&MpWardrop::m_buffer))
.AddTraceSource ("Buffer1",
				"Buffer occupancy",
				MakeTraceSourceAccessor (&MpWardrop::m_buffer1))
.AddTraceSource ("Buffer2",
				"Buffer occupancy",
				MakeTraceSourceAccessor (&MpWardrop::m_buffer2))
;
  return tid;
}

MpWardrop::MpWardrop()
: //m_limit(100),
  //m_limit2(200),
  m_alpha(0),
  m_beta(0),
  //m_landaRate11(0),
 // m_lastSampleLanda11(0),
  //m_lastLanda11(0),
  m_landaRate(0),
  m_lastLanda(0),
  data(0),
  m_landaRate1(0),
  m_lastLanda1(0),
  data1(0),
  m_landaRate2(0),
  m_lastLanda2(0),
  data2(0),
  /*data12(0),
  m_landaRate21(0),
  m_lastSampleLanda21(0),
  m_lastLanda21(0),
  data21(0),
  m_landaRate22(0),
  m_lastSampleLanda22(0),
  m_lastLanda22(0),
  data22(0),*/
  m_landaRate11(0),
  data11(0),
  m_landaRate12(0),
  data12(0),
  m_landaRate21(0),
  data21(0),
  m_landaRate22(0),
  data22(0),
  m_nextTxSequence(1),
  m_nextTxSequence1(1),
  m_nextTxSequence2(1),
  m_buffer(0),
  m_buffer1(0),
  m_buffer2(0),
  m_landa21(0),
  m_landa22(0),
  sent(false),
  nextSubFlow(0),
  update(false)
{

	drop=false;
	m_txBuffer1.SetHeadSequence(SequenceNumber64(1));
	m_txBuffer2.SetHeadSequence(SequenceNumber64(1));

}

MpWardrop::~MpWardrop() {
	// TODO Auto-generated destructor stub
}

int
MpWardrop::Add(Ptr<Packet> p,uint32_t subflow)
{
	 NS_LOG_FUNCTION(this << p->GetSize() << m_txBuffer1.Size () << m_txBuffer2.Size());

	  if(subflow==0)return 0;
	  if(!sent){
		  t = Simulator::Now();
		  sent = true;
	  }
	  /*if (!m_txBuffer.Add (p))
	  { // TxBuffer overflow, send failed
		 NS_LOG_FUNCTION("Buffer full");
		 return -1;
	  }
	  m_buffer = m_txBuffer.Size();
	  NS_LOG_FUNCTION(this << p->GetSize() << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size());
	  NS_LOG_LOGIC("TailSeq insert  " << m_txBuffer.TailSequence()-p->GetSize() << " " << p->GetSize());
	  m_sizeTxMap.insert(SizePair(m_txBuffer.TailSequence()-p->GetSize(),p->GetSize()));*/

	  t0 = Simulator::Now()-t0;
	  data+= p->GetSize();
	  if(t0.GetSeconds()>0){
		  m_landaRate = data / t0.GetSeconds();
		  data = 0;
		  double alpha = 0.4;
		  m_landaRate = (alpha * m_lastLanda) + ((1 - alpha) * m_landaRate);
		  m_lastLanda = m_landaRate;
	  }
	  t0 = Simulator::Now();
	  switch(subflow)
	  {

	  	  case 1:
	  	  {
			  SequenceNumber64 tailSeq1 = m_txBuffer1.TailSequence();
	  		  if (!m_txBuffer1.Add (p))
	  		  { // TxBuffer overflow, send failed
	  			 NS_LOG_FUNCTION("Buffer full");
	  			 return -1;
	  		  }

	  		  m_buffer1 = m_txBuffer1.Size();
	  		  m_nextTxSequence+=p->GetSize();
	  		  m_seqTxMap1.insert(SeqPair(m_nextTxSequence,m_txBuffer1.TailSequence()-p->GetSize()));
	  		  m_sizeTxMap1.insert(SizePair(m_txBuffer1.TailSequence()-p->GetSize(),p->GetSize()));
	  		  t1 = Simulator::Now()-t1;
	  		  data1+= p->GetSize();
	  		  if(t1.GetSeconds()>0){
	  			  m_landaRate1 = data1 / t1.GetSeconds();
	  			  data1 = 0;
	  			  double alpha = 0.4;
	  			  m_landaRate1 = (alpha * m_lastLanda1) + ((1 - alpha) * m_landaRate1);
	  			  m_lastLanda1 = m_landaRate1;
	  		  }
	  		  t1 = Simulator::Now();
	  		  break;
	  	  }
	  	  case 2:
	  	  {
			  SequenceNumber64 tailSeq2 = m_txBuffer2.TailSequence();
	  		  if (!m_txBuffer2.Add (p))
	  		  { // TxBuffer overflow, send failed
	  			 NS_LOG_FUNCTION("Buffer full");
	  			 return -1;
	  		  }
	  		  m_buffer2 = m_txBuffer2.Size();
	  		  m_nextTxSequence+=p->GetSize();
	  		  m_seqTxMap2.insert(SeqPair(m_nextTxSequence,m_txBuffer2.TailSequence()-p->GetSize()));
	  		  m_sizeTxMap2.insert(SizePair(m_txBuffer2.TailSequence()-p->GetSize(),p->GetSize()));
	  		  t2 = Simulator::Now()-t2;
	  		  data2+= p->GetSize();
	  		  if(t2.GetSeconds()>0){
	  			  m_landaRate2 = data2 / t2.GetSeconds();
	  			  data2 = 0;
	  			  double alpha = 0.4;
	  			  m_landaRate2 = (alpha * m_lastLanda2) + ((1 - alpha) * m_landaRate2);
	  			  m_lastLanda2 = m_landaRate2;
	  		  }
	  		  t2 = Simulator::Now();
	  		  //m_seqTxMap2.insert(SeqPair(m_txBuffer.TailSequence()-p->GetSize(),m_txBuffer2.TailSequence()-p->GetSize()));

	  		  break;
	  	  }
	  	  default:
	  		  break;
	  }

	  //std::map<SequenceNumber64,uint32_t>::iterator it;

	 /* for(std::map<SequenceNumber64,uint32_t>::iterator iter = m_sizeTxMap1.begin(); iter != m_sizeTxMap1.end(); iter++)
	  	{
			  NS_LOG_LOGIC("Seq buffer1 " << iter->first << " "<< iter->second);
	  	}


	  for(std::map<SequenceNumber64,uint32_t>::iterator iter = m_sizeTxMap2.begin(); iter != m_sizeTxMap2.end(); iter++)
	  	{
			  NS_LOG_LOGIC("Seq buffer2 " << iter->first << " "<< iter->second);
	  	}*/

  return 1;

}


void
MpWardrop::SetMtu(uint32_t mtu)
{

}
int
MpWardrop::Discard(SequenceNumber64 seq)
{
	//NS_LOG_FUNCTION(this);
	 NS_LOG_FUNCTION(this << seq <<  m_txBuffer1.Size () << m_txBuffer2.Size());
	//NS_LOG_FUNCTION(this << seq << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size());


	 /*for(std::map<SequenceNumber64,SequenceNumber64>::iterator iter = m_seqTxMap1.begin(); iter != m_seqTxMap1.end(); iter++)
	  	{
			  NS_LOG_LOGIC("Seq buffer1 " << iter->first << " "<< iter->second);
	  	}


	  for(std::map<SequenceNumber64,SequenceNumber64>::iterator iter = m_seqTxMap2.begin(); iter != m_seqTxMap2.end(); iter++)
	  	{
			  NS_LOG_LOGIC("Seq buffer2 " << iter->first << " "<< iter->second);
	  	}*/

	std::map<SequenceNumber64,SequenceNumber64>::iterator iter;
	std::map<SequenceNumber64,SequenceNumber64>::iterator iter2;

	iter=m_seqTxMap1.find(seq);
	iter2=m_seqTxMap2.find(seq);

	if(iter != m_seqTxMap1.end()){
		NS_LOG_LOGIC("Buffer 1 " << iter->second);
		m_seqTxMap1.erase(iter);
		m_txBuffer1.DiscardUpTo(iter->second);
	} else if(iter2 != m_seqTxMap2.end()){
		NS_LOG_LOGIC("Buffer 2 " << iter2->second);
		m_seqTxMap2.erase(iter2);
		m_txBuffer2.DiscardUpTo(iter2->second);
	}
	 NS_LOG_FUNCTION(this << seq <<  m_txBuffer1.Size () << m_txBuffer2.Size());

    return 0;
}

Ptr<Packet>
MpWardrop::GetPacket(std::vector<Ptr<TcpSocketBase> > sockets)
{
	NS_LOG_FUNCTION(this<<m_nextTxSequence1 << m_nextTxSequence2 << m_txBuffer1.TailSequence() << m_txBuffer1.Size()<< m_txBuffer2.TailSequence() << m_txBuffer2.Size());
	nextSubFlow=-1;
	Ptr<Packet> packet;
	//std::map<SequenceNumber64,SequenceNumber64>::iterator it;
	std::map<SequenceNumber64,uint32_t>::iterator it2;

	if(m_txBuffer1.Size()==0&&m_txBuffer2.Size()==0)return Create<Packet>();
	//if(m_txBuffer1.TailSequence()==m_nextTxSequence1&&m_txBuffer2.TailSequence()==m_nextTxSequence2)return Create<Packet>();
	for(uint32_t i=0;i<sockets.size();i++){
	//	NS_LOG_LOGIC("RTT " << sockets[i]->GetRtt()->GetCurrentEstimate().GetSeconds() << " " << sockets[i]->GetPending());

    	if(m_rtt<sockets[i]->GetMinRtt()){
    		m_rtt = Seconds(2*sockets[i]->GetMinRtt().GetSeconds());
    		NS_LOG_LOGIC(m_rtt.GetSeconds() << " " << sockets[i]->GetMinRtt());
    	}
	}

    if(!m_IsCount&&m_rtt.GetSeconds()>0){
	  NS_LOG_LOGIC("Waterfilling event " <<  m_rtt.GetSeconds());
	  m_IsCount = true;
	  m_bwEstimateEvent.Cancel();
	  m_bwEstimateEvent = Simulator::Schedule (Seconds(2*m_rtt.GetSeconds()), &MpWardrop::Update,this,sockets);
    }
	//NS_LOG_LOGIC("Subflow size " << (uint32_t)sockets.size() << " seq " << seq);
	NS_LOG_LOGIC("Landa11 " << GetLanda(0,0)*8 << " alphac1 " << (double)m_alpha*c1*8);
	NS_LOG_LOGIC("Landa12 " << GetLanda(0,1)*8 << " betac2 " << (double)m_beta*c2*8);
	NS_LOG_LOGIC("Landa21 " << GetLanda(1,0)*8 << " 1-alphac1 " << (double)(1-m_alpha)*c1*8);
	NS_LOG_LOGIC("Landa22 " << GetLanda(1,1)*8 << " 1-betac2 " << (double)(1-m_beta)*c2*8);
	//SequenceNumber64 seq;

	if(m_txBuffer1.TailSequence()>m_nextTxSequence1){
		nextSubFlow = (m_lastUsedsFlowIdx + 1) % sockets.size();
		m_lastUsedsFlowIdx = nextSubFlow;
		it2=m_sizeTxMap1.find(m_nextTxSequence1);
		NS_LOG_LOGIC("seq:" <<m_nextTxSequence1<<" "<<it2->second);
		packet =  m_txBuffer1.CopyFromSequence (it2->second,m_nextTxSequence1);
		//m_txBuffer1.DiscardUpTo(m_nextTxSequence1);
		//m_nextTxSequence1+=it2->second;
		NS_LOG_LOGIC("Nextsubflow1 " << nextSubFlow);

		switch(m_lastUsedsFlowIdx){
			case 0:
				data11+= it2->second;
				break;
			case 1:
				data12+= it2->second;
				break;
			default:
				break;
		}
		NS_LOG_LOGIC("data11:" << data11 << " data12:" << data12);


	} else if(m_txBuffer2.TailSequence()>m_nextTxSequence2){

			it2=m_sizeTxMap2.find(m_nextTxSequence2);
			packet =  m_txBuffer2.CopyFromSequence (it2->second,m_nextTxSequence2);
			//m_txBuffer2.DiscardUpTo(m_nextTxSequence2);
			//m_nextTxSequence2+=it2->second;

			//NS_LOG_LOGIC("Landarate2: " << m_landaRate2);
			//double landa21 = (1-m_alpha)*c1 - (double)(m_limit2/1000)*m_mtu;
			//double landa22 = (1-m_beta)*c2 - (double)(m_limit2/1000)*m_mtu;
			//NS_LOG_LOGIC("landa21: " << (1-m_alpha)*c1 << " " << (double)m_mtu/((double)m_limit/1000));
			//double landa21 = ((1-m_alpha)*c1) - ((double)m_mtu/((double)m_limit/1000));
			//double landa22 = ((1-m_beta)*c2) - ((double)m_mtu/((double)m_limit/1000));
			NS_LOG_LOGIC("data21:" << data21 << " data22:" << data22 << " landa21:" << m_landa21 << " landa22:" << m_landa22);
			//NS_LOG_LOGIC("Landa22:" << GetLanda(1,1)*8 << " Landa21:" << GetLanda(1,0)*8 << " lastsubflow:" << m_lastUsedsFlowIdx);
			if(m_landa21!=0&&m_landa22!=0){
				if(m_lastUsedsFlowIdx2==0){
					if((data22+it2->second)<m_landa22){
						nextSubFlow = 1;
					} else if((data21+it2->second)<m_landa21) {
						nextSubFlow = 0;
					}
				} else {
					if((data21+it2->second)<m_landa21){
						nextSubFlow = 0;
					} else if((data22+it2->second)<m_landa22) {
						nextSubFlow = 1;
					}

				}
				m_lastUsedsFlowIdx2 = nextSubFlow;
			} else {
				nextSubFlow = (m_lastUsedsFlowIdx2 + 1) % sockets.size();
			}

			NS_LOG_LOGIC("Nextsubflow2 " << nextSubFlow);
			if(nextSubFlow!=-1){
				m_lastUsedsFlowIdx2 = nextSubFlow;

				switch(m_lastUsedsFlowIdx2){
					case 0:
						data21+= it2->second;
						break;
					case 1:
						data22+= it2->second;
						break;
					default:
						break;
				}
			} else {
				NS_LOG_LOGIC("Packet not sent");
				packet = Create<Packet>();
				std::map<SequenceNumber64,uint32_t>::iterator iter;
				iter=m_sizeTxMap2.find(m_nextTxSequence2);
				//NS_LOG_FUNCTION(seq << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size() << iter->second);
				//NS_LOG_FUNCTION(m_nextTxSequence2 << m_txBuffer2.HeadSequence () << m_txBuffer2.TailSequence () << m_txBuffer2.Size() << iter->second);
				//SequenceNumber64 sequence = seq + SequenceNumber64(iter->second);
				//m_txBuffer.Remove (sequence,iter->second);
				//m_txBuffer2.Remove (m_nextTxSequence2,iter->second);
				//sequence = SequenceNumber64(it->second) + SequenceNumber64(iter->second);
				//m_txBuffer2.Remove (sequence,iter->second);
				//m_txBuffer2.Remove (m_nextTxSequence2,it2->second);
				//m_seqTxMap2.erase(m_nextTxSequence2);
				//m_sizeTxMap2.erase(m_nextTxSequence2);
				//NS_LOG_FUNCTION(seq << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size() << iter->second);
				//NS_LOG_FUNCTION(m_nextTxSequence2 << m_txBuffer2.HeadSequence () << m_txBuffer2.TailSequence () << m_txBuffer2.Size() << iter->second);
			}
	} else {
		packet = Create<Packet>();
	}

	return packet;
}

void
MpWardrop::SetNextSequence(SequenceNumber64 seq)
{
	std::map<SequenceNumber64,SequenceNumber64>::iterator iter;
	std::map<SequenceNumber64,SequenceNumber64>::iterator iter2;

	std::map<SequenceNumber64,uint32_t>::iterator it;
	iter=m_seqTxMap1.find(seq);
	iter2=m_seqTxMap2.find(seq);

	if(iter != m_seqTxMap1.end()){
		it=m_sizeTxMap1.find(iter->second);
		NS_LOG_LOGIC("m_nextTxSequence1 " << m_nextTxSequence1 << " " << it->second);

		m_nextTxSequence1 += it->second;
	} else if(iter2 != m_seqTxMap2.end()){
		it=m_sizeTxMap2.find(iter2->second);
		NS_LOG_LOGIC("m_nextTxSequence2 " << m_nextTxSequence2 << " " << it->second);
		m_nextTxSequence2 += it->second;
	}

}
void
MpWardrop::SetSndBufSize (uint32_t size)
{
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
MpWardrop::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}
void
MpWardrop::SetSndBufSize1 (uint32_t size)
{
  m_txBuffer1.SetMaxBufferSize (size);
}

uint32_t
MpWardrop::GetSndBufSize1 (void) const
{
  return m_txBuffer1.MaxBufferSize ();
}
void
MpWardrop::SetSndBufSize2 (uint32_t size)
{
  m_txBuffer2.SetMaxBufferSize (size);
}

uint32_t
MpWardrop::GetSndBufSize2 (void) const
{
  return m_txBuffer2.MaxBufferSize ();
}

int
MpWardrop::GetSubflowToUse()
{

    return nextSubFlow;
}

void
MpWardrop::Update(std::vector<Ptr<TcpSocketBase> > sockets)
{
	NS_LOG_FUNCTION(this);
	bweMap.clear();
	landaMap.clear();

//	for(uint32_t i=0;i<sockets.size();i++){
	 NS_LOG_LOGIC("socket 1 tx " << (double)1460/c1 << " socket 2 tx " << (double)1460/c2 << " socket 1 pending " << (double)sockets[0]->GetPending()/c1 << " socket 2 pending " << (double)sockets[1]->GetPending()/c2);
	 NS_LOG_LOGIC("buffer 1 size " << m_txBuffer1.SizeFromSequence(m_nextTxSequence1) << " buffer2 size " << m_txBuffer2.SizeFromSequence(m_nextTxSequence2));
	   NS_LOG_LOGIC("RTT t1 c1 " << ((double)sockets[0]->GetPending()/c1)+((double)m_txBuffer1.SizeFromSequence(m_nextTxSequence1)/c1)+((double)1400/c1));
	   NS_LOG_LOGIC("RTT t1 c2" << (double)sockets[1]->GetPending()/c2+(double)m_txBuffer1.SizeFromSequence(m_nextTxSequence1)/c2+(double)1400/c2);
	   NS_LOG_LOGIC("RTT t2 c1" << (double)sockets[0]->GetPending()/c1+(double)m_txBuffer2.SizeFromSequence(m_nextTxSequence2)/c1+(double)1400/c1);
	   NS_LOG_LOGIC("RTT t2 c2" << (double)sockets[1]->GetPending()/c2+(double)m_txBuffer2.SizeFromSequence(m_nextTxSequence2)/c2+(double)1400/c2);

//	}
	update=true;
	/*for(uint32_t i=0; i < sockets.size(); i++){
	   NS_LOG_LOGIC ("bw" << i << " " << sockets[i]->GetCurrentBw()*8);
	   if(sockets[i]->GetCurrentBw()>0)
		   bweMap.insert(std::pair<double,uint32_t>(sockets[i]->GetCurrentBw(),i));
	}*/
	//m_landaRate22 = m_landaRate21 = 0;
	NS_LOG_LOGIC ("bw" << 0 << " " << c1*8);
	NS_LOG_LOGIC ("bw" << 1 << " " << c2*8);

	bweMap.insert(std::pair<double,uint32_t>(c1,0));
	bweMap.insert(std::pair<double,uint32_t>(c2,0));


	m_IsCount = false;

	NS_LOG_LOGIC("landa11 " << (double)GetLanda(0,0)*8 << " landa12 " << (double)GetLanda(0,1)*8 << " landa 1 " << (double)GetLanda(0,0)*8+(double)GetLanda(0,1)*8 << " " << m_landaRate1*8);
	NS_LOG_LOGIC("landa21 " << (double)GetLanda(1,0)*8 << " landa22 " << (double)GetLanda(1,1)*8 << " landa 2 " << (double)GetLanda(1,0)*8+(double)GetLanda(1,1)*8 << " " << m_landaRate2*8);
	NS_LOG_LOGIC("landa_total " << (double)GetLanda(0,0)*8+(double)GetLanda(0,1)*8+(double)GetLanda(1,0)*8+(double)GetLanda(1,1)*8 << " " << m_landaRate*8);

	if((double)GetLanda(0,0)!=0&&(double)GetLanda(0,1)!=0){
		m_alpha = ((double)(m_mtu / ((double)m_limit/1000)) + (double)GetLanda(0,0)) / c1;
		m_beta = ((double)(m_mtu / ((double)m_limit/1000)) + (double)GetLanda(0,1)) / c2;

		NS_LOG_LOGIC("Alpha " << m_alpha << " beta " << m_beta);

		//NS_LOG_LOGIC ("Landa11 " << (double)GetLanda(0,0)*8 << " time " << (double)1400/(double)((m_alpha*c1)-((double)GetLanda(0,0))) << " "<< m_alpha*c1*8);
		//NS_LOG_LOGIC ("Landa12 " << (double)GetLanda(0,1)*8 << " time " << (double)1400/(double)((m_beta*c2)-((double)GetLanda(0,1))) << " "<< m_beta*c2*8);
		//NS_LOG_LOGIC ("Landa21 " << (double)GetLanda(1,0)*8 << " time " << (double)1400/(double)(((1-m_alpha)*c1)-((double)GetLanda(1,0))) << " "<< (1-m_alpha)*c1*8);
		//NS_LOG_LOGIC ("Landa22 " << (double)GetLanda(1,1)*8 << " time " << (double)1400/(double)(((1-m_beta)*c2)-((double)GetLanda(1,1)))<< " "<< (1-m_beta)*c2*8);

		m_landa21 = (((1-m_alpha)*c1) - ((double)m_mtu/((double)m_limit2/1000)))*(Simulator::Now().GetSeconds()-t.GetSeconds());
		m_landa22 = (((1-m_beta)*c2) - ((double)m_mtu/((double)m_limit2/1000)))*(Simulator::Now().GetSeconds()-t.GetSeconds());

		NS_LOG_LOGIC("landa21 " << m_landa21 << " landa22 " << m_landa22 << " " << Simulator::Now().GetSeconds()-t.GetSeconds());

		data11=data12=data21=data22=0;
		t=Simulator::Now();
		if(m_landa21<0)m_landa21=0;
		if(m_landa22<0)m_landa22=0;
	}
}

double
MpWardrop::GetLanda(uint32_t flow, uint32_t channel)
{
	//NS_LOG_LOGIC("landa11 " << m_landa11Rate << " landa12 " << m_landa12Rate);
	//NS_LOG_LOGIC("landa21 " << m_landa21Rate << " landa22 " << m_landa22Rate);

	double landa;
	//double rate1 = m_landa11Rate+m_landa12Rate;
	//double rate2 = m_landa21Rate+m_landa22Rate;

	switch(flow)
	{
	case 0:
		//if(rate1==0)return 0;
		//else{
			if(channel==0)
			{
			//	NS_LOG_LOGIC("landa rate11 " << (double)data11 << " " << Simulator::Now().GetSeconds()-t.GetSeconds());
				//double landarate = (double)m_landa11Rate/rate1;
				landa =  (double)data11/(Simulator::Now().GetSeconds()-t.GetSeconds());
			} else if (channel==1){
			//	NS_LOG_LOGIC("landa rate12 " << (double)data12 << " " << Simulator::Now().GetSeconds()-t.GetSeconds());
				//double landarate = (double)m_landa12Rate/rate1;
				landa =  (double)data12/(Simulator::Now().GetSeconds()-t.GetSeconds());
			}
		//}
		break;
	case 1:
		//if(rate2==0)return 0;
		//else {
			if(channel==0)
			{
			//	NS_LOG_LOGIC("landa rate21 " << (double)data21 << " " << Simulator::Now().GetSeconds()-t.GetSeconds());
				landa =  (double)data21/(Simulator::Now().GetSeconds()-t.GetSeconds());
			} else if (channel==1){
			//	NS_LOG_LOGIC("landa rate22 " << (double)data22 << " " << Simulator::Now().GetSeconds()-t.GetSeconds());
				landa =  (double)data22/(Simulator::Now().GetSeconds()-t.GetSeconds());
			}
		//}

		break;
	default:
		return 0;
		break;

	}

	//NS_LOG_LOGIC("return landa  " << m_landaRate << " " << m_landaRate2 << " " << (double)landa);

	return landa;

}

/*uint32_t
MpWardrop::GetSubflowToUse(uint8_t subflow, uint32_t lastUsedsFlowIdx)
{
	uint32_t nextSubflow;
    if(m_subflowsToUse.size()==0)nextSubflow = (lastUsedsFlowIdx + 1) % sockets.size();
    else
	{
    	bool found=false;
    	nextSubflow = (lastUsedsFlowIdx + 1) % sockets.size();
    	while(!found){
			NS_LOG_LOGIC("Subflow to use while " << nextSubflow);
			for(uint32_t i = 0; i<m_subflowsToUse.size();i++){
				NS_LOG_LOGIC("Subflow to use for " <<  m_subflowsToUse[i]);
				if(nextSubflow == m_subflowsToUse[i]){
					NS_LOG_LOGIC("Subflow to use found " <<  m_subflowsToUse[i]);
					found=true;
					break;
				}
			}
			if(!found)nextSubflow = (nextSubflow + 1) % sockets.size();
    	}

	}


	switch(subflow){
	case 0:
	case 1:
		if(nextSubflow==0){
			m_landa11Rate++;
		} else if(nextSubflow==1){
			m_landa12Rate++;
		}
		break;
	case 2:
		if(nextSubflow==0){
			m_landa21Rate++;
		} else if(nextSubflow==1){
			m_landa22Rate++;
		}
		break;

	default:

		break;
	}

	return nextSubflow;
}*/

}
