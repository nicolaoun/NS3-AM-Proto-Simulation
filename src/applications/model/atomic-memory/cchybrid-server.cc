/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "cchybrid-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CCHybridServerApplication");

NS_OBJECT_ENSURE_REGISTERED (CCHybridServer);

void
CCHybridServer::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[SERVER " << m_personalID << " - "<< Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}


TypeId
CCHybridServer::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::CCHybridServer")
    				.SetParent<Application> ()
					.SetGroupName("Applications")
					.AddConstructor<CCHybridServer> ()
					.AddAttribute ("Port", "Port on which we listen for incoming packets.",
							UintegerValue (9),
							MakeUintegerAccessor (&CCHybridServer::m_port),
							MakeUintegerChecker<uint16_t> ())
					.AddAttribute ("PacketSize", "Size of echo data in outbound packets",
							UintegerValue (100),
							MakeUintegerAccessor (&CCHybridServer::m_size),
							MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("LocalAddress",
							"The local Address of the current node",
							AddressValue (),
							MakeAddressAccessor (&CCHybridServer::m_myAddress),
							MakeAddressChecker ())
					.AddAttribute ("Optimize",
							"Optimize protocol with propagation flag",
							UintegerValue (1),
							MakeUintegerAccessor (&CCHybridServer::m_optimize),
							MakeUintegerChecker<uint16_t> ())
					.AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&CCHybridServer::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("Verbose",
					 "Verbose for debug mode",
					 UintegerValue (0),
					 MakeUintegerAccessor (&CCHybridServer::m_verbose),
					 MakeUintegerChecker<uint16_t> ())
	;
	return tid;
}

CCHybridServer::CCHybridServer ()
{
	NS_LOG_FUNCTION (this);
	m_socket = 0;
	m_ts = 0;
	m_value = 0;
	m_propagated = false;
	m_sent=0;
	//m_optimize = 1;
}

CCHybridServer::~CCHybridServer()
{
	NS_LOG_FUNCTION (this);
	m_socket = 0;
	m_ts = 0;
	m_value = 0;
	m_propagated = false;
	m_sent=0;
	//m_optimize = 1;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/

void 
CCHybridServer::StartApplication (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;
	sstm << "Debug Mode="<<m_verbose;
	LogInfo(sstm);

	if (m_socket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
		m_socket->Bind (local);
		m_socket->Listen ();

		if (addressUtils::IsMulticast (m_local))
		{
			Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
			if (udpSocket)
			{
				// equivalent to setsockopt (MCAST_JOIN_GROUP)
				udpSocket->MulticastJoinGroup (0, m_local);
			}
			else
			{
				NS_FATAL_ERROR ("Error: Failed to join multicast group");
			}
		}
	}

	m_socket->SetRecvCallback (MakeCallback (&CCHybridServer::HandleRead, this));

	// Accept new connection
	m_socket->SetAcceptCallback (
			MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
			MakeCallback (&CCHybridServer::HandleAccept, this));
	// Peer socket close handles
	m_socket->SetCloseCallbacks (
			MakeCallback (&CCHybridServer::HandlePeerClose, this),
			MakeCallback (&CCHybridServer::HandlePeerError, this));
}

void 
CCHybridServer::StopApplication ()
{
	NS_LOG_FUNCTION (this);

	if (m_socket != 0)
	{
		m_socket->Close ();
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
	std::stringstream sstm;
	sstm << "** SERVER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent<<" **";
	std::cout << "** SERVER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent<<" **"<<std::endl;
	//std::cout << "** SERVER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent<<" **"<<std::endl;
	LogInfo(sstm);
}

void
CCHybridServer::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}


/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void CCHybridServer::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void CCHybridServer::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void CCHybridServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&CCHybridServer::HandleRead, this));
	m_socketList.push_back (s);
}

/**************************************************************************************
 * PACKET DATA Handler
 **************************************************************************************/
void
CCHybridServer::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      //delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

/**************************************************************************************
 * ABD Rcv Handler
 **************************************************************************************/
void 
CCHybridServer::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;
	uint32_t msgT, msgTs, msgV, msgVp, msgC;
	std::stringstream sstm;

	while ((packet = socket->RecvFrom (from)))
	{
		//deserialize the contents of the packet
		uint8_t buf[1024];
		packet->CopyData(buf,1024);
		std::stringbuf sb;
		sb.str(std::string((char*) buf));
		std::istream istm(&sb);
		istm >> msgC >> msgT >> msgTs >> msgV >> msgVp;

		if (m_verbose)
		{
			AsmCommon::Reset(sstm);
			sstm << "Received " << packet->GetSize () << " bytes from " <<
					InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
					InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf << " optimized " << m_optimize;
			LogInfo(sstm);
		}


		packet->RemoveAllPacketTags ();
		packet->RemoveAllByteTags ();

		if ( m_ts < msgTs )
		{
			NS_LOG_LOGIC ("Updating Local Info (ts and seen set)");
			m_ts = msgTs;
			m_value = msgV;
			m_pvalue = msgVp;

			//reinitialize the seen set
			m_seen.clear();

			//reset propagate flag
			m_propagated = false;
		}

		//insert the sender in the seen set
		m_seen.insert(from);

		// set the propagation flag if msg received from reader
		if(m_ts == msgTs && msgT == INFORM && m_optimize == 1)
		{
			m_propagated = true;
		}

		NS_LOG_LOGIC ("Replying to packet");

		// Prepare packet content
		std::stringstream pkts;
		// serialize <counter, msgType, ts, value, |seen|, propflag>
		pkts << msgC << " " << msgT << " " << m_ts << " " << m_value << " " << m_pvalue << " " << m_seen.size() << " " << m_propagated;

		SetFill(pkts.str());

		Ptr<Packet> p;
		if (m_dataSize)
		{
			NS_ASSERT_MSG (m_dataSize == m_size, "CCHybridServer::HandleSend(): m_size and m_dataSize inconsistent");
			NS_ASSERT_MSG (m_data, "CCHybridServer::HandleSend(): m_dataSize but no m_data");
			p = Create<Packet> (m_data, m_dataSize);
		}
		else
		{
			p = Create<Packet> (m_size);
		}

		//socket->SendTo (p, 0, from);
		socket->Send (p);
		m_sent++;

		if (m_verbose)
		{
			AsmCommon::Reset(sstm);
			sstm << "Sent " << packet->GetSize () << " bytes to " <<
					InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
					InetSocketAddress::ConvertFrom (from).GetPort () << " seen: { ";

			for(std::set< Address >::iterator it=m_seen.begin(); it!=m_seen.end(); it++)
				sstm << " " <<  InetSocketAddress::ConvertFrom( *it ).GetIpv4();

			sstm << "} data " << pkts.str();
			LogInfo(sstm);
		}
	}
}

} // Namespace ns3
