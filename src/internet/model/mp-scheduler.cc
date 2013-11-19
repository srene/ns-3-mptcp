/*
 * mp-scheduler.cc
 *
 *  Created on: Aug 20, 2013
 *      Author: sergi
 */

#include "mp-scheduler.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpScheduler);

TypeId
MpScheduler::GetTypeId (void) {
   static TypeId tid = TypeId ("ns3::MpScheduler")
	.SetParent<Object> ();
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

} /* namespace ns3 */
