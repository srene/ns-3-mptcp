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
	               UintegerValue (2097152), // 256k
	               MakeUintegerAccessor (&MpWaterfilling::GetSndBufSize,
	                                     &MpWaterfilling::SetSndBufSize),
	               MakeUintegerChecker<uint32_t> ())
;
  return tid;
}

MpWaterfilling::MpWaterfilling()
: /*m_landaRate(0),
  m_lastSampleLanda(0),
  m_lastLanda(0),*/
  data(0),
  /*m_landaRate1(0),
  m_lastSampleLanda1(0),
  m_lastLanda1(0),
  m_landaRate2(0),
  m_lastSampleLanda2(0),
  m_lastLanda2(0),*/
  data1(0),
  data2(0),
  data3(0),
  data4(0),
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
		 NS_LOG_FUNCTION("Buffer full");
		 return -1;
	  }

	  t1 = Simulator::Now()-t1;
	  //NS_LOG_FUNCTION(Simulator::Now().GetSeconds() << t.GetSeconds());
	  data+= p->GetSize();
	  if(t1.GetSeconds()>0){
		  m_landaRate = data / t1.GetSeconds();
		  data = 0;
		  double alpha = 0.6;
		  double sample_landa = m_landaRate;
		  m_landaRate = (alpha * m_lastLanda) + ((1 - alpha) * ((sample_landa + m_lastSampleLanda) / 2));
		  m_lastSampleLanda = sample_landa;
		  m_lastLanda = sample_landa;
	  }
	  t1 = Simulator::Now();
	 m_sizeTxMap.insert(SizePair(m_txBuffer.TailSequence()-p->GetSize(),p->GetSize()));


  return 1;

}


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
	NS_LOG_FUNCTION(this << m_landaRate << sockets.size() << m_nextTxSequence);

	//Find packet size

	Ptr<Packet> packet;
	std::map<SequenceNumber64,uint32_t>::iterator it;
	it=m_sizeTxMap.find(m_nextTxSequence);

	NS_LOG_LOGIC("last subflow " << m_lastUsedsFlowIdx << " " << nextSubFlow << " " << sockets.size());

	//Update 2*RTTmin value
	for(uint32_t i=0;i<sockets.size();i++){

    	if(m_rtt<sockets[i]->GetMinRtt()){
    		m_rtt = Seconds(5*sockets[i]->GetMinRtt().GetSeconds());
    		NS_LOG_LOGIC(m_rtt.GetSeconds() << " " << sockets[i]->GetMinRtt());
    	}
	}

	//Trigger Update function every 2*RTTmin
    if(!m_IsCount&&m_rtt.GetSeconds()>0){
	  NS_LOG_LOGIC("Waterfilling event " <<  m_rtt.GetSeconds());
	  m_IsCount = true;
	  m_bwEstimateEvent.Cancel();
	  m_bwEstimateEvent = Simulator::Schedule (m_rtt, &MpWaterfilling::Update,this,sockets);
    }

    //Before first update send traffic like a round robin
    if (it!=m_sizeTxMap.end()){

        if(update==false){
        	nextSubFlow = (m_lastUsedsFlowIdx + 1) % sockets.size();
        	NS_LOG_LOGIC("Not update " << m_lastUsedsFlowIdx << " " << nextSubFlow << " " << sockets.size());

        //If there is any packet
        } else {
			//Find Lambdas
			std::map<uint32_t,double>::iterator iter = lambdaMap.find(0);
			std::map<uint32_t,double>::iterator iter2 = lambdaMap.find(1);

			NS_LOG_LOGIC("Comp " << data1 << " " << iter->second << " " << data2 << " " << iter2->second << " " << data3 << " " <<data4);
			if(data1<iter->second)
			{
				nextSubFlow=0;
			} else if(data2<iter2->second)
			{
				nextSubFlow=1;
			} else {
				nextSubFlow=-1;
			}
        }

        //If valid subflow send packet
     	if(nextSubFlow!=-1){
     		packet =  m_txBuffer.CopyFromSequence (it->second, m_nextTxSequence);
     	} else {
     		packet = Create<Packet>();
     	}
         m_lastUsedsFlowIdx = nextSubFlow;
     	NS_LOG_LOGIC("Nextsubflow " << nextSubFlow);

     	//if(nextSubFlow!=-1)data+=it->second;

     	//Calculate throughput
     	switch(nextSubFlow){
     	case 0:
     		//data1+=it->second;
     		data1+=m_mtu;
     		NS_LOG_LOGIC("data1 " << data1);

     		break;
     	case 1:
     		//data2+=it->second;
     		data2+=m_mtu;
     		NS_LOG_LOGIC("data2 " << data2);

     		break;
     	default:

     		break;
     	}
    } else {
		packet = Create<Packet>();
    }



	//NS_LOG_LOGIC ("lambda3 " << GetLambda(2)*8);
	if(nextSubFlow==-1)return Create<Packet>();
	else return packet;
}


double
MpWaterfilling::GetLambda(uint32_t channel)
{
	NS_LOG_LOGIC("data1 " << data1 << " data2 " << data2 << " time " << (Simulator::Now().GetSeconds()-t.GetSeconds()));

	double lambda;

	switch(channel)
	{
	case 0:
		lambda = (double)data1/(Simulator::Now().GetSeconds()-t.GetSeconds());
		break;
	case 1:
		lambda = (double)data2/(Simulator::Now().GetSeconds()-t.GetSeconds());
		break;
	default:
		return 0;
		break;

	}

	return lambda;

}

void
MpWaterfilling::SetNextSequence(SequenceNumber64 seq)
{
	m_nextTxSequence = seq;
}

int
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
	lambdaMap.clear();

	NS_LOG_LOGIC ("bw" << 0 << " " << sockets[0]->GetCurrentBw());
	NS_LOG_LOGIC ("bw" << 1 << " " << sockets[1]->GetCurrentBw());


	int i = 0;
	for(std::vector<Ptr<TcpSocketBase> >::iterator it = sockets.begin(); it!=sockets.end();++it)
	{
		Ptr<TcpSocketBase> sock = *it;
		bweMap.insert(std::pair<double,uint32_t>(sock->GetCurrentBw(),i));
		i++;
	}


	double lambda = m_landaRate;

	NS_LOG_LOGIC("Lambda" << lambda);
	int s = 1;
	for(std::map<double,uint32_t>::reverse_iterator i = bweMap.rbegin (); i != bweMap.rend (); i++)
	{

		double c = 0;
		std::map<double,uint32_t>::reverse_iterator j = i;
		j++;
		if(j!=bweMap.rend()){
		    c= i->first - j->first;
		    NS_LOG_LOGIC("c " << c);
		    if(c>lambda)c = lambda;
		    NS_LOG_LOGIC("c " << c);
		} else {
			c = lambda;
		    NS_LOG_LOGIC("c " << c);
		}
		lambda-=c;
		NS_LOG_LOGIC("lambdac " << lambda << " c " << c);
		for(std::map<uint32_t,double>::iterator it = lambdaMap.begin (); it != lambdaMap.end (); ++it)
		{
			lambdaMap.erase(it);
			lambdaMap.insert(std::pair<uint32_t,double>(it->first,it->second+(c/s)));
		}
		lambdaMap.insert(std::pair<uint32_t,double>(i->second,c/s));
		s++;

	}
	for(std::map<uint32_t,double>::iterator it = lambdaMap.begin (); it != lambdaMap.end (); ++it)
	{
		NS_LOG_LOGIC("channel " << it->first << " lambda " << it->second);
		lambdaMap.erase(it);
		NS_LOG_LOGIC("lambda " << it->first << " " << (double)it->second*(Simulator::Now().GetSeconds()-t.GetSeconds()));
		lambdaMap.insert(std::pair<uint32_t,double>(it->first,((double)it->second*(Simulator::Now().GetSeconds()-t.GetSeconds()))));
	}
	data3=sockets[0]->GetCurrentBw()*(Simulator::Now().GetSeconds()-t.GetSeconds());
	data4=sockets[1]->GetCurrentBw()*(Simulator::Now().GetSeconds()-t.GetSeconds());

	t= Simulator::Now();
	data1=data2=0;
	m_IsCount = false;

}
} /* namespace ns3 */
