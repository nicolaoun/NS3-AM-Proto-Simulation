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
#include "ohMam-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OhMamServerApplication");

NS_OBJECT_ENSURE_REGISTERED (OhMamServer);

void
OhMamServer::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[SERVER "<< m_personalID << " - " << Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}


TypeId
OhMamServer::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::OhMamServer")
    				.SetParent<Application> ()
					.SetGroupName("Applications")
					.AddConstructor<OhMamServer> ()
					.AddAttribute ("Port", "Port on which we listen for incoming packets.",
							UintegerValue (9),
							MakeUintegerAccessor (&OhMamServer::m_port),
							MakeUintegerChecker<uint16_t> ())
					.AddAttribute ("PacketSize", "Size of echo data in outbound packets",
							UintegerValue (100),
							MakeUintegerAccessor (&OhMamServer::m_size),
							MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("LocalAddress",
							"The local Address of the current node",
							AddressValue (),
							MakeAddressAccessor (&OhMamServer::m_myAddress),
							MakeAddressChecker ())
					.AddAttribute ("ID", 
                     		"Client ID",
                   	 		UintegerValue (100),
                  	 		MakeUintegerAccessor (&OhMamServer::m_personalID),
                  	 		MakeUintegerChecker<uint32_t> ())
					.AddAttribute ("MaxFailures",
					  		"The maximum number of server failures",
					  		UintegerValue (100),
					  		MakeUintegerAccessor (&OhMamServer::m_fail),
					  		MakeUintegerChecker<uint32_t> ())
	;
	return tid;
}

OhMamServer::OhMamServer ()
{
	NS_LOG_FUNCTION (this);
	//m_socket = 0;
	m_id = 0;
	m_ts = 0;
	m_value = 0;
	m_serversConnected =0;
	//we have to put the number of servers + writers + readers there
	m_writeop.resize(100);
	//has to be readers size
	m_operations.resize(100);
	m_relays.resize(100);

}

OhMamServer::~OhMamServer()
{
	NS_LOG_FUNCTION (this);
	//m_socket = 0;
	m_id = 0;
	m_ts = 0;
	m_value = 0;
	m_serversConnected =0;
	//we have to put the number of clients there
	m_writeop.resize(100);
	//has to be readers size
	m_operations.resize(100);
	m_relays.resize(100);
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/

void
OhMamServer::SetServers (std::vector<Address> ip)
{
	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}

void
OhMamServer::SetClients (std::vector<Address> ip)
{
	m_clntAddress = ip;
	m_numClients = m_clntAddress.size();

	for (unsigned i=0; i<m_clntAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_clntAddress[i]));
	}
}


void 
OhMamServer::StartApplication (void)
{
	NS_LOG_FUNCTION (this);
	
	std::stringstream sstm;
	// AsmCommon::Reset(sstm);
	// sstm << "Check0";
	// LogInfo(sstm);

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

		m_socket->SetRecvCallback (MakeCallback (&OhMamServer::HandleRead, this));

			// Accept new connection
			m_socket->SetAcceptCallback (
					MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
					MakeCallback (&OhMamServer::HandleAccept, this));
			// Peer socket close handles
			m_socket->SetCloseCallbacks (
					MakeCallback (&OhMamServer::HandlePeerClose, this),
					MakeCallback (&OhMamServer::HandlePeerError, this));

	}


	//connect to the other servers
	if ( m_srvSocket.empty() )
	{
		//Set the number of sockets we need
		m_srvSocket.resize( m_serverAddress.size() );

		for (int i = 0; i < m_serverAddress.size(); i++ )
		{
			AsmCommon::Reset(sstm);
			sstm << "Connecting to SERVER (" << Ipv4Address::ConvertFrom(m_serverAddress[i]) << ")";
			LogInfo(sstm);

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_srvSocket[i] = Socket::CreateSocket (GetNode (), tid);

			m_srvSocket[i]->Bind();
			m_srvSocket[i]->Listen ();
			m_srvSocket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress[i]), m_port));

			m_srvSocket[i]->SetRecvCallback (MakeCallback (&OhMamServer::HandleRead, this));
			m_srvSocket[i]->SetAllowBroadcast (false);

			m_srvSocket[i]->SetConnectCallback (
					MakeCallback (&OhMamServer::ConnectionSucceeded, this),
					MakeCallback (&OhMamServer::ConnectionFailed, this));

		}
	}

	//connect to the other servers
	if ( m_clntSocket.empty() )
	{
		//Set the number of sockets we need
		m_clntSocket.resize( m_clntAddress.size() );

		for (int i = 0; i < m_clntAddress.size(); i++ )
		{
			AsmCommon::Reset(sstm);
			sstm << "Connecting to SERVER (" << Ipv4Address::ConvertFrom(m_clntAddress[i]) << ")";
			LogInfo(sstm);

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_clntSocket[i] = Socket::CreateSocket (GetNode (), tid);

			m_clntSocket[i]->Bind();
			m_clntSocket[i]->Listen();
			m_clntSocket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_clntAddress[i]), m_port));

			m_clntSocket[i]->SetRecvCallback (MakeCallback (&OhMamServer::HandleRead, this));
			m_clntSocket[i]->SetAllowBroadcast (false);

			m_clntSocket[i]->SetConnectCallback (
					MakeCallback (&OhMamServer::ConnectionSucceeded, this),
					MakeCallback (&OhMamServer::ConnectionFailed, this));
		}
	}
}

// void 
// OhMamServer::StopApplication ()
// {
// 	NS_LOG_FUNCTION (this);

// 	if ( !m_socket.empty() )
//     {
// 	  for(int i=0; i< m_socket.size(); i++ )
// 	  {
// 		  m_socket[i]->Close ();
// 		  m_socket[i]->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
// 	  }
//     }
// }

void 
OhMamServer::StopApplication ()
{
	NS_LOG_FUNCTION (this);

	if (m_socket != 0)
	{
		m_socket->Close ();
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}

	if ( !m_srvSocket.empty() )
	{
		for(int i=0; i< m_srvSocket.size(); i++ )
		{
			m_srvSocket[i]->Close ();
			m_srvSocket[i]->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
		}
	}
	if ( !m_clntSocket.empty() )
	{
		for(int i=0; i< m_clntSocket.size(); i++ )
		{
			m_clntSocket[i]->Close ();
			m_clntSocket[i]->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
		}
	}
}

void
OhMamServer::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	Application::DoDispose ();
}


/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void OhMamServer::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void OhMamServer::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void OhMamServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&OhMamServer::HandleRead, this));
	m_socketList.push_back (s);
}

 void OhMamServer::ConnectionSucceeded (Ptr<Socket> socket)
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

 void OhMamServer::ConnectionFailed (Ptr<Socket> socket)
 {
   NS_LOG_FUNCTION (this << socket);
 }

/**************************************************************************************
 * PACKET DATA Handler
 **************************************************************************************/
void
OhMamServer::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      //if (m_data )
    //  delete [] m_data;
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
 * OhMam Rcv Handler
 **************************************************************************************/
void 
OhMamServer::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);

	Ptr<Packet> packet;
	Address from;


	// Added the msgId, msgWriteOp
	Address msgAddr;
	uint32_t msgT, msgTs, msgId, msgV, msgOp ,msgC, msgSenderID, reply_type;
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
		
		//NN: THIS IS WRONG
		//istm >> msgT >> msgTs >> msgId >> msgV >> msgOp >> msgC >> msgSenderID;

		istm >> msgT;


		if (msgT==WRITE)
			message_type = "write";
		else if (msgT==DISCOVER)
			message_type = "discover";
		else if (msgT==READ)
			message_type = "read";
		else
			message_type = "readRelay";

		
		//// CASE WRITE
		if ((msgT==WRITE) || (msgT==DISCOVER)){
			
			istm >> msgTs >> msgId >> msgV >> msgOp >> msgC;

			AsmCommon::Reset(sstm);
					sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
							InetSocketAddress::ConvertFrom (from).GetIpv4 () << "(ID:" << msgId << ") port " <<
							InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
					LogInfo(sstm);

			message_response_type = "discoverAck";
			reply_type = DISCOVERACK;
			NS_LOG_LOGIC ("Updating Local Info");

			if (((m_ts < msgTs) || ((m_ts == msgTs)&&(m_id<msgId))) && (m_writeop[msgId] < msgOp) && (msgT==WRITE))
			{
				m_ts = msgTs;
				m_value = msgV;
				m_id = msgId;
				message_response_type = "writeAck";
				reply_type = WRITEACK;
			}
			NS_LOG_LOGIC ("Echoing packet");
			// Prepare packet content
			// serialize <msgType, ts, id, value, msgOp ,counter>
			AsmCommon::Reset(pkts);
			pkts << reply_type << " " << m_ts << " " << m_id << " " << m_value << " " << msgOp << " " << msgC;
			SetFill(pkts.str());

			Ptr<Packet> p;
		  		if (m_dataSize)
		    	{
		      		NS_ASSERT_MSG (m_dataSize == m_size, "OhMamServer::HandleSend(): m_size and m_dataSize inconsistent");
		      		NS_ASSERT_MSG (m_data, "OhMamServer::HandleSend(): m_dataSize but no m_data");
		      		p = Create<Packet> (m_data, m_dataSize);
		    	}
		  		else
		   		{
		    		p = Create<Packet> (m_size);
		    	}

			//socket->SendTo (p, 0, from);
		  	socket->Send (p);

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
			
			istm >> msgTs >> msgId >> msgV >> msgOp >> msgC;
			//istm >> msgOp >> msgC >> msgSenderID;

			AsmCommon::Reset(sstm);
					sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
							InetSocketAddress::ConvertFrom (from).GetIpv4 () << "port " <<
							InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
					LogInfo(sstm);

			//Broadcast a ReadRelay to all the servers! 
			//will need the readerId here -> msgId
			// from is the address that we will eventually reply back the ReadAck

			message_response_type = "readRelay";
			msgT = READRELAY;
			AsmCommon::Reset(pkts);

			AsmCommon::Reset(sstm);
			sstm << "Value:" << m_value << " " << msgV;
			LogInfo(sstm);

			//m_value = 807;

			// NN: THE BELOW IS WRONG
			////// Fanos: I want the message to be <READRELAY, <ts,id>,value, readop, counter, who_was_the_initial_sender>
			//pkts << msgT << " " << m_ts << " " << m_id << " " << m_value << " " << msgOp << " " << msgC << " " << msgId;      //Initial
			//pkts << msgT << " " << m_ts << " " << msgId << " " << msgV << " " << msgOp << " " << msgC << " " << m_personalID; //replaced with this
			
			//pkts << msgT << " " << m_ts << " " << m_id << " " << msgV << " " << msgOp << " " << msgC << " " << msgId;	//Test3 works
			pkts << msgT << " " << m_ts << " " << m_id << " " << m_value << " " << msgOp << " " << msgC << " " << msgId;	//Test4

			//pkts << msgT << " " << m_ts << " " << m_id << " " << m_value << " " << msgOp << " " << msgC << " " << msgSenderID; //Test 1 small

			SetFill(pkts.str());

			Ptr<Packet> pc;
		  		if (m_dataSize)
		    	{
		      		NS_ASSERT_MSG (m_dataSize == m_size, "OhMamServer::HandleSend(): m_size and m_dataSize inconsistent");
		      		NS_ASSERT_MSG (m_data, "OhMamServer::HandleSend(): m_dataSize but no m_data");
		      		pc = Create<Packet> (m_data, m_dataSize);
		    	}
		  		else
		   		{
		    		pc = Create<Packet> (m_size);
		    	}

			//Send a single packet to each server
  			for (int i=0; i<m_serverAddress.size(); i++)
  			{
	  			
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

			istm >> msgTs >> msgId >> msgV >> msgOp >> msgC >> msgSenderID;


			AsmCommon::Reset(sstm);
			sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
					InetSocketAddress::ConvertFrom (from).GetIpv4 () << "(ID:" << msgSenderID << ") port " <<
					InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf << " Value " << msgV << " SenderID " << msgSenderID;
			LogInfo(sstm);

			NS_LOG_LOGIC ("Updating Local Info");


			if ((m_ts < msgTs) || ((m_ts == msgTs)&&(m_id<msgId)))
			{
				m_ts = msgTs;
				m_value = msgV;
				m_id = msgId;
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
				pkts << msgT << " " << m_ts << " " << m_id << " " << m_value << " " << msgOp << " " << msgC;
				SetFill(pkts.str());

				Ptr<Packet> pk;
				if (m_dataSize)
				{
					NS_ASSERT_MSG (m_dataSize == m_size, "OhMamServer::HandleSend(): m_size and m_dataSize inconsistent");
					NS_ASSERT_MSG (m_data, "OhMamServer::HandleSend(): m_dataSize but no m_data");
					pk = Create<Packet> (m_data, m_dataSize);
				}
				else
				{
					pk = Create<Packet> (m_size);
				}

				//Send to the corresponding client (from the info of the message is msgSenderId)
				m_clntSocket[msgSenderID]->Send(pk);
				AsmCommon::Reset(sstm);
				sstm << "Sent " << message_response_type <<" "<< pk->GetSize () << " bytes to " << Ipv4Address::ConvertFrom (m_clntAddress[msgSenderID]);
				LogInfo ( sstm );
				pk->RemoveAllPacketTags ();
				pk->RemoveAllByteTags ();

				//zero the replies for that reader
				m_relays[msgSenderID] =1;
			}
		}

	}
}


} // Namespace ns3
