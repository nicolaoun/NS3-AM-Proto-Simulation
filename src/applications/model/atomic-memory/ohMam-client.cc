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
#include "ns3/trace-source-accessor.h"
#include "ohMam-client.h"
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OhMamClientApplication");

NS_OBJECT_ENSURE_REGISTERED (OhMamClient);


void
OhMamClient::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[CLIENT " << m_personalID << " - "<< Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}



TypeId
OhMamClient::GetTypeId (void)
{
  //static vector<Address> tmp_address;
  static TypeId tid = TypeId ("ns3::OhMamClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<OhMamClient> ()
    .AddAttribute ("MaxOperations",
                   "The maximum number of operations to be invoked",
                   UintegerValue (100),
                   MakeUintegerAccessor (&OhMamClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("MaxFailures",
					  "The maximum number of server failures",
					  UintegerValue (100),
					  MakeUintegerAccessor (&OhMamClient::m_fail),
					  MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&OhMamClient::m_interval),
                   MakeTimeChecker ())
   .AddAttribute ("SetRole",
					  "The role of the client (reader/writer)",
					  UintegerValue (0),
					  MakeUintegerAccessor (&OhMamClient::m_prType),
					  MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("LocalAddress",
					  "The local Address of the current node",
					  AddressValue (),
					  MakeAddressAccessor (&OhMamClient::m_myAddress),
					  MakeAddressChecker ())
	.AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&OhMamClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&OhMamClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
					UintegerValue (9),
					MakeUintegerAccessor (&OhMamClient::m_port),
					MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&OhMamClient::SetDataSize,
                                         &OhMamClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&OhMamClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&OhMamClient::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

/**************************************************************************************
 * Constructors
 **************************************************************************************/
OhMamClient::OhMamClient ()
{
	NS_LOG_FUNCTION (this);
	m_sent = 0;
	//m_socket = 0;
	m_sendEvent = EventId ();
	m_data = 0;
	m_dataSize = 0;
	m_serversConnected = 0;

	m_ts = 0;					//initialize the local timestamp
	m_id = 0;					//initialize the id of the tag
	m_value = 0;				//initialize local value
	m_opStatus = PHASE1; 		//initialize status
	m_MINts = 10000000;
	m_MINId = 10000000;

	m_fail = 0;
}

OhMamClient::~OhMamClient()
{
	NS_LOG_FUNCTION (this);
	//m_socket = 0;

	delete [] m_data;
	m_data = 0;
	m_dataSize = 0;
	m_serversConnected = 0;

	m_ts = 0;					//initialize the local timestamp - here is the MIN_TS
	m_value = 0;				//initialize local value
	m_id = 0;					//initialize the id of the tag
	m_opStatus = PHASE1; 		//initialize status
	m_MINts = 10000000;
	m_MINId = 10000000;

	m_fail = 0;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/
void 
OhMamClient::StartApplication (void)
{

	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	//////////////////////////////////////////////////////////////////////////////
	if (m_insocket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
		m_insocket = Socket::CreateSocket (GetNode (), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
		m_insocket->Bind (local);
		m_insocket->Listen ();

		if (addressUtils::IsMulticast (m_local))
		{
			Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_insocket);
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

	}
	m_insocket->SetRecvCallback (MakeCallback (&OhMamClient::HandleRecv, this));

	// Accept new connection
	m_insocket->SetAcceptCallback (
			MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
			MakeCallback (&OhMamClient::HandleAccept, this));
	// Peer socket close handles
	m_insocket->SetCloseCallbacks (
			MakeCallback (&OhMamClient::HandlePeerClose, this),
			MakeCallback (&OhMamClient::HandlePeerError, this));
	//////////////////////////////////////////////////////////////////////////////

	if ( m_socket.empty() )
	{
		//Set the number of sockets we need
		m_socket.resize( m_serverAddress.size() );

		for (int i = 0; i < m_serverAddress.size(); i++ )
		{
			AsmCommon::Reset(sstm);
			sstm << "Connecting to SERVER (" << Ipv4Address::ConvertFrom(m_serverAddress[i]) << ")";
			LogInfo(sstm);

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_socket[i] = Socket::CreateSocket (GetNode (), tid);

			m_socket[i]->Bind();
			//m_socket[i]->Listen ();
			m_socket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress[i]), m_peerPort));

			m_socket[i]->SetRecvCallback (MakeCallback (&OhMamClient::HandleRecv, this));
			m_socket[i]->SetAllowBroadcast (false);

			m_socket[i]->SetConnectCallback (
				        MakeCallback (&OhMamClient::ConnectionSucceeded, this),
				        MakeCallback (&OhMamClient::ConnectionFailed, this));
		}
	}

	
	AsmCommon::Reset(sstm);
	sstm << "Started Succesfully: #S=" << m_numServers <<", #F=" << m_fail << ", opInt=" << m_interval;
	LogInfo(sstm);

}

void 
OhMamClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  std::stringstream sstm;

  if (m_insocket != 0)
	{
		m_insocket->Close ();
		m_insocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}

  if ( !m_socket.empty() )
    {
	  for(int i=0; i< m_socket.size(); i++ )
	  {
		  m_socket[i]->Close ();
		  m_socket[i]->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	  }
    }

  Simulator::Cancel (m_sendEvent);

  switch(m_prType)
  {
  case WRITER:
	  sstm << "** WRITER TERMINATED: #writes=" << m_opCount << ", AveOpTime="<< ( (m_opAve.GetSeconds()) /m_opCount) <<"s **";
	  LogInfo(sstm);
	  break;
  case READER:
	  sstm << "** READER TERMINATED: #reads=" << m_opCount << ", AveOpTime="<< ((m_opAve.GetSeconds())/m_opCount) <<"s **";
	  LogInfo(sstm);
	  break;
  }
}

void
OhMamClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void OhMamClient::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void OhMamClient::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void OhMamClient::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&OhMamClient::HandleRecv, this));
	m_socketList.push_back (s);
}



void OhMamClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Address from;
  socket->GetPeerName (from);

  m_serversConnected++;

  std::stringstream sstm;
  sstm << "Connected to SERVER (" << InetSocketAddress::ConvertFrom (from).GetIpv4() <<")";
  LogInfo(sstm);

  // Check if connected to the all the servers start operations
  if (m_serversConnected == m_serverAddress.size() )
  {
	  ScheduleOperation (m_interval);
  }
}

void OhMamClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  std::stringstream sstm;
  sstm << "Connection to SERVER Failed.";
  LogInfo(sstm);
}

/**************************************************************************************
 * Functions to Set Variables
 **************************************************************************************/
void
OhMamClient::SetServers (std::vector<Address> ip)
{
	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}

void
OhMamClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

/**************************************************************************************
 * PACKET Handlers
 **************************************************************************************/
void 
OhMamClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so 
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t 
OhMamClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
OhMamClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

/**
 * OPERATION SCHEDULER
 */
void 
OhMamClient::ScheduleOperation (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  if (m_prType == READER )
  {
	  m_sendEvent = Simulator::Schedule (dt, &OhMamClient::InvokeRead, this);
  }
  else
  {
	  m_sendEvent = Simulator::Schedule (dt, &OhMamClient::InvokeWrite, this);
  }
}

/**************************************************************************************
 * OhMam Read/Write Handlers
 **************************************************************************************/
void
OhMamClient::InvokeRead (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	// AsmCommon::Reset(sstm);
	// sstm << "** STEP! ";
	// LogInfo(sstm);

	m_opCount ++;
	m_opStart = Now();

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		//Phase 1
		m_opStatus = PHASE1;
		m_msgType = READ;
		m_readop ++;

		m_MINts = 10000000;
		m_MINId = 10000000;
		//m_value = 10000000;

		//Send msg to all
		m_replies = 0;		//reset replies
		HandleSend();

		AsmCommon::Reset(sstm);
		sstm << "** READ INVOKED: " << m_personalID << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
	}
}

void
OhMamClient::InvokeWrite (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	m_opCount ++;
	m_opStart = Now();
	

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		//Phase 1
		m_opStatus = PHASE1;
		m_msgType = DISCOVER;
		m_writeop ++;

		
		//Send msg to all
		m_replies = 0;		//reset replies
		HandleSend();

		AsmCommon::Reset(sstm);
		sstm << "** WRITE INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
	}
}

void 
OhMamClient::HandleSend (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  ++m_sent;

  // Prepare packet content
  std::stringstream pkts;
  std::string message_type;
  
  // Serialize the appropriate message for READ or WRITE
  // serialize <msgType, ts, id, value, writeop ,counter>
  // serialize <msgType, ts, id, value, readop ,counter>
  if ((m_msgType == DISCOVER) && (m_opStatus == PHASE1))
  	{
  		pkts << m_msgType << " " << m_ts << " " << m_id << " " << m_value << " " << m_writeop<<" " << m_sent;
  		message_type = "discover";
	}
  else if ((m_msgType == WRITE) && (m_opStatus == PHASE2))
  	{
  		pkts << m_msgType << " " << m_ts << " " << m_personalID << " " << m_value << " " << m_writeop<<" " << m_sent;
  		message_type = "write";
	}
  else 
    {
		pkts << m_msgType << " " << m_ts << " " << m_personalID << " " << m_value << " " << m_readop<<" " << m_sent;
		message_type = "read";
    }

  SetFill(pkts.str());

  // Create packet
  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size, "OhMamClient::HandleSend(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "OhMamClient::HandleSend(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      p = Create<Packet> (m_size);
    }


  //Send a single packet to each server
  for (int i=0; i<m_serverAddress.size(); i++)
  {
	  // call to the trace sinks before the packet is actually sent
	  m_txTrace (p);
	  m_socket[i]->Send (p);

	  std::stringstream sstm;
	  sstm << "Sent " << message_type <<" "<< p->GetSize() << " bytes to " << Ipv4Address::ConvertFrom (m_serverAddress[i])
	  	   << " port " << m_peerPort << " data " << pkts.str();
	  LogInfo ( sstm );
  }
  	p->RemoveAllPacketTags ();
	p->RemoveAllByteTags ();
}

void
OhMamClient::HandleRecv (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  uint32_t msgT, msgTs, msgId, msgV, msgOp, msgC;

  while ((packet = socket->RecvFrom (from)))
    {
	  //deserialize the contents of the packet
	  uint8_t buf[1024];
	  packet->CopyData(buf,1024);
	  std::stringbuf sb;
	  sb.str(std::string((char*) buf));
	  std::istream istm(&sb);
	  istm >> msgT >> msgTs >> msgId >> msgV >> msgOp >> msgC;
	  std::stringstream sstm;
	  std::string message_type;

	  if (msgT==READACK)
			message_type = "readAck";
	  else if (msgT==DISCOVERACK)
			message_type = "discoverAck";
	  else 
			message_type = "writeAck";
	 

	  if((msgOp==m_readop)||(msgOp==m_writeop))
	  {
	  	sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
	                        InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
	                        InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
   		LogInfo (sstm);
   		packet->RemoveAllPacketTags ();
		packet->RemoveAllByteTags ();
	  }

      // check message freshness and if client is waiting
      if ((((msgOp == m_readop) && (msgT==READACK)) || ((msgOp == m_writeop) && (msgT==WRITEACK)) || ((msgOp == m_writeop) && (msgT==DISCOVERACK))) && m_opStatus != IDLE)
       {
    	  ProcessReply(msgT, msgTs, msgId, msgV, msgOp);
       }
    
    }
}

void
OhMamClient::ProcessReply(uint32_t type, uint32_t ts, uint32_t id, uint32_t val, uint32_t op)
{
	NS_LOG_FUNCTION (this);

	std::stringstream sstm;

	//increment the number of replies received
	m_replies ++;

	if (m_prType == WRITER)
	{
		if(m_opStatus==PHASE1)
		{
			// Find the max Timestamp 
			// We do not care about the id comparison since 
			// we want to write and we will put ours. 
			//if ((m_ts < ts) || ((m_ts == ts)&&(m_id<id))) 
			if( ts >= m_ts)
			{
				m_ts = ts;
				
				// AsmCommon::Reset(sstm);
				// sstm << "Discoved Max TS [" << m_ts << "]";
				// LogInfo(sstm);
			}

			AsmCommon::Reset(sstm);
			sstm << "Waiting for " << (m_numServers-m_fail) << " discoverAck replies, received " << m_replies;
			LogInfo(sstm);

			//if we received enough replies go to the next phase
			if (m_replies >= (m_numServers - m_fail))
			{	
				//Phase 1
				m_opStatus = PHASE2;
				m_msgType = WRITE;
				// Increase my timestamp, my writeop and generate a random value
				m_ts ++; 
				m_writeop ++;
				m_value = rand()%1000;
				
				//Send msg to all
				m_replies = 0;		//reset replies
				HandleSend();
			}
		}
		else
		{	/////////////////////////////////
			//////////// PHASE 2 ////////////
			/////////////////////////////////

			if (m_replies >= (m_numServers - m_fail))
			{
				m_opStatus = IDLE;
				ScheduleOperation (m_interval);

				m_opEnd = Now();
				AsmCommon::Reset(sstm);
				sstm << "*** WRITE COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, [<tag>,value]: [<" << m_ts << "," << m_personalID << ">," << m_value << "] - @ 4 EXCH **";
				LogInfo(sstm);
				m_replies = 0;
				m_opAve += m_opEnd - m_opStart;
			}
		}
	}
	else
	{	////////////////////////////////////////////////
		//////////////////// READER ////////////////////
		////////////////////////////////////////////////

		//We have to find the min timestamp
      	if((ts<m_MINts) || ((ts==m_MINts) && (id<m_MINId)))
   		{
      		m_MINts = ts;
			m_MINId = id;
			m_MINvalue = val;
      	}
		m_replies++;

      	if (m_replies >= (m_numServers - m_fail))
      	{
      		m_opEnd = Now();
   			m_opStatus = IDLE;
			ScheduleOperation (m_interval);
      		AsmCommon::Reset(sstm);
			sstm << "*** READ COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, [<tag>,value]: [<" << m_MINts << "," << m_MINId << ">," << m_MINvalue << "] - @ 3 EXCH **";
			LogInfo(sstm);
			m_replies =0;
		}
	}
}


} // Namespace ns3
