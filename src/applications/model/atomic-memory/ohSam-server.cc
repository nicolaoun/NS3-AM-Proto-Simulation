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
#include "ohSam-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ohSamServerApplication");

NS_OBJECT_ENSURE_REGISTERED (ohSamServer);

void
ohSamServer::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[SERVER "<< m_personalID << " - " << Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}


TypeId
ohSamServer::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::ohSamServer")
    				.SetParent<Application> ()
					.SetGroupName("Applications")
					.AddConstructor<ohSamServer> ()
					.AddAttribute ("Port", "Port on which we listen for incoming packets.",
							UintegerValue (9),
							MakeUintegerAccessor (&ohSamServer::m_port),
							MakeUintegerChecker<uint16_t> ())
					.AddAttribute ("PacketSize", "Size of echo data in outbound packets",
							UintegerValue (100),
							MakeUintegerAccessor (&ohSamServer::m_size),
							MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("LocalAddress",
							"The local Address of the current node",
							AddressValue (),
							MakeAddressAccessor (&ohSamServer::m_myAddress),
							MakeAddressChecker ())
					.AddAttribute ("ID", 
                     		"Client ID",
                   	 		UintegerValue (100),
                  	 		MakeUintegerAccessor (&ohSamServer::m_personalID),
                  	 		MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("MaxFailures",
					  		"The maximum number of server failures",
					  		UintegerValue (100),
					  		MakeUintegerAccessor (&ohSamServer::m_fail),
					  		MakeUintegerChecker<uint32_t> ())
	;
	return tid;
}

ohSamServer::ohSamServer ()
{
	NS_LOG_FUNCTION (this);
	m_id = 0;
	m_ts = 0;
	m_value = 0;
	m_serversConnected =0;
	m_sent=0;
	m_operations.resize(100);
	m_relays.resize(100);

}

ohSamServer::~ohSamServer()
{
	NS_LOG_FUNCTION (this);
	//m_socket = 0;
	m_id = 0;
	m_ts = 0;
	m_value = 0;
	m_sent=0;
	m_serversConnected =0;
	m_operations.resize(100);
	m_relays.resize(100);
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/

void
ohSamServer::SetServers (std::vector<Address> ip)
{
	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}

void
ohSamServer::SetClients (std::vector<Address> ip)
{
	m_clntAddress = ip;
	m_numClients = m_clntAddress.size();

	for (unsigned i=0; i<m_clntAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_clntAddress[i]));
	}
}


void 
ohSamServer::StartApplication (void)
{
	NS_LOG_FUNCTION (this);
	
	std::stringstream sstm;
	
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
				NS_FATAL_ERROR ("Error: Server " << m_personalID << m_fail << " Failed to join multicast group.");
			}
		}

		m_socket->SetRecvCallback (MakeCallback (&ohSamServer::HandleRead, this));

			// Accept new connection
			m_socket->SetAcceptCallback (
					MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
					MakeCallback (&ohSamServer::HandleAccept, this));
			// Peer socket close handles
			m_socket->SetCloseCallbacks (
					MakeCallback (&ohSamServer::HandlePeerClose, this),
					MakeCallback (&ohSamServer::HandlePeerError, this));

	}


	//connect to the other servers
	if ( m_srvSocket.empty() )
	{
		//Set the number of sockets we need
		m_srvSocket.resize( m_serverAddress.size() );

		for (uint32_t i = 0; i < m_serverAddress.size(); i++ )
		{
			AsmCommon::Reset(sstm);
			sstm << "Connecting to SERVER (" << Ipv4Address::ConvertFrom(m_serverAddress[i]) << ")";
			LogInfo(sstm);

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_srvSocket[i] = Socket::CreateSocket (GetNode (), tid);

			m_srvSocket[i]->Bind();
			m_srvSocket[i]->Listen ();
			m_srvSocket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress[i]), m_port));

			m_srvSocket[i]->SetRecvCallback (MakeCallback (&ohSamServer::HandleRead, this));
			m_srvSocket[i]->SetAllowBroadcast (false);

			m_srvSocket[i]->SetConnectCallback (
					MakeCallback (&ohSamServer::ConnectionSucceeded, this),
					MakeCallback (&ohSamServer::ConnectionFailed, this));

		}
	}

	//connect to the other servers
	if ( m_clntSocket.empty() )
	{
		//Set the number of sockets we need
		m_clntSocket.resize( m_clntAddress.size() );

		for (uint32_t i = 0; i < m_clntAddress.size(); i++ )
		{
			AsmCommon::Reset(sstm);
			sstm << "Connecting to SERVER (" << Ipv4Address::ConvertFrom(m_clntAddress[i]) << ")";
			LogInfo(sstm);

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_clntSocket[i] = Socket::CreateSocket (GetNode (), tid);

			m_clntSocket[i]->Bind();
			m_clntSocket[i]->Listen();
			m_clntSocket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_clntAddress[i]), m_port));

			m_clntSocket[i]->SetRecvCallback (MakeCallback (&ohSamServer::HandleRead, this));
			m_clntSocket[i]->SetAllowBroadcast (false);

			m_clntSocket[i]->SetConnectCallback (
					MakeCallback (&ohSamServer::ConnectionSucceeded, this),
					MakeCallback (&ohSamServer::ConnectionFailed, this));
		}
	}
}


void 
ohSamServer::StopApplication ()
{
	NS_LOG_FUNCTION (this);

	if (m_socket != 0)
	{
		m_socket->Close ();
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}

	if ( !m_srvSocket.empty() )
	{
		for(uint32_t i=0; i< m_srvSocket.size(); i++ )
		{
			m_srvSocket[i]->Close ();
			m_srvSocket[i]->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
		}
	}
	if ( !m_clntSocket.empty() )
	{
		for(uint32_t i=0; i< m_clntSocket.size(); i++ )
		{
			m_clntSocket[i]->Close ();
			m_clntSocket[i]->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
		}
	}
	std::stringstream sstm;
	sstm << "** SERVER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent<<" **";
	LogInfo(sstm);
}

void
ohSamServer::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}


/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void ohSamServer::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void ohSamServer::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void ohSamServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&ohSamServer::HandleRead, this));
	m_socketList.push_back (s);
}

 void ohSamServer::ConnectionSucceeded (Ptr<Socket> socket)
 {
   NS_LOG_FUNCTION (this << socket);
   Address from;
   socket->GetPeerName (from);

   m_serversConnected++;

   std::stringstream sstm;
   sstm << "Connected to NODE (" << InetSocketAddress::ConvertFrom (from).GetIpv4() <<")";
   LogInfo(sstm);

   // Check if connected to the all the servers start operations
   //if (m_serversConnected == m_serverAddress.size() )
   if (m_serversConnected == (m_serverAddress.size() + m_clntAddress.size()) )
   {
 	AsmCommon::Reset(sstm);
 	sstm << "Connected to all Nodes.";
 	LogInfo(sstm);
   }
 }

 void ohSamServer::ConnectionFailed (Ptr<Socket> socket)
 {
   NS_LOG_FUNCTION (this << socket);
 }

/**************************************************************************************
 * PACKET DATA Handler
 **************************************************************************************/
void
ohSamServer::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize) 
    {
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);
  m_size = dataSize;
}

/**************************************************************************************
 * ohSam Rcv Handler
 **************************************************************************************/
void 
ohSamServer::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;


	Address msgAddr;
	uint32_t msgT, msgTs, msgV, msgOp, msgSenderID, reply_type;
	std::stringstream sstm;
	std::string message_type = "";
	std::string message_response_type = "";
	std::stringstream pkts;
	AsmCommon::Reset(pkts);
	AsmCommon::Reset(sstm);



	while ((packet = socket->RecvFrom (from)))
	{

		//deserialize the contents of the packet
		uint8_t buf[1024];
		packet->CopyData(buf,1024);
		std::stringbuf sb;
		sb.str(std::string((char*) buf));
		std::istream istm(&sb);
		istm >> msgT;


		if (msgT==WRITE){
			message_type = "write";
			istm >> msgTs >> msgV;
		}else if (msgT==READ){
			message_type = "read";
			istm >> msgSenderID >> msgOp;
		}else{
			message_type = "readRelay";
			istm >> msgTs >> msgV >> msgSenderID >> msgOp;
		}
		
		//// CASE WRITE
		if (msgT==WRITE){
			
			AsmCommon::Reset(sstm);
					sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
							InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
							InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
					LogInfo(sstm);

			NS_LOG_LOGIC ("Updating Local Info");

			if (m_ts < msgTs)
			{
				m_ts = msgTs;
				m_value = msgV;
				message_response_type = "writeAck";
				reply_type = WRITEACK;
			}
			NS_LOG_LOGIC ("Echoing packet");

			AsmCommon::Reset(pkts);
			pkts << reply_type << " " << m_ts << " " << m_value;
			SetFill(pkts.str());

			Ptr<Packet> p;
		  		if (m_dataSize)
		    	{
		      		NS_ASSERT_MSG (m_dataSize == m_size, "ohSamServer::HandleSend(): m_size and m_dataSize inconsistent");
		      		NS_ASSERT_MSG (m_data, "ohSamServer::HandleSend(): m_dataSize but no m_data");
		      		p = Create<Packet> (m_data, m_dataSize);
		    	}
		  		else
		   		{
		    		p = Create<Packet> (m_size);
		    	}

			
		  	socket->Send (p);
		  	m_sent++; //count the sent messages
			AsmCommon::Reset(sstm);
			sstm << "Sent "<< message_response_type <<" " << p->GetSize () << " bytes to " <<
				InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				InetSocketAddress::ConvertFrom (from).GetPort ();
			LogInfo(sstm);
			p->RemoveAllPacketTags ();
			p->RemoveAllByteTags ();
		}
		else if (msgT == READ)
		{
			
			AsmCommon::Reset(sstm);
					sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
							InetSocketAddress::ConvertFrom (from).GetIpv4 () << "port " <<
							InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
					LogInfo(sstm);

			
			message_response_type = "readRelay";
			msgT = READRELAY;
			AsmCommon::Reset(pkts);
			pkts << msgT << " " << m_ts << " " << m_value << " " << msgSenderID << " " << msgOp;
			SetFill(pkts.str());

			Ptr<Packet> pc;
		  		if (m_dataSize)
		    	{
		      		NS_ASSERT_MSG (m_dataSize == m_size, "ohSamServer::HandleSend(): m_size and m_dataSize inconsistent");
		      		NS_ASSERT_MSG (m_data, "ohSamServer::HandleSend(): m_dataSize but no m_data");
		      		pc = Create<Packet> (m_data, m_dataSize);
		    	}
		  		else
		   		{
		    		pc = Create<Packet> (m_size);
		    	}

			//Send a single packet to each server
  			for (uint32_t i=0; i<m_serverAddress.size(); i++)
  			{
	  			m_sent++; //increase here to count also "our" never sent to ourselves message :) 
  				if (m_serverAddress[i] != m_myAddress)
  				{
  					m_srvSocket[i]->Send(pc);

  					AsmCommon::Reset(sstm);
  					sstm << "Sent "<< message_response_type << " " << pc->GetSize () << " bytes to " << Ipv4Address::ConvertFrom (m_serverAddress[i]);
  					LogInfo ( sstm );
  				}
  			}
  			pc->RemoveAllPacketTags ();
			pc->RemoveAllByteTags ();
		}
		else{
			/////////////////////////////////
			////////// READ RELAY ///////////
			/////////////////////////////////

			AsmCommon::Reset(sstm);
			sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
					InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
					InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
			LogInfo(sstm);

			NS_LOG_LOGIC ("Updating Local Info");


			if (m_ts < msgTs)
			{
				m_ts = msgTs;
				m_value = msgV;
			}

			if (m_operations[msgSenderID] < msgOp)
			{
				m_operations[msgSenderID] = msgOp;
				m_relays[msgSenderID] = 1;
			}

			if (m_operations[msgSenderID] == msgOp)
			{
				m_relays[msgSenderID] ++;
			}

			if (m_relays[msgSenderID] >= (m_numServers - m_fail))
			{
				// REPLY back to the reader
				message_response_type = "readAck";
				msgT = READACK;
				AsmCommon::Reset(pkts);
				pkts << msgT << " " << m_ts << " " << m_value << " " << msgOp;
				SetFill(pkts.str());

				Ptr<Packet> pk;
				if (m_dataSize)
				{
					NS_ASSERT_MSG (m_dataSize == m_size, "ohSamServer::HandleSend(): m_size and m_dataSize inconsistent");
					NS_ASSERT_MSG (m_data, "ohSamServer::HandleSend(): m_dataSize but no m_data");
					pk = Create<Packet> (m_data, m_dataSize);
				}
				else
				{
					pk = Create<Packet> (m_size);
				}

				//Send to the corresponding client (from the info of the message is msgSenderId)
				m_clntSocket[msgSenderID]->Send(pk);
				m_sent++;
				AsmCommon::Reset(sstm);
				sstm << "Sent " << message_response_type <<" "<< pk->GetSize () << " bytes to " << Ipv4Address::ConvertFrom (m_clntAddress[msgSenderID]);
				LogInfo ( sstm );
				pk->RemoveAllPacketTags ();
				pk->RemoveAllByteTags ();

				//reset the replies for that reader to one (to count ours)
				m_relays[msgSenderID] =1;
			}
		}

	}
}


} // Namespace ns3
