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

#include "semifast-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SemifastServerApplication");

NS_OBJECT_ENSURE_REGISTERED (SemifastServer);

void
SemifastServer::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[SERVER " << m_personalID << " - "<< Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}


TypeId
SemifastServer::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::SemifastServer")
    				.SetParent<Application> ()
					.SetGroupName("Applications")
					.AddConstructor<SemifastServer> ()
					.AddAttribute ("Port", "Port on which we listen for incoming packets.",
							UintegerValue (9),
							MakeUintegerAccessor (&SemifastServer::m_port),
							MakeUintegerChecker<uint16_t> ())
					.AddAttribute ("PacketSize", "Size of echo data in outbound packets",
							UintegerValue (100),
							MakeUintegerAccessor (&SemifastServer::m_size),
							MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("LocalAddress",
							"The local Address of the current node",
							AddressValue (),
							MakeAddressAccessor (&SemifastServer::m_myAddress),
							MakeAddressChecker ())
					.AddAttribute ("Optimize",
							"Optimize protocol with propagation flag",
							UintegerValue (1),
							MakeUintegerAccessor (&SemifastServer::m_optimize),
							MakeUintegerChecker<uint16_t> ())
					.AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&SemifastServer::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("Verbose",
					 "Verbose for debug mode",
					 UintegerValue (0),
					 MakeUintegerAccessor (&SemifastServer::m_verbose),
					 MakeUintegerChecker<uint16_t> ())
					;
	return tid;
}

SemifastServer::SemifastServer ()
{
	NS_LOG_FUNCTION (this);
	m_socket = 0;
	m_ts = 0;
	m_value = 0;
	m_ps = 0;
	m_sent=0;
	m_optimize = 1;
}

SemifastServer::~SemifastServer()
{
	NS_LOG_FUNCTION (this);
	m_socket = 0;
	m_ts = 0;
	m_value = 0;
	m_ps = 0;
	m_sent=0;
	m_optimize = 1;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/

void 
SemifastServer::StartApplication (void)
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

	m_socket->SetRecvCallback (MakeCallback (&SemifastServer::HandleRead, this));

	// Accept new connection
	m_socket->SetAcceptCallback (
			MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
			MakeCallback (&SemifastServer::HandleAccept, this));
	// Peer socket close handles
	m_socket->SetCloseCallbacks (
			MakeCallback (&SemifastServer::HandlePeerClose, this),
			MakeCallback (&SemifastServer::HandlePeerError, this));
}

void 
SemifastServer::StopApplication ()
{
	NS_LOG_FUNCTION (this);

	if (m_socket != 0)
	{
		m_socket->Close ();
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
	std::stringstream sstm;
	sstm << "** SERVER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent<<" **";
	LogInfo(sstm);
}

void
SemifastServer::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}


/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void SemifastServer::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void SemifastServer::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void SemifastServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&SemifastServer::HandleRead, this));
	m_socketList.push_back (s);
}

/**************************************************************************************
 * PACKET DATA Handler
 **************************************************************************************/
void
SemifastServer::SetFill (std::string fill)
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
SemifastServer::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;
	uint32_t msgT, msgTs, msgV, msgVp, msgC, msgVid;
	std::stringstream sstm;

	while ((packet = socket->RecvFrom (from)))
	{
		//deserialize the contents of the packet
		uint8_t buf[1024];
		packet->CopyData(buf,1024);
		std::stringbuf sb;
		sb.str(std::string((char*) buf));
		std::istream istm(&sb);
		istm >> msgC >> msgT >> msgTs >> msgV >> msgVp >> msgVid;

		AsmCommon::Reset(sstm);
		sstm << "Received " << packet->GetSize () << " bytes from " <<
				InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf << " optimized " << m_optimize;
		LogInfo(sstm);


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
		}

		//insert the sender in the seen set
		m_seen.insert(msgVid);

		// set the propagation flag if msg received from reader
		if(msgTs > m_ps && msgT == INFORM && m_optimize == 1)
		{
			m_ps = msgTs;
		}

		NS_LOG_LOGIC ("Replying to packet");

		// Prepare packet content
		std::stringstream pkts;
		// serialize <counter, msgType, ts, value, |seen|, propflag>
		pkts << msgC << " " << msgT << " " << m_ts << " " << m_value << " " << m_pvalue << " " << m_ps << " " << m_seen.size();

		// serialize the members of seen set
		for (std::set<uint32_t>::iterator it=m_seen.begin(); it!=m_seen.end(); it++ )
		{
			pkts << " " << *it;
		}

		SetFill(pkts.str());

		Ptr<Packet> p;
		if (m_dataSize)
		{
			NS_ASSERT_MSG (m_dataSize == m_size, "SemifastServer::HandleSend(): m_size and m_dataSize inconsistent");
			NS_ASSERT_MSG (m_data, "SemifastServer::HandleSend(): m_dataSize but no m_data");
			p = Create<Packet> (m_data, m_dataSize);
		}
		else
		{
			p = Create<Packet> (m_size);
		}

		//socket->SendTo (p, 0, from);
		socket->Send (p);
		m_sent++;

		AsmCommon::Reset(sstm);
		sstm << "Sent " << packet->GetSize () << " bytes to " <<
				InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				InetSocketAddress::ConvertFrom (from).GetPort () << " "
				<< SetOperation<uint32_t>::printSet(m_seen,"Seen", "");

		/*for(std::set< Address >::iterator it=m_seen.begin(); it!=m_seen.end(); it++)
			sstm << " " <<  *it;

		sstm << "} data " << pkts.str();
		*/
		LogInfo(sstm);
	}
}

} // Namespace ns3
