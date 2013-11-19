/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
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
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#include "tcp-suboption.h"
#include "tcp-option-mp_join.h"
#include "tcp-option-mp_addr.h"
#include "tcp-option-mp_fclose.h"
#include "tcp-option-mp_rmaddr.h"
#include "tcp-option-mp_cap.h"
#include "tcp-option-mp_prio.h"
#include "tcp-option-mp_dss.h"


namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpSubOption);

TcpSubOption::TcpSubOption ()
{
}

TcpSubOption::~TcpSubOption ()
{
}

TypeId
TcpSubOption::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSubOption")
    .SetParent<Object> ()
  ;
  return tid;
}

TypeId
TcpSubOption::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

Ptr<TcpSubOption>
TcpSubOption::CreateSubOption (uint8_t subtype)
{

  if(subtype == 0)
	{
		return CreateObject<TcpOptionMpCapable> ();
	}
  else if (subtype == 1)
	{
	//	return CreateObject<TcpSubOptionMpJoin>
	}
  else if (subtype == 2)
	{
		return CreateObject<TcpOptionMpDss> ();
	}
  else if (subtype == 3)
	{
	//	return CreateObject<TcpSubOptionMpAddAddr> ();
	}
  else if (subtype == 4)
	{
	//	return CreateObject<TcpSubOptionMpRmAddr> ();
	}
  else if (subtype == 5)
	{
	//	return CreateObject<TcpSubOptionMpPrio> ();
	}
  else if (subtype == 6)
	{
	//	return CreateObject<TcpSubOptionMpFail> ();
	}
  else if (subtype == 7)
	{
		//return CreateObject<TcpSubOptionMpFClose> ();
	}

  return NULL;
} 

} // namespace ns3
