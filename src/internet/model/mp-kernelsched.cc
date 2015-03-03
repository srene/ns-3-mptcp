/*
 * mp-scheduler.cc
 *
 *  Created on: Aug 19, 2013
 *      Author: sergi
 */

#include "mp-kernelsched.h"
#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

NS_LOG_COMPONENT_DEFINE ("MpKernelScheduler");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpKernelScheduler);

TypeId
MpKernelScheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpKernelScheduler")
.SetParent<MpScheduler> ()
.AddConstructor<MpKernelScheduler> ()
.AddAttribute ("SndBufSize",
	               "TcpSocket maximum transmit buffer size (bytes)",
	               UintegerValue (2097152), // 256k
	               MakeUintegerAccessor (&MpKernelScheduler::GetSndBufSize,
	                                     &MpKernelScheduler::SetSndBufSize),
	               MakeUintegerChecker<uint32_t> ());
  return tid;
}

MpKernelScheduler::MpKernelScheduler()
: m_landaRate(0),
m_lastSampleLanda(0),
m_lastLanda(0),
data(0),
nextSubFlow(0),
data1(0),
data2(0)
{
	// TODO Auto-generated constructor stub

}

MpKernelScheduler::~MpKernelScheduler() {
	// TODO Auto-generated destructor stub
}

/*void
MpKernelScheduler::SetSndBufSize (uint32_t size)
{
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
MpKernelScheduler::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}*/
/*int
MpKernelScheduler::Add(Ptr<Packet> packet,uint32_t subflow)
{

}*/
int
MpKernelScheduler::Add(Ptr<Packet> p,uint32_t subflow)
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
MpKernelScheduler::Discard(SequenceNumber64 seq)
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
	   m_txBuffer.DiscardUpTo(seq);
	   NS_LOG_LOGIC ("New ack of " << seq << " " <<m_txBuffer.Size());
	   MpScheduler::Discard(seq);
	   return 1;
	}
    return 0;

}

bool
MpKernelScheduler::GetPacket(std::vector<Ptr<TcpSocketBase> > sockets)
{
	NS_LOG_FUNCTION(this << GetLanda(0)*8 << GetLanda(1)*8 << sockets.size() << m_lastUsedsFlowIdx);

	Ptr<Packet> packet;
	NS_LOG_FUNCTION(this << sockets[0]->AvailableWindow() << sockets[1]->AvailableWindow() << sockets[0]->GetRtt()->GetCurrentEstimate() << sockets[1]->GetRtt()->GetCurrentEstimate() << sockets.size() << m_lastUsedsFlowIdx);

	std::map<uint32_t,Ptr<TcpSocketBase> > socket_list;
	uint32_t pos = 0;
	nextSubFlow = -1;

	for(std::vector<Ptr<TcpSocketBase> >::iterator iter=  sockets.begin(); iter!= sockets.end();++iter)
	{
		Ptr<TcpSocketBase> sock = *iter;
		NS_LOG_FUNCTION(this << sock->AvailableWindow() << sock->GetRtt()->GetCurrentEstimate() << sockets.size() << m_lastUsedsFlowIdx << pos);

		if(sock->AvailableWindow()>=m_mtu){
			NS_LOG_LOGIC("insert " << pos << " " << *iter);
			socket_list.insert(std::pair<uint32_t,Ptr<TcpSocketBase> >(pos,*iter));
		}
		pos++;
	}

	std::map<uint32_t,Ptr<TcpSocketBase> >::iterator map_it = socket_list.begin();
	if(map_it!=socket_list.end()){
		nextSubFlow = map_it->first;
		Ptr<TcpSocketBase> socket = map_it->second;
		for(std::map<uint32_t,Ptr<TcpSocketBase> >::iterator iter=  socket_list.begin(); iter!= socket_list.end();++iter)
		{
			Ptr<TcpSocketBase> sock = iter->second;
			//NS_LOG_LOGIC("insert " << iter->first << " " << iter->second <<  " " << socket->GetRtt()->GetCurrentEstimate() << " " << sock->GetRtt()->GetCurrentEstimate());
			if(socket->GetRtt()->GetCurrentEstimate()>sock->GetRtt()->GetCurrentEstimate()){
				socket=sock;
				nextSubFlow = iter->first;
			}

		}
	}
	//NS_LOG_LOGIC("subflow " << nextSubFlow << " " << m_nextTxSequence << " " << m_txBuffer.TailSequence());

	/*for(std::map<SequenceNumber64,uint32_t>::iterator iter = m_sizeTxMap.begin(); iter != m_sizeTxMap.end(); iter++)
	{
		  NS_LOG_LOGIC("Seq buffer1 " << iter->first << " "<< iter->second);
	}*/
	NS_LOG_LOGIC("Nextsubflow " << nextSubFlow << " tailseq " << m_txBuffer.TailSequence() << " nextseq " << m_nextTxSequence);
	if((nextSubFlow!=-1)&&(m_txBuffer.TailSequence()>m_nextTxSequence)){
		std::map<SequenceNumber64,uint32_t>::iterator it;
		it=m_sizeTxMap.find(m_nextTxSequence);
		if(it!=m_sizeTxMap.end()){
			//nextSubFlow = (m_lastUsedsFlowIdx + 1) % sockets.size();
		    m_lastUsedsFlowIdx = nextSubFlow;
		    NS_LOG_LOGIC("Get " << it->second << " " << m_nextTxSequence << " " << nextSubFlow);
		    MapSequence(sockets[m_lastUsedsFlowIdx]);
		    m_nextTxSequence+=it->second;

			//packet = m_txBuffer.CopyFromSequence (it->second, m_nextTxSequence);
			//NS_LOG_LOGIC("Packet sent " << packet);

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
	    return true;
	} else {
		NS_LOG_LOGIC("Packet not sent");
		return false;
	}
	return false;

}


double
MpKernelScheduler::GetLanda(uint32_t channel)
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

} /* namespace ns3 */
