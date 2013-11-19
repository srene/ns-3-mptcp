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
.AddAttribute ("SndBufSize",
	               "TcpSocket maximum transmit buffer size (bytes)",
	               UintegerValue (4194304), // 256k
	               MakeUintegerAccessor (&MpWaterfilling::GetSndBufSize,
	                                     &MpWaterfilling::SetSndBufSize),
	               MakeUintegerChecker<uint32_t> ())
.AddAttribute ("c1", "Capacity channel 1",
			//UintegerValue (187500),
			UintegerValue(150000),
			MakeUintegerAccessor (&MpWaterfilling::c1),
			MakeUintegerChecker<uint32_t> ())
.AddAttribute ("c2", "Capacity channel 2",
			//UintegerValue (125000),
			UintegerValue(75000),
			MakeUintegerAccessor (&MpWaterfilling::c2),
			MakeUintegerChecker<uint32_t> ())
/*.AddAttribute ("c3", "Capacity channel 2",
			UintegerValue (62500),
			MakeUintegerAccessor (&MpWaterfilling::c3),
			MakeUintegerChecker<uint32_t> ())*/
;
  return tid;
}

MpWaterfilling::MpWaterfilling()
: m_landaRate(0),
m_lastSampleLanda(0),
m_lastLanda(0),
data(0),
m_landa1Rate(0),
m_landa2Rate(0),
m_landa3Rate(0),
data1(0),
data2(0),
nextSubFlow(0),
sent(false),
update(false),
m_lastUsedsFlowIdx(0)
{
	// TODO Auto-generated constructor stub

}

MpWaterfilling::~MpWaterfilling() {
	// TODO Auto-generated destructor stub
}

void
MpWaterfilling::SetSndBufSize (uint32_t size)
{
  m_txBuffer.SetMaxBufferSize (size);
}

void
MpWaterfilling::SetMtu(uint32_t mtu)
{

}
uint32_t
MpWaterfilling::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}
int
MpWaterfilling::Add(Ptr<Packet> p,uint32_t subflow)
{
	NS_LOG_FUNCTION(this << p->GetSize() << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size());


	  if(subflow==0)return 0;
	  if(!sent){
		  t = Simulator::Now();
		  sent = true;
	  }

	  if (!m_txBuffer.Add (p))
	  { // TxBuffer overflow, send failed
		 //m_socket->SetError(Socket::ERROR_MSGSIZE);
		 NS_LOG_FUNCTION("Buffer full");
		 return -1;
	  }
	 m_sizeTxMap.insert(SizePair(m_txBuffer.TailSequence()-p->GetSize(),p->GetSize()));

	  /*t = Simulator::Now()-t;
	  data+= p->GetSize();
	  if(t.GetSeconds()>0){
		  m_landaRate = data / t.GetSeconds();
		  data = 0;
		  double alpha = 0.9;
		  //double sample_landa = m_landaRate;
		  //m_landaRate = (alpha * m_lastLanda) + ((1 - alpha) * ((sample_landa + m_lastSampleLanda) / 2));
		  //m_lastSampleLanda = sample_landa;
		  //m_lastLanda = sample_landa;

		  m_landaRate = (alpha * m_lastLanda) + ((1 - alpha) * m_landaRate);
		  //m_lastSampleLanda = sample_landa;
		  m_lastLanda = m_landaRate;
	  }
	  t = Simulator::Now();*/

  return 1;

}

/*int
MpWaterfilling::Add(Ptr<Packet> packet,uint32_t subflow)
{

}*/

int
MpWaterfilling::Discard(SequenceNumber64 seq)
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
MpWaterfilling::GetPacket(std::vector<Ptr<TcpSocketBase> > sockets)
{
	NS_LOG_FUNCTION(this << GetLanda(0)*8 << GetLanda(1)*8 << sockets.size() << m_nextTxSequence);

	Ptr<Packet> packet;
	std::map<SequenceNumber64,uint32_t>::iterator it;
	it=m_sizeTxMap.find(m_nextTxSequence);
	if(it!=m_sizeTxMap.end()){
		packet =  m_txBuffer.CopyFromSequence (it->second, m_nextTxSequence);
	} else {
		return Create<Packet>();
	}
	NS_LOG_LOGIC("last subflow " << m_lastUsedsFlowIdx << " " << nextSubFlow << " " << sockets.size());

	for(uint32_t i=0;i<sockets.size();i++){

    	if(m_rtt<sockets[i]->GetMinRtt()){
    		m_rtt = Seconds(2*sockets[i]->GetMinRtt().GetSeconds());
    		NS_LOG_LOGIC(m_rtt.GetSeconds() << " " << sockets[i]->GetMinRtt());
    	}
	}

    if(!m_IsCount&&m_rtt.GetSeconds()>0){
	  NS_LOG_LOGIC("Waterfilling event " <<  m_rtt.GetSeconds());
	  m_IsCount = true;
	  m_bwEstimateEvent.Cancel();
	  m_bwEstimateEvent = Simulator::Schedule (m_rtt, &MpWaterfilling::Update,this,sockets);
    }
    if(update==false){
    	nextSubFlow = (m_lastUsedsFlowIdx + 1) % sockets.size();
    	NS_LOG_LOGIC("Not update " << m_lastUsedsFlowIdx << " " << nextSubFlow << " " << sockets.size());

    } else {
		/*for(std::map<uint32_t,double>::iterator it = landaMap.begin (); it != landaMap.end (); ++it)
		{
			NS_LOG_LOGIC ("landa"<< it->first << " " << GetLanda(it->first)*8);
			NS_LOG_LOGIC ("landa  " <<  it->second*8);
			if(data1 < it->second){
				nextSubFlow=it->first;
				break;
			}
		}*/
    	std::map<uint32_t,double>::iterator iter = landaMap.find(0);
    	if(data1+it->second<iter->second)nextSubFlow=0;
    	else nextSubFlow=1;
    }
    m_lastUsedsFlowIdx = nextSubFlow;
	NS_LOG_LOGIC("Nextsubflow " << nextSubFlow);
	switch(nextSubFlow){
	case 0:
		data1+=it->second;
		NS_LOG_LOGIC("data1 " << data1);

		break;
	case 1:
		data2+=it->second;
		NS_LOG_LOGIC("data2 " << data2);

		break;
	default:

		break;
	}


	//NS_LOG_LOGIC ("landa3 " << GetLanda(2)*8);

	return packet;
}


double
MpWaterfilling::GetLanda(uint32_t channel)
{
	//NS_LOG_LOGIC("landa11 " << m_landa11Rate << " landa12 " << m_landa12Rate);
	//NS_LOG_LOGIC("landa21 " << m_landa21Rate << " landa22 " << m_landa22Rate);

	double landa;
	//double rate = m_landa1Rate+m_landa2Rate+m_landa3Rate;
	//double landarate;
	switch(channel)
	{
	case 0:
		landa = (double)data1/(Simulator::Now().GetSeconds()-t.GetSeconds());
		break;
	case 1:
		landa = (double)data2/(Simulator::Now().GetSeconds()-t.GetSeconds());
		break;
	default:
		return 0;
		break;

	}

	//NS_LOG_LOGIC("return landa  " << m_landaRate << " " << m_landaRate2 << " " << (double)landa);

	return landa;

}

void
MpWaterfilling::SetNextSequence(SequenceNumber64 seq)
{
	m_nextTxSequence = seq;
}

int
//MpWaterfilling::GetSubflowToUse(SequenceNumber64 seq, std::vector<Ptr<TcpSocketBase> > sockets,uint32_t lastUsedsFlowIdx)
MpWaterfilling::GetSubflowToUse()
{
	NS_LOG_FUNCTION(this<<nextSubFlow);
    return nextSubFlow;
}


void
MpWaterfilling::Update(std::vector<Ptr<TcpSocketBase> > sockets)
{
	NS_LOG_FUNCTION(this);
	if(update==false)update=true;
	bweMap.clear();
	landaMap.clear();

	NS_LOG_LOGIC ("bw" << 0 << " " << c1*8);
	NS_LOG_LOGIC ("bw" << 1 << " " << c2*8);
	NS_LOG_LOGIC ("landa1 " << data1 << " " << (Simulator::Now().GetSeconds()-t.GetSeconds()) << " " << GetLanda(0)*8);
	NS_LOG_LOGIC ("landa2 " << data2 << " " << (Simulator::Now().GetSeconds()-t.GetSeconds()) << " " <<GetLanda(1)*8);


	bweMap.insert(std::pair<double,uint32_t>(c1,0));
	bweMap.insert(std::pair<double,uint32_t>(c2,1));


	double landa = (data1+data2)/(Simulator::Now().GetSeconds()-t.GetSeconds());
	NS_LOG_LOGIC("Landa " << landa*8);
	int s = 1;
	for(std::map<double,uint32_t>::reverse_iterator i = bweMap.rbegin (); i != bweMap.rend (); i++)
	{

		double c = 0;
		std::map<double,uint32_t>::reverse_iterator j = i;
		j++;
		if(j!=bweMap.rend()){
		    c= i->first - j->first;
		    if(c>landa)c = landa;
		} else {
			c = landa;
		}
		//NS_LOG_LOGIC("Landac " << landa*8 << " c " << c*8);
		landa-=c;
		//NS_LOG_LOGIC("Landac " << landa*8 << " c " << c*8);
		for(std::map<uint32_t,double>::iterator it = landaMap.begin (); it != landaMap.end (); ++it)
		{
			landaMap.erase(it);
			//NS_LOG_LOGIC("Landa " << it->first << " " << it->second*8+(c/s));
			landaMap.insert(std::pair<uint32_t,double>(it->first,it->second+(c/s)));

		}
		//NS_LOG_LOGIC("Landa " << i->second << " " << c*8/s);

		landaMap.insert(std::pair<uint32_t,double>(i->second,c/s));

		s++;
		//NS_LOG_LOGIC ("Sent landa " << landa*8 << " c total " << c*8);


	}
	for(std::map<uint32_t,double>::iterator it = landaMap.begin (); it != landaMap.end (); ++it)
	{
		NS_LOG_LOGIC("channel " << it->first << " landa " << it->second*8);
		landaMap.erase(it);
		NS_LOG_LOGIC("Landa " << it->first << " " << (double)it->second*(Simulator::Now().GetSeconds()-t.GetSeconds()));
		landaMap.insert(std::pair<uint32_t,double>(it->first,((double)it->second*(Simulator::Now().GetSeconds()-t.GetSeconds()))));
	}
	t= Simulator::Now();
	data1=data2=0;
	m_IsCount = false;

}
} /* namespace ns3 */
