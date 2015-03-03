/*
 * mp-scheduler.cc
 *
 *  Created on: Aug 20, 2013
 *      Author: sergi
 */

#include "mp-scheduler.h"
#include "ns3/log.h"
#include "ns3/packet.h"

NS_LOG_COMPONENT_DEFINE ("MpScheduler");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpScheduler);

TypeId
MpScheduler::GetTypeId (void) {
   static TypeId tid = TypeId ("ns3::MpScheduler")
	.SetParent<Object> ()
   .AddAttribute ("Mtu", "Flow one time limit in ms",
   			UintegerValue (1387),
   			MakeUintegerAccessor (&MpScheduler::m_mtu),
   			MakeUintegerChecker<uint32_t> ())
   ;
   return tid;
 }

MpScheduler::MpScheduler()
: m_txBuffer (0),
  m_nextTxSequence(1),
  m_lastUsedsFlowIdx(0)
{
	m_txBuffer.SetHeadSequence(m_nextTxSequence);
	// TODO Auto-generated constructor stub

}

MpScheduler::~MpScheduler() {
	// TODO Auto-generated destructor stub
}

void
MpScheduler::SetSndBufSize (uint32_t size)
{
	  NS_LOG_FUNCTION(this<<size);
  m_txBuffer.SetMaxBufferSize (size);
}

uint32_t
MpScheduler::GetSndBufSize (void) const
{
  return m_txBuffer.MaxBufferSize ();
}

uint32_t
MpScheduler::GetBufSize (void) const
{
  return m_txBuffer.Size ();
}

int
MpScheduler::Add(Ptr<Packet> p,uint32_t subflow)
{
	NS_LOG_FUNCTION(this << p->GetSize() << m_txBuffer.HeadSequence () << m_txBuffer.TailSequence () << m_txBuffer.Size());

	  if (!m_txBuffer.Add (p))
	  { // TxBuffer overflow, send failed
		 NS_LOG_FUNCTION("Buffer full");
		 return -1;
	  }
	  m_sizeTxMap.insert(SizePair(m_txBuffer.TailSequence()-p->GetSize(),p->GetSize()));

  return 1;

}

uint32_t
MpScheduler::Available(void) const
{
	NS_LOG_FUNCTION(this<<m_txBuffer.Available()<<m_txBuffer.MaxBufferSize()<<m_txBuffer.Size());
	return m_txBuffer.Available();
}

uint32_t
MpScheduler::Assigned(Ptr<TcpSocketBase> tcpSocketBase)
{

	BuffSizeMap::iterator biter = m_assignedSizeMap.find(tcpSocketBase);
	uint32_t size = 0;
	if(biter!=m_assignedSizeMap.end())
	{
		size = biter->second;
	}
	NS_LOG_FUNCTION(this<<tcpSocketBase<<size<<Size(tcpSocketBase));
	return size;
}
uint32_t
MpScheduler::Size(Ptr<TcpSocketBase> tcpSocketBase)
{
	BuffSizeMap::iterator biter = m_bufSizeMap.find(tcpSocketBase);
	uint32_t size = 0;
	if(biter!=m_bufSizeMap.end())
	{
		size = biter->second;
	}
	//NS_LOG_FUNCTION(this<<tcpSocketBase<<size);
	return size;
}

uint32_t
MpScheduler::SizeFromSequence(Ptr<TcpSocketBase> tcpSocketBase,const SequenceNumber32& seq)
{
	SequenceNumber32 lastSeq;
	FirstSeqMap::iterator it = m_seqMap.find(tcpSocketBase);
	/*for(FirstSeqMap::iterator fit=m_seqMap.begin();fit!=m_seqMap.end();++fit)
	{
		BuffSizeMap::iterator bit = m_bufSizeMap.find(fit->first);
		NS_LOG_LOGIC("FirstSeq " << fit->first << " " << fit->second << " Size " << bit->second);

	}*/
	if(it!=m_seqMap.end())
	{
		BuffSizeMap::iterator iter = m_bufSizeMap.find(tcpSocketBase);
		if(iter!=m_bufSizeMap.end())
		{
			NS_LOG_LOGIC("firstseq " << it->second << " size " << iter->second);
			lastSeq = it->second + iter->second;

		}
	}
	NS_LOG_LOGIC("SFS " <<tcpSocketBase << " " << lastSeq << " " << seq);
    return lastSeq - seq;
}
void
MpScheduler::SetNextSequence(SequenceNumber64 seq)
{
	m_nextTxSequence = seq;
}

void
MpScheduler::MapSequence(Ptr<TcpSocketBase> tcpSocketBase)
{
	SubflowMap::iterator it = m_assignedSubflow.find(tcpSocketBase);
	std::map<SequenceNumber64,uint32_t>::iterator iter=m_sizeTxMap.find(m_nextTxSequence);
	BuffSizeMap::iterator biter = m_bufSizeMap.find(tcpSocketBase);
	uint32_t size;
	if(biter!=m_bufSizeMap.end())size = biter->second;
	else size = 0;

	NS_LOG_LOGIC("Seq " << m_nextTxSequence << " " << iter->first << " " <<iter->second << " " << size);

	if(iter!=m_sizeTxMap.end()){
		size+=iter->second;
		NS_LOG_LOGIC("Insert " << tcpSocketBase << " " << m_nextTxSequence << " " << HeadSequence(tcpSocketBase) << " " << Size(tcpSocketBase) << " " << TailSequence(tcpSocketBase));
		if(it!=m_assignedSubflow.end())
		{
			NS_LOG_LOGIC("Assigned");
			std::vector<SeqPair> seq = it->second;
			seq.push_back(SeqPair(m_nextTxSequence,TailSequence(tcpSocketBase)));
			m_assignedSubflow.erase(it);
			m_assignedSubflow.insert(SubflowPair(tcpSocketBase,seq));
		} else {
			NS_LOG_LOGIC("Not assigned " << m_nextTxSequence << " " << TailSequence(tcpSocketBase));
			std::vector <SeqPair> socketsAssigned;
			socketsAssigned.push_back(SeqPair(m_nextTxSequence,TailSequence(tcpSocketBase)));
			m_assignedSubflow.insert(SubflowPair(tcpSocketBase,socketsAssigned));
		}
	}


	if(biter!=m_bufSizeMap.end())
	{
		m_bufSizeMap.erase(biter);
	}
	m_bufSizeMap.insert(BuffSizePair(tcpSocketBase,size));

	UpdateAssigned(tcpSocketBase);
	NS_LOG_LOGIC("Insert " << tcpSocketBase << " " << m_nextTxSequence << " " << HeadSequence(tcpSocketBase) << " " << Size(tcpSocketBase) << " " << TailSequence(tcpSocketBase));

}

void
MpScheduler::UpdateAssigned(Ptr<TcpSocketBase> tcpSocketBase)
{

	SequenceNumber32 seq;
	FirstSeqMap::iterator it = m_asSeqMap.find(tcpSocketBase);
	if(it!=m_asSeqMap.end())
	{
		seq = it->second;
	}

	BuffSizeMap::iterator biter = m_assignedSizeMap.find(tcpSocketBase);
	if(biter!=m_assignedSizeMap.end())
	{
		m_assignedSizeMap.erase(biter);
	}
	m_assignedSizeMap.insert(BuffSizePair(tcpSocketBase,TailSequence(tcpSocketBase)-seq));
	NS_LOG_FUNCTION(this<<tcpSocketBase<<seq<<TailSequence(tcpSocketBase)<<Assigned(tcpSocketBase));
}
int
MpScheduler::GetSubflowToUse()
{
	NS_LOG_FUNCTION(this<<m_lastUsedsFlowIdx);


    return m_lastUsedsFlowIdx;
}

int
MpScheduler::Discard(SequenceNumber64 seq)
{

	NS_LOG_FUNCTION(this << seq);
	SequenceNumber32 seq32;
	Ptr<TcpSocketBase> sock;
	bool found = false;
	for(SubflowMap::iterator sIt = m_assignedSubflow.begin();sIt!=m_assignedSubflow.end();++sIt)
	{
		//NS_LOG_LOGIC("socket " << sIt->first);
		std::vector<SeqPair> sequence = sIt->second;
		std::vector<SeqPair>::iterator seqIt;
		for(seqIt = sequence.begin();seqIt!=sequence.end();++seqIt)
		{
			if(seqIt->first<seq)
			{
				sock=sIt->first;
				found=true;
				break;
			}
		}
		if(found)
		{
			//NS_LOG_LOGIC("Seq64 " << seqIt->first << " seq32 " << seqIt->second);
			m_assignedSubflow.erase(sIt);
			sequence.erase(seqIt);
			SeqPair spair = sequence[0];
			//NS_LOG_LOGIC("Seq64 " << spair.first << " seq32 " << spair.second);
			seq32 = spair.second;
			m_assignedSubflow.insert(SubflowPair(sock,sequence));
			break;
		}
	}


	found = false;

	for(SubflowMap::iterator sIt = m_sentSubflow.begin();sIt!=m_sentSubflow.end();++sIt)
	{
		std::vector<SeqPair> sequence = sIt->second;
		std::vector<SeqPair>::iterator seqIt;
		for(seqIt = sequence.begin();seqIt!=sequence.end();++seqIt)
		{
			if(seqIt->first<seq)
			{
				sock=sIt->first;
				found=true;
				break;
			}
		}
		if(found)
		{
			//NS_LOG_LOGIC("Seq64 " << seqIt->first << " seq32 " << seqIt->second);
			m_sentSubflow.erase(sIt);
			sequence.erase(seqIt);
			SeqPair spair = sequence[0];
			//NS_LOG_LOGIC("Seq64 " << spair.first << " seq32 " << spair.second);
			seq32 = spair.second;
			m_sentSubflow.insert(SubflowPair(sock,sequence));
			break;
		}
	}

    return 0;

}
void
MpScheduler::DiscardUpTo(Ptr<TcpSocketBase> sock,SequenceNumber32 seq)
{
	NS_LOG_LOGIC("Insert " << sock << " " << seq << " " << HeadSequence(sock) << " " << Size(sock) << " " << TailSequence(sock));
	SequenceNumber32 tail = TailSequence(sock);
	m_seqMap.erase(sock);
	NS_LOG_LOGIC("Seqmap " << sock << " " << seq);
	m_seqMap.insert(FirstSeqPair(sock,seq));
	m_bufSizeMap.erase(sock);
	//NS_LOG_LOGIC("Socket " << sock << " size " << tail << " " << HeadSequence(sock));
	uint32_t size = (tail-HeadSequence(sock))>0?(tail-HeadSequence(sock)):0;
	NS_LOG_LOGIC("Socket " << sock << " size " << tail << " " << HeadSequence(sock) << " " << size << " " << Assigned(sock));

	m_bufSizeMap.insert(BuffSizePair(sock,size));

	NS_LOG_FUNCTION(this << seq);
	SequenceNumber64 seq64;

	SubflowMap::iterator sIt = m_assignedSubflow.find(sock);
	if(sIt!=m_assignedSubflow.end())
	{

		for(std::vector<SeqPair>::iterator seqIt = sIt->second.begin();seqIt!=sIt->second.end();++seqIt)
		{
			if(seqIt->second==seq)
			{
				seq64 = seqIt->first;
				break;
			}
			m_txBuffer.Remove(seqIt->first);
		}
	}
	NS_LOG_LOGIC("Erase " << seq << " " << seq64 << " " << m_mtu << " " << m_txBuffer.Size());
}

void
MpScheduler::SetHeadSequence(Ptr<TcpSocketBase> tcpSocketBase,const SequenceNumber32& seq)
{
	NS_LOG_FUNCTION(this<<tcpSocketBase<<seq);
	m_seqMap.insert(FirstSeqPair(tcpSocketBase,seq));
	m_asSeqMap.insert(FirstSeqPair(tcpSocketBase,seq));
}

SequenceNumber32
MpScheduler::HeadSequence(Ptr<TcpSocketBase> tcpSocketBase)
{
	SequenceNumber32 seq;
	FirstSeqMap::iterator it = m_seqMap.find(tcpSocketBase);
	if(it!=m_seqMap.end())
	{
		seq = it->second;
	}
	return seq;
}

SequenceNumber32
MpScheduler::TailSequence(Ptr<TcpSocketBase> tcpSocketBase)
{
	SequenceNumber32 seq;
	FirstSeqMap::iterator it = m_seqMap.find(tcpSocketBase);
	if(it!=m_seqMap.end())
	{
		//NS_LOG_LOGIC(it->second);
		seq = it->second;
		BuffSizeMap::iterator iter = m_bufSizeMap.find(tcpSocketBase);
		if(iter!=m_bufSizeMap.end())
		{
			seq +=iter->second;
		}
	}
	return seq;
}

void
MpScheduler::SendBackup(Ptr<TcpSocketBase> tcpSocketBase)
{
	m_nextTxSequence-=m_mtu;
	NS_LOG_LOGIC("SendBackup " << m_nextTxSequence);
	MapSequence(tcpSocketBase);
}
Ptr<Packet>
MpScheduler::CopyFromSequence (Ptr<TcpSocketBase> tcpSocketBase, uint32_t numBytes, const SequenceNumber32& seq)
{
	NS_LOG_FUNCTION(this<<tcpSocketBase<<numBytes<<seq);
	Ptr<Packet> p;
	SequenceNumber64 seq64;
	SubflowMap::iterator it = m_assignedSubflow.find(tcpSocketBase);


	if(it!=m_assignedSubflow.end())
	{
		std::vector<SeqPair> sequence = it->second;
		for(std::vector<SeqPair>::iterator seqIt = sequence.begin();seqIt!=sequence.end();++seqIt)
		{
			//NS_LOG_LOGIC("vector " << seqIt->first << " " <<seqIt->second);
			if(seq==seqIt->second)seq64=seqIt->first;
		}
		NS_LOG_LOGIC("Copy " << numBytes << " " << seq64 << " " << m_txBuffer.HeadSequence());
		if(seq64>=m_txBuffer.HeadSequence()){
			Ptr<Packet> packet =  m_txBuffer.CopyFromSequence (numBytes, seq64);
			NS_LOG_LOGIC("Packet " << packet->GetSize() << " " << packet);
			return packet;
		}else{
			//Ptr<Packet> packet =  m_txBuffer.CopyFromSequence (numBytes, m_txBuffer.HeadSequence());
			//NS_LOG_LOGIC("Packet " << packet->GetSize() << " " << packet);
			NS_LOG_LOGIC("Empty packet");
			return Create<Packet>(1000);
		}
	}
	NS_LOG_LOGIC("Return 0");

	return 0;
}

SequenceNumber64
MpScheduler::GetNextSequence (Ptr<TcpSocketBase> tcpSocketBase, const SequenceNumber32& seq)
{
	NS_LOG_FUNCTION(this<<tcpSocketBase<<seq);
	SequenceNumber64 seq64;
	SubflowMap::iterator it = m_assignedSubflow.find(tcpSocketBase);
	if(it!=m_assignedSubflow.end())
	{
		std::vector<SeqPair> sequence = it->second;
		for(std::vector<SeqPair>::iterator seqIt = sequence.begin();seqIt!=sequence.end();++seqIt)
		{
			//NS_LOG_LOGIC("vector " << seqIt->first << " " <<seqIt->second);
			if(seq==seqIt->second)seq64=seqIt->first;
		}

	}

	SubflowMap::iterator sit = m_sentSubflow.find(tcpSocketBase);
	if(sit!=m_sentSubflow.end())
	{
		NS_LOG_LOGIC("Assigned");
		std::vector<SeqPair> sequence = sit->second;
		sequence.push_back(SeqPair(seq64,seq));
		m_sentSubflow.erase(sit);
		m_sentSubflow.insert(SubflowPair(tcpSocketBase,sequence));
	} else {
		NS_LOG_LOGIC("Not assigned " << m_nextTxSequence << " " << TailSequence(tcpSocketBase));
		std::vector <SeqPair> socketsAssigned;
		socketsAssigned.push_back(SeqPair(seq64,seq));
		m_sentSubflow.insert(SubflowPair(tcpSocketBase,socketsAssigned));
	}

	FirstSeqMap::iterator asit = m_asSeqMap.find(tcpSocketBase);
	if(asit!=m_asSeqMap.end())
	{
		m_asSeqMap.erase(asit);
		m_asSeqMap.insert(FirstSeqPair(tcpSocketBase,seq));
	}
	UpdateAssigned(tcpSocketBase);

	return seq64;
}
} /* namespace ns3 */

