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

#include "abd-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AbdServerApplication");

NS_OBJECT_ENSURE_REGISTERED (AbdServer);

void
AbdServer::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[SERVER " << m_personalID << " - "<< InetSocketAddress::ConvertFrom(m_myAddress).GetIpv4() << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}


TypeId
AbdServer::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::AbdServer")
    				.SetParent<Application> ()
					.SetGroupName("Applications")
					.AddConstructor<AbdServer> ()
					.AddAttribute ("Port", "Port on which we listen for incoming packets.",
							UintegerValue (9),
							MakeUintegerAccessor (&AbdServer::m_port),
							MakeUintegerChecker<uint16_t> ())
					.AddAttribute ("PacketSize", "Size of echo data in outbound packets",
							UintegerValue (100),
							MakeUintegerAccessor (&AbdServer::m_size),
							MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("LocalAddress",
							"The local Address of the current node",
							AddressValue (),
							MakeAddressAccessor (&AbdServer::m_myAddress),
							MakeAddressChecker ())
					.AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&AbdServer::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("Verbose",
					 "Verbose for debug mode",
					 UintegerValue (0),
					 MakeUintegerAccessor (&AbdServer::m_verbose),
					 MakeUintegerChecker<uint16_t> ())
		;
	return tid;
}

AbdServer::AbdServer ()
{
	NS_LOG_FUNCTION (this);
	m_socket = 0;
	m_ts = 0;
	m_value = 0;
	m_sent=0;     //!< sent messages counter
}

AbdServer::~AbdServer()
{
	NS_LOG_FUNCTION (this);
	m_socket = 0;
	m_ts = 0;
	m_value = 0;
	m_sent=0;     //!< sent messages counter

}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/

void 
AbdServer::StartApplication (void)
{
	NS_LOG_FUNCTION (this);

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

	m_socket->SetRecvCallback (MakeCallback (&AbdServer::HandleRead, this));

	// Accept new connection
	m_socket->SetAcceptCallback (
			MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
			MakeCallback (&AbdServer::HandleAccept, this));
	// Peer socket close handles
	m_socket->SetCloseCallbacks (
			MakeCallback (&AbdServer::HandlePeerClose, this),
			MakeCallback (&AbdServer::HandlePeerError, this));
}

void 
AbdServer::StopApplication ()
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
AbdServer::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}


/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void AbdServer::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void AbdServer::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void AbdServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&AbdServer::HandleRead, this));
	m_socketList.push_back (s);
}

/**************************************************************************************
 * PACKET DATA Handler
 **************************************************************************************/
void
AbdServer::SetFill (std::string fill)
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
AbdServer::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;
	uint32_t msgT, msgTs, msgV, msgC;
	std::stringstream sstm;

	while ((packet = socket->RecvFrom (from)))
	{
		//deserialize the contents of the packet
		uint8_t buf[1024];
		packet->CopyData(buf,1024);
		std::stringbuf sb;
		sb.str(std::string((char*) buf));
		std::istream istm(&sb);
		istm >> msgT >> msgTs >> msgV >> msgC;

		AsmCommon::Reset(sstm);
		sstm << "Received " << packet->GetSize () << " bytes from " <<
				InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
		LogInfo(sstm);


		packet->RemoveAllPacketTags ();
		packet->RemoveAllByteTags ();

		NS_LOG_LOGIC ("Updating Local Info");
		if ( m_ts < msgTs )
		{
			m_ts = msgTs;
			m_value = msgV;
		}


		NS_LOG_LOGIC ("Echoing packet");

		 // Prepare packet content
		  std::stringstream pkts;
		  // serialize <msgType, ts, value, counter>
		  pkts << msgT << " " << m_ts << " " << m_value << " " << msgC;

		  SetFill(pkts.str());

		Ptr<Packet> p;
		  if (m_dataSize)
		    {
		      NS_ASSERT_MSG (m_dataSize == m_size, "AbdClient::HandleSend(): m_size and m_dataSize inconsistent");
		      NS_ASSERT_MSG (m_data, "AbdClient::HandleSend(): m_dataSize but no m_data");
		      p = Create<Packet> (m_data, m_dataSize);
		    }
		  else
		    {
		      p = Create<Packet> (m_size);
		    }

		//socket->SendTo (p, 0, from);
		socket->Send (p);
		m_sent++;     //!< sent messages counter
		AsmCommon::Reset(sstm);
		sstm << "Sent " << packet->GetSize () << " bytes to " <<
				InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				InetSocketAddress::ConvertFrom (from).GetPort ();
		LogInfo(sstm);
	}
}

} // Namespace ns3
