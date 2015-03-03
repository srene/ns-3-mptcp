/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Raj Bhattacharjea <raj.b@gatech.edu>
 */
#ifndef MP_TCP_SOCKET_FACTORY_IMPL_H
#define MP_TCP_SOCKET_FACTORY_IMPL_H

#include "ns3/mp-tcp-socket-factory.h"
#include "ns3/ptr.h"

namespace ns3 {

class MpTcpL4Protocol;

/**
 * \ingroup internet
 * \defgroup tcp Tcp
 *
 * This class serves to create sockets of the TcpSocketBase type.
 */

/**
 * \ingroup tcp
 *
 * \brief socket factory implementation for native ns-3 TCP
 *
 */
class MpTcpSocketFactoryImpl : public MpTcpSocketFactory
{
public:
  MpTcpSocketFactoryImpl ();
  virtual ~MpTcpSocketFactoryImpl ();

  void SetTcp (Ptr<MpTcpL4Protocol> tcp);

  virtual Ptr<Socket> CreateSocket (void);

protected:
  virtual void DoDispose (void);
private:
  Ptr<MpTcpL4Protocol> m_tcp;
};

} // namespace ns3

#endif /* MP_TCP_SOCKET_FACTORY_IMPL_H */
