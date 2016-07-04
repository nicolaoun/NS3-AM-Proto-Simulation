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
#include "ohfast-server.h"
//#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OhFastServerApplication");

NS_OBJECT_ENSURE_REGISTERED (OhFastServer);

void
OhFastServer::Log( logLevel_t l, std::stringstream& s)
{
	if ( l == INFO )
	{
		NS_LOG_INFO("[SERVER "<< m_personalID << " - " << Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
	}
	else
	{
		NS_LOG_DEBUG("[SERVER "<< m_personalID << " - " << Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
	}
}

TypeId
OhFastServer::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::OhFastServer")
    				.SetParent<Application> ()
					.SetGroupName("Applications")
					.AddConstructor<OhFastServer> ()
					.AddAttribute ("Port", "Port on which we listen for incoming packets.",
							UintegerValue (9),
							MakeUintegerAccessor (&OhFastServer::m_port),
							MakeUintegerChecker<uint16_t> ())
					.AddAttribute ("PacketSize", "Size of echo data in outbound packets",
							UintegerValue (100),
							MakeUintegerAccessor (&OhFastServer::m_size),
							MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("LocalAddress",
							"The local Address of the current node",
							AddressValue (),
							MakeAddressAccessor (&OhFastServer::m_myAddress),
							MakeAddressChecker ())
					.AddAttribute ("ID", 
                     		"Client ID",
                   	 		UintegerValue (100),
                  	 		MakeUintegerAccessor (&OhFastServer::m_personalID),
                  	 		MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("MaxFailures",
					  		"The maximum number of server failures",
					  		UintegerValue (100),
					  		MakeUintegerAccessor (&OhFastServer::m_fail),
					  		MakeUintegerChecker<uint32_t> ())
	;
	return tid;
}

OhFastServer::OhFastServer ()
{
	NS_LOG_FUNCTION (this);
	//m_id = 0;
	m_ts = 0;
	m_value = 0;
	m_pvalue = 0;
	m_serversConnected =0;
	m_sent=0;

	m_numClients=0;
}

OhFastServer::~OhFastServer()
{
	NS_LOG_FUNCTION (this);
	//m_socket = 0;
	//m_id = 0;
	m_ts = 0;
	m_value = 0;
	m_pvalue = 0;
	m_sent=0;
	m_serversConnected =0;

	m_numClients=0;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/

void
OhFastServer::SetServers (std::vector<Address> ip)
{
	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}


void 
OhFastServer::StartApplication (void)
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

		m_socket->SetRecvCallback (MakeCallback (&OhFastServer::HandleRead, this));

			// Accept new connection
			m_socket->SetAcceptCallback (
					MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
					MakeCallback (&OhFastServer::HandleAccept, this));
			// Peer socket close handles
			m_socket->SetCloseCallbacks (
					MakeCallback (&OhFastServer::HandlePeerClose, this),
					MakeCallback (&OhFastServer::HandlePeerError, this));

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
			Log(DEBUG, sstm);

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_srvSocket[i] = Socket::CreateSocket (GetNode (), tid);

			m_srvSocket[i]->Bind();
			m_srvSocket[i]->Listen ();
			m_srvSocket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress[i]), m_port));

			m_srvSocket[i]->SetRecvCallback (MakeCallback (&OhFastServer::HandleRead, this));
			m_srvSocket[i]->SetAllowBroadcast (false);

			m_srvSocket[i]->SetConnectCallback (
					MakeCallback (&OhFastServer::ConnectionSucceeded, this),
					MakeCallback (&OhFastServer::ConnectionFailed, this));

		}
	}
}


void 
OhFastServer::StopApplication ()
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
	Log(INFO, sstm);
}

void
OhFastServer::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}


/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void OhFastServer::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void OhFastServer::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void OhFastServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	//Address from;
	bool isServer = false;
	std::stringstream sstm;

	s->SetRecvCallback (MakeCallback (&OhFastServer::HandleRead, this));
	//s->GetPeerName(from);

	for (uint32_t i=0; i < m_serverAddress.size(); i++)
	{
		if ( InetSocketAddress::ConvertFrom(from).GetIpv4() == m_serverAddress[i] )
		{
			isServer = true;
		}
	}

	if( !isServer )
	{
		m_clntAddress.push_back(from);
		m_clntSocket.push_back(s);
		m_numClients++;

		m_relayTs.resize(m_numClients);
		m_relays.resize(m_numClients);

		AsmCommon::Reset(sstm);
		sstm << "ACCEPTED CLIENT " << m_clntAddress.size() << ": " << InetSocketAddress::ConvertFrom(from).GetIpv4();
		Log(DEBUG, sstm);
	}
	else
	{
		m_socketList.push_back (s);

		AsmCommon::Reset(sstm);
		sstm << "ACCEPTED SERVER " << m_socketList.size() << ": " << InetSocketAddress::ConvertFrom(from).GetIpv4();
		Log(DEBUG, sstm);
	}
}

 void OhFastServer::ConnectionSucceeded (Ptr<Socket> socket)
 {
   NS_LOG_FUNCTION (this << socket);
   Address from;
   socket->GetPeerName (from);

   m_serversConnected++;

   std::stringstream sstm;
   sstm << "Connected to NODE (" << InetSocketAddress::ConvertFrom (from).GetIpv4() <<")";
   Log(DEBUG, sstm);

   // Check if connected to the all the servers start operations
   //if (m_serversConnected == m_serverAddress.size() )
   if (m_serversConnected == (m_serverAddress.size() + m_clntAddress.size()) )
   {
 	AsmCommon::Reset(sstm);
 	sstm << "Connected to all Nodes.";
 	Log(DEBUG, sstm);
   }
 }

 void OhFastServer::ConnectionFailed (Ptr<Socket> socket)
 {
   NS_LOG_FUNCTION (this << socket);
 }

/**************************************************************************************
 * PACKET DATA Handler
 **************************************************************************************/
void
OhFastServer::SetFill (std::string fill)
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
 * OhFast Rcv Handler
 **************************************************************************************/
void 
OhFastServer::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;

	uint32_t msgT;
	std::stringstream sstm;
	std::string message_type = "";
	MessageType replyT;
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
			replyT = WRITEACK;
		}else if (msgT==READ){
			message_type = "read";
			replyT = READACK;
		}else{
			message_type = "readRelay";
		}

		//istm >> msgOp >> msgTs >> msgV >> msgVp >> msgSenderID;

		AsmCommon::Reset(sstm);
		sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
				InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
		Log(DEBUG, sstm);

		if ( msgT == WRITE || msgT == READ )
		{
			HandleRecvMsg(istm, socket, replyT);
		}
		else
		{
			HandleRelay(istm, socket);
		}
	}
}

void
OhFastServer::HandleRecvMsg(std::istream& istm, Ptr<Socket> socket, MessageType replyT)
{
	uint32_t msgTs, msgV, msgVp, msgOp;
	int msgSenderID = -1;
	std::stringstream sstm;
	std::string message_type = "";
	std::string message_response_type = "";
	std::stringstream pkts;
	Address from;

	AsmCommon::Reset(pkts);
	AsmCommon::Reset(sstm);

	socket->GetPeerName(from);

	// the writer does not send its id
	if ( replyT == WRITEACK )
	{
		istm >> msgTs >> msgV >> msgVp >> msgOp;
	}
	else
	{
		istm >> msgTs >> msgV >> msgVp >> msgSenderID >> msgOp;
	}

	//find if the socket that client is connected to
	for (uint32_t i=0; i < m_clntAddress.size(); i++)
	{
		if ( InetSocketAddress::ConvertFrom(from).GetIpv4() == InetSocketAddress::ConvertFrom(m_clntAddress[i]).GetIpv4() )
		{
			msgSenderID = i;
			break;	//stop at the first client we find
		}
	}


	// if not sender detected - drop the package
	if ( ( msgSenderID >= 0 && msgSenderID < (int) m_clntAddress.size() )|| m_fail == 0)
	{
		if ( m_ts < msgTs )
		{
			NS_LOG_LOGIC ("Updating Local Info (ts and seen set)");
			m_ts = msgTs;
			m_value = msgV;
			m_pvalue = msgVp;

			//reinitialize the seen set
			m_seen.clear();

			//reset propagate flag
			m_tsSecured = false;
		}

		//insert the sender in the seen set
		m_seen.insert( InetSocketAddress::ConvertFrom(from).GetIpv4() );

		AsmCommon::Reset(sstm);
		sstm << "SeenSize: "<< m_seen.size() << ", Bound: (" << m_numServers << "/" << m_fail << ")-2 = " << ((m_numServers/m_fail)-2)
		 	 << ", TsSecured: " << m_tsSecured << ", RelayTs: " << m_relayTs[msgSenderID] << ", ServerTs: " << m_ts;
		Log(DEBUG,  sstm );

		// check condition to move to relay phase
		if ( m_seen.size() > ((m_numServers/m_fail) - 2) && !m_tsSecured && m_relayTs[msgSenderID] < m_ts)
		{
			message_response_type = "readRelay";
			m_relayTs[msgSenderID] = m_ts;
			m_relays[msgSenderID] = 1;

			int ipSize = from.GetSerializedSize();
			uint8_t ipBuffer[ipSize];
			from.CopyTo(ipBuffer);

			// prepare and send packet to all servers
			AsmCommon::Reset(pkts);
			// <msgType, <ts,v,vp>, q, counter>
			pkts << READRELAY << " " << m_ts << " " << m_value << " " << m_pvalue << " "<< ipSize << " " << ipBuffer << " " << msgOp;

			SetFill(pkts.str());

			Ptr<Packet> pc;
			if (m_dataSize)
			{
				NS_ASSERT_MSG (m_dataSize == m_size, "OhFastServer::HandleSend(): m_size and m_dataSize inconsistent");
				NS_ASSERT_MSG (m_data, "OhFastServer::HandleSend(): m_dataSize but no m_data");
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
					sstm << "Sent "<< message_response_type << " " << pc->GetSize () << " bytes to " << Ipv4Address::ConvertFrom (m_serverAddress[i]) << ", seen size: " << m_seen.size();
					Log(DEBUG,  sstm );
				}
			}
		}
		else // reply to the sender without relaying
		{
			// prepare and send packet
			AsmCommon::Reset(pkts);
			// serialize <counter, msgType, <ts,v,vp>, |seen|, secured, initiator>
			pkts << msgOp << " " << replyT << " " << m_ts << " " << m_value << " " << m_pvalue << " " << m_seen.size() << " " << m_tsSecured << " " << false;

			SetFill(pkts.str());

			Ptr<Packet> p;
			if (m_dataSize)
			{
				NS_ASSERT_MSG (m_dataSize == m_size, "OhFastServer::HandleSend(): m_size and m_dataSize inconsistent");
				NS_ASSERT_MSG (m_data, "OhFastServer::HandleSend(): m_dataSize but no m_data");
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

			sstm << " { ";
			for(std::set< Address >::iterator it=m_seen.begin(); it!=m_seen.end(); it++)
				sstm << Ipv4Address::ConvertFrom( *it ) << ", ";

			sstm << "} data " << pkts.str();

			Log(DEBUG, sstm);
		}
	} // end if(msgSenderID)
}

void
OhFastServer::HandleRelay(std::istream& istm, Ptr<Socket> socket)
{
	/////////////////////////////////
	////////// READ RELAY ///////////
	/////////////////////////////////
	uint32_t msgTs, msgV, msgVp, msgOp, msgIpSize;
	int msgSenderID = -1;
	Address senderIp;
	std::stringstream sstm;
	std::string message_type = "";
	std::string message_response_type = "";
	std::stringstream pkts;
	AsmCommon::Reset(pkts);
	AsmCommon::Reset(sstm);
	Address from;

	socket->GetPeerName(from);

	istm >> msgTs >> msgV >> msgVp >> msgIpSize;

	char ipBuffer[msgIpSize];
	uint8_t ipBuffer2[msgIpSize];
	istm.read(ipBuffer, 1);	// flush the space
	istm.read(ipBuffer, msgIpSize-1);

	memcpy(ipBuffer2, ipBuffer, msgIpSize-1);

	istm >> msgOp;
	senderIp.CopyFrom(ipBuffer2, msgIpSize-1);

	//find if the socket that client is connected to
	for (uint32_t i=0; i < m_clntAddress.size(); i++)
	{
		if ( Ipv4Address::ConvertFrom(senderIp) == InetSocketAddress::ConvertFrom(m_clntAddress[i]).GetIpv4() )
		{
			msgSenderID = i;
			break;	//stop at the first client we find
		}
	}

	AsmCommon::Reset(sstm);
	sstm << "Processing ReadRelay: InitiatorIp= " << Ipv4Address::ConvertFrom(senderIp) << " InitiatorID=" << msgSenderID;// << ", relayTs=" << m_relayTs[msgSenderID] << ", msgTs=" << msgTs << ", #RelaysRcved=" << m_relays[msgSenderID];
	Log(DEBUG,  sstm );

	if ( msgSenderID >= 0 && msgSenderID < (int) m_clntAddress.size() )
	{
		if ( m_ts < msgTs )
		{
			NS_LOG_LOGIC ("Updating Local Info (ts and seen set)");
			m_ts = msgTs;
			m_value = msgV;
			m_pvalue = msgVp;

			//reinitialize the seen set
			m_seen.clear();

			//insert the client in the seen set
			m_seen.insert( InetSocketAddress::ConvertFrom(m_clntAddress[msgSenderID]).GetIpv4() );
		}
		else if ( m_ts  == msgTs )
		{
			//insert the client in the seen set
			m_seen.insert( InetSocketAddress::ConvertFrom(m_clntAddress[msgSenderID]).GetIpv4() );
		}


		if (m_relayTs[msgSenderID] == msgTs)
		{
			m_relays[msgSenderID] ++;

			if (m_relays[msgSenderID] >= (m_numServers - m_fail))
			{
				// if we have the timestamp that reached a majority - secure it
				if( m_ts == msgTs)
				{
					m_tsSecured = true;
				}

				// REPLY back to the reader
				message_response_type = "readAck";

				AsmCommon::Reset(pkts);
				// <counter, msgType, <ts,v,vp>, |seen|, secured, initiator>
				pkts << msgOp << " " << READACK << " " << m_ts << " " << m_value << " " << m_pvalue << " "<< m_seen.size() << " " << m_tsSecured << " " << true;

				SetFill(pkts.str());

				Ptr<Packet> pk;
				if (m_dataSize)
				{
					NS_ASSERT_MSG (m_dataSize == m_size, "OhFastServer::HandleSend(): m_size and m_dataSize inconsistent");
					NS_ASSERT_MSG (m_data, "OhFastServer::HandleSend(): m_dataSize but no m_data");
					pk = Create<Packet> (m_data, m_dataSize);
				}
				else
				{
					pk = Create<Packet> (m_size);
				}

				pk->RemoveAllPacketTags ();
				pk->RemoveAllByteTags ();

				//Send to the client that initiated the relay (from the info of the message is msgSenderId)
				m_clntSocket[msgSenderID]->Send(pk);
				m_sent++;
				AsmCommon::Reset(sstm);
				sstm << "Sent " << message_response_type <<" "<< pk->GetSize () << " bytes after RELAY to " << InetSocketAddress::ConvertFrom (m_clntAddress[msgSenderID]).GetIpv4();
				Log(DEBUG,  sstm );

				//reset the replies for that reader to one (to count ours)
				//m_relays[msgSenderID] =1;
			}

		}
		else if (m_relayTs[msgSenderID] < msgTs) // if this node did not relay a higher ts for the same client reply back
		{
			// someone else relayed this ts - echo his msg
			message_response_type = "readRelay";

			int ipSize = senderIp.GetSerializedSize();
			uint8_t ipBuffer3[ipSize];
			senderIp.CopyTo(ipBuffer3);

			AsmCommon::Reset(pkts);
			// serialize <msgType, <ts,v,vp>, q, counter>
			pkts << READRELAY << " " << msgTs << " " << msgV << " " << msgVp <<" "<< ipSize << " "<< ipBuffer3 << " " << msgOp;

			SetFill(pkts.str());

			Ptr<Packet> pk;
			if (m_dataSize)
			{
				NS_ASSERT_MSG (m_dataSize == m_size, "OhFastServer::HandleSend(): m_size and m_dataSize inconsistent");
				NS_ASSERT_MSG (m_data, "OhFastServer::HandleSend(): m_dataSize but no m_data");
				pk = Create<Packet> (m_data, m_dataSize);
			}
			else
			{
				pk = Create<Packet> (m_size);
			}

			//Send to the corresponding client (from the info of the message is msgSenderId)
			socket->Send(pk);
			m_sent++;
			AsmCommon::Reset(sstm);
			sstm << "Echo  " << message_response_type <<" "<< pk->GetSize () << " bytes to " << InetSocketAddress::ConvertFrom (from).GetIpv4() << " data " << pkts.str();
			Log(DEBUG,  sstm );
		}
	}
}


} // Namespace ns3
