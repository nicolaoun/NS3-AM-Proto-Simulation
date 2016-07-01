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
#include "ohSam-client.h"
#include <string>
#include <cstdlib>
#include <iostream>
#include <ctime>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ohSamClientApplication");

NS_OBJECT_ENSURE_REGISTERED (ohSamClient);


void
ohSamClient::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[CLIENT " << m_personalID << " - "<< Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}



TypeId
ohSamClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ohSamClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<ohSamClient> ()
    .AddAttribute ("MaxOperations",
                   "The maximum number of operations to be invoked",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ohSamClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("MaxFailures",
					  "The maximum number of server failures",
					  UintegerValue (100),
					  MakeUintegerAccessor (&ohSamClient::m_fail),
					  MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&ohSamClient::m_interval),
                   MakeTimeChecker ())
   .AddAttribute ("SetRole",
					  "The role of the client (reader/writer)",
					  UintegerValue (0),
					  MakeUintegerAccessor (&ohSamClient::m_prType),
					  MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("LocalAddress",
					  "The local Address of the current node",
					  AddressValue (),
					  MakeAddressAccessor (&ohSamClient::m_myAddress),
					  MakeAddressChecker ())
	.AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&ohSamClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ohSamClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
					UintegerValue (9),
					MakeUintegerAccessor (&ohSamClient::m_port),
					MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ohSamClient::SetDataSize,
                                         &ohSamClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&ohSamClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&ohSamClient::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
	 .AddAttribute ("RandomInterval",
					 "Apply randomness on the invocation interval",
					 UintegerValue (0),
					 MakeUintegerAccessor (&ohSamClient::m_randInt),
					 MakeUintegerChecker<uint16_t> ())
	 .AddAttribute ("Seed",
					 "Seed for the pseudorandom generator",
					 UintegerValue (0),
					 MakeUintegerAccessor (&ohSamClient::m_seed),
					 MakeUintegerChecker<uint16_t> ())
					 ;
  ;
  return tid;
}

/**************************************************************************************
 * Constructors
 **************************************************************************************/
ohSamClient::ohSamClient ()
{
	NS_LOG_FUNCTION (this);
	m_sent = 0;
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
	m_opCount=0;
	m_slowOpCount=0;
	m_completeOps=0;
	m_fastOpCount=0;
}

ohSamClient::~ohSamClient()
{
	NS_LOG_FUNCTION (this);
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
	m_opCount=0;
	m_slowOpCount=0;
	m_completeOps=0;
	m_fastOpCount=0;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/
void 
ohSamClient::StartApplication (void)
{

	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	// seed pseudo-randomness
	srand(m_seed);

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
	m_insocket->SetRecvCallback (MakeCallback (&ohSamClient::HandleRecv, this));

	// Accept new connection
	m_insocket->SetAcceptCallback (
			MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
			MakeCallback (&ohSamClient::HandleAccept, this));
	// Peer socket close handles
	m_insocket->SetCloseCallbacks (
			MakeCallback (&ohSamClient::HandlePeerClose, this),
			MakeCallback (&ohSamClient::HandlePeerError, this));

	if ( m_socket.empty() )
	{
		//Set the number of sockets we need
		m_socket.resize( m_serverAddress.size() );

		for (uint32_t i = 0; i < m_serverAddress.size(); i++ )
		{
			AsmCommon::Reset(sstm);
			sstm << "Connecting to SERVER (" << Ipv4Address::ConvertFrom(m_serverAddress[i]) << ")";
			LogInfo(sstm);

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_socket[i] = Socket::CreateSocket (GetNode (), tid);

			m_socket[i]->Bind();
			//m_socket[i]->Listen ();
			m_socket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress[i]), m_peerPort));

			m_socket[i]->SetRecvCallback (MakeCallback (&ohSamClient::HandleRecv, this));
			m_socket[i]->SetAllowBroadcast (false);

			m_socket[i]->SetConnectCallback (
				        MakeCallback (&ohSamClient::ConnectionSucceeded, this),
				        MakeCallback (&ohSamClient::ConnectionFailed, this));
		}
	}

	
	AsmCommon::Reset(sstm);
	sstm << "Started Succesfully: #S=" << m_numServers <<", #F=" << m_fail << ", opInt=" << m_interval;
	LogInfo(sstm);

}

void 
ohSamClient::StopApplication ()
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
	  for(uint32_t i=0; i< m_socket.size(); i++ )
	  {
		  m_socket[i]->Close ();
		  m_socket[i]->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	  }
    }

  Simulator::Cancel (m_sendEvent);

  float avg_time=0;
  if(m_opCount==0)
  	avg_time = 0;
  else
  	avg_time = ((m_opAve.GetSeconds()) /m_opCount);

  switch(m_prType)
  {
  case WRITER:
	  sstm << "** WRITER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent <<", #InvokedWrites=" << m_opCount <<", #CompletedWrites="<<m_completeOps <<", AveOpTime="<< avg_time <<"s **";
	  LogInfo(sstm);
	  break;
  case READER:
	  sstm << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads=" << m_opCount <<", #CompletedReads="<<m_completeOps <<", #3EXCH_reads="<< m_completeOps << ", #2EXCH_reads=0, AveOpTime="<< avg_time <<"s **";
	  LogInfo(sstm);
	  break;
  }
}

void
ohSamClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void ohSamClient::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void ohSamClient::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void ohSamClient::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&ohSamClient::HandleRecv, this));
	m_socketList.push_back (s);
}



void ohSamClient::ConnectionSucceeded (Ptr<Socket> socket)
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

void ohSamClient::ConnectionFailed (Ptr<Socket> socket)
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
ohSamClient::SetServers (std::vector<Address> ip)
{
	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}

void
ohSamClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

/**************************************************************************************
 * PACKET Handlers
 **************************************************************************************/
void 
ohSamClient::SetDataSize (uint32_t dataSize)
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
ohSamClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
ohSamClient::SetFill (std::string fill)
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

/**
 * OPERATION SCHEDULER
 */
void 
ohSamClient::ScheduleOperation (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  std::stringstream sstm;

  // if rndomness is set - choose a random interval
  if ( m_randInt )
  {
	  dt = Time::From( ((int) rand() % (int) dt.GetSeconds())+1 );
  }

  AsmCommon::Reset(sstm);
  sstm << "** NEXT OPERATION: in " << dt.GetSeconds() <<"s";
  LogInfo(sstm);

  if (m_prType == READER )
  {
	  m_sendEvent = Simulator::Schedule (dt, &ohSamClient::InvokeRead, this);
  }
  else
  {
	  m_sendEvent = Simulator::Schedule (dt, &ohSamClient::InvokeWrite, this);
  }
}

/**************************************************************************************
 * ohSam Read/Write Handlers
 **************************************************************************************/
void
ohSamClient::InvokeRead (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;
	m_opStart = Now();

	//check if we still have operations to perfrom
	if ( m_opCount <  m_count )
	{
		//Phase 1
		m_opStatus = PHASE1;
		m_msgType = READ;
		m_opCount++;

		m_MINts = 10000000;
		m_MINId = 10000000;
		
		//Send msg to all
		m_replies = 0;		//reset replies
		AsmCommon::Reset(sstm);
		sstm << "** READ INVOKED: " << m_personalID << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
		HandleSend();
	}
}

void
ohSamClient::InvokeWrite (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	m_opCount ++;
	m_opStart = Now();
	

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		m_opStatus = PHASE1;
		m_msgType = WRITE;
		m_ts ++;
		m_opCount++;

		m_value = m_opCount + 900;
		m_replies = 0;

		AsmCommon::Reset(sstm);
		sstm << "** WRITE INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
		HandleSend();
	}
}

void 
ohSamClient::HandleSend (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  
  // Prepare packet content
  std::stringstream pkts;
  std::string message_type;
  
  AsmCommon::Reset(pkts);
  // Serialize the appropriate message for READ or WRITE
  if ( m_msgType == WRITE )
  	{
  		pkts << m_msgType << " " << m_ts << " " << m_value;
  		message_type = "write";
	}
  else 
    {
		pkts << m_msgType <<" "<< m_opCount;
		message_type = "read";
    }

  SetFill(pkts.str());

  // Create packet
  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size, "ohSamClient::HandleSend(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "ohSamClient::HandleSend(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      p = Create<Packet> (m_size);
    }


  //Send a single packet to each server
  for (uint32_t i=0; i<m_serverAddress.size(); i++)
  {
  	  m_sent++; //count the messages sent
	  //m_txTrace (p);
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
ohSamClient::HandleRecv (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  uint32_t msgT, msgTs, msgV, msgOp;

  while ((packet = socket->RecvFrom (from)))
    {
	  //deserialize the contents of the packet
	  uint8_t buf[1024];
	  packet->CopyData(buf,1024);
	  std::stringbuf sb;
	  sb.str(std::string((char*) buf));
	  std::istream istm(&sb);
	  istm >> msgT;
	  std::stringstream sstm;
	  std::string message_type;

	  if (msgT==READACK)
	  {
			message_type = "readAck";
			istm >> msgTs >> msgV >> msgOp;
	  }
	  else
	  {
			message_type = "writeAck";
			istm >> msgTs >> msgV;
	  }
	 

	  if((msgOp==m_opCount)||(msgTs==m_ts))
	  {
	  	sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
	                        InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
	                        InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
   		LogInfo (sstm);
   		packet->RemoveAllPacketTags ();
		packet->RemoveAllByteTags ();
	  }

      // check message freshness and if client is waiting
      if ((msgOp == m_opCount) && (msgT==READACK) && (m_opStatus != IDLE))
       {
    		ProcessReply(msgT, msgTs, msgV);
       }
       else if ((msgTs == m_ts) && (msgT==WRITEACK) && (m_opStatus != IDLE))
       {
       		ProcessReply(msgT, msgTs, msgV);
       }
       else
       {
       	//Do nothing
       }
    
    }
}

void
ohSamClient::ProcessReply(uint32_t type, uint32_t ts, uint32_t val)
{
	NS_LOG_FUNCTION (this);

	std::stringstream sstm;

	//increment the number of replies received
	m_replies ++;

	if (m_prType == WRITER)
	{
		/////////////////////////////////
		//////////// WRITEACK ///////////
		/////////////////////////////////

		if (m_replies >= (m_numServers - m_fail))
		{
			m_completeOps++;
			m_opStatus = IDLE;
			ScheduleOperation (m_interval);

			m_opEnd = Now();
			m_opAve += m_opEnd - m_opStart;
			AsmCommon::Reset(sstm);
			sstm << "*** WRITE COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, [<ts,value>]: [<" << m_ts << "," << m_value << ">] - @ 2 EXCH **";
			LogInfo(sstm);
			m_replies = 0;
		}
	}
	else
	{	////////////////////////////////////////////////
		//////////////////// READER ////////////////////
		////////////////////////////////////////////////

		//We have to find the min timestamp
      	if(ts<m_MINts)
   		{
      		m_MINts = ts;
			m_MINvalue = val;
      	}
		
      	if (m_replies >= (m_numServers - m_fail))
      	{
      		// HERE WE WILL NEED AN IF STATEMENT TO COUNT THE SLOW 
			// ONES IN THE NEXT ALGORITHM
			m_completeOps++;
			m_slowOpCount++;

      		m_opEnd = Now();
      		m_opAve += m_opEnd - m_opStart;
   			m_opStatus = IDLE;
			ScheduleOperation (m_interval);
      		AsmCommon::Reset(sstm);
			sstm << "*** READ COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, [<ts,value>]: [<" << m_MINts << "," << m_MINvalue << ">] - @ 3 EXCH **";
			LogInfo(sstm);
			m_replies =0;
		}
	}
}


} // Namespace ns3
