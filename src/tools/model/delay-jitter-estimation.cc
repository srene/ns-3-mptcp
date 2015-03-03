
#include "delay-jitter-estimation.h"
#include "ns3/tag.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("DelayJitterEstimation");

namespace ns3 {

TypeId
DelayJitterEstimationTimestampTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("anon::DelayJitterEstimationTimestampTag")
    .SetParent<Tag> ()
    .AddConstructor<DelayJitterEstimationTimestampTag> ()
    .AddAttribute ("CreationTime",
                   "The time at which the timestamp was created",
                   StringValue ("0.0s"),
                   MakeTimeAccessor (&DelayJitterEstimationTimestampTag::GetTxTime),
                   MakeTimeChecker ())
  ;
  return tid;
}


DelayJitterEstimationTimestampTag::DelayJitterEstimationTimestampTag ()
  : m_creationTime (Simulator::Now ().GetTimeStep ())
{
}


TypeId
DelayJitterEstimationTimestampTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
DelayJitterEstimationTimestampTag::GetSerializedSize (void) const
{
  return 8;
}
void
DelayJitterEstimationTimestampTag::Serialize (TagBuffer i) const
{
  i.WriteU64 (m_creationTime);
}
void
DelayJitterEstimationTimestampTag::Deserialize (TagBuffer i)
{
  m_creationTime = i.ReadU64 ();
}
void
DelayJitterEstimationTimestampTag::Print (std::ostream &os) const
{
  os << "CreationTime=" << m_creationTime;
}
Time
DelayJitterEstimationTimestampTag::GetTxTime (void) const
{
  return TimeStep (m_creationTime);
}

DelayJitterEstimation::DelayJitterEstimation ()
  : m_previousRx (Simulator::Now ()),
    m_previousRxTx (Simulator::Now ()),
    m_jitter (0),
    m_delay (Seconds (0.0))
{
}
void
DelayJitterEstimation::PrepareTx (Ptr<const Packet> packet)
{
  DelayJitterEstimationTimestampTag tag;
  NS_LOG_LOGIC("tag " << packet);
  packet->AddByteTag (tag);
}
void
DelayJitterEstimation::RecordRx (Ptr<const Packet> packet)
{
  NS_LOG_LOGIC("jitter "<<packet);
  DelayJitterEstimationTimestampTag tag;
  bool found;
  found = packet->FindFirstMatchingByteTag (tag);
  if (!found)
    {
	  NS_LOG_LOGIC("not found");

      return;
    }
  tag.GetTxTime ();

  delta = (Simulator::Now () - m_previousRx) - (tag.GetTxTime () - m_previousRxTx);
  m_jitter += (Abs (delta) - m_jitter) / 16;
  m_previousRx = Simulator::Now ();
  m_previousRxTx = tag.GetTxTime ();
  m_delay = Simulator::Now () - tag.GetTxTime ();
}

Time 
DelayJitterEstimation::GetLastDelay (void) const
{
  return m_delay;
}
uint64_t
DelayJitterEstimation::GetLastJitter (void) const
{
  return m_jitter.GetHigh ();
}

Time
DelayJitterEstimation::GetLastDelta (void) const
{
  return delta;
}
} // namespace ns3
