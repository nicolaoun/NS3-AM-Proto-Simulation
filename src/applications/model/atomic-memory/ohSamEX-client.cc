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
#include "ohSamEX-client.h"
#include <string>
#include <cstdlib>
#include <iostream>
#include <ctime>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ohSamEXClientApplication");

NS_OBJECT_ENSURE_REGISTERED (ohSamEXClient);


void
ohSamEXClient::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[CLIENT " << m_personalID << " - "<< Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}



TypeId
ohSamEXClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ohSamEXClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<ohSamEXClient> ()
    .AddAttribute ("MaxOperations",
                   "The maximum number of operations to be invoked",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ohSamEXClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("MaxFailures",
					  "The maximum number of server failures",
					  UintegerValue (100),
					  MakeUintegerAccessor (&ohSamEXClient::m_fail),
					  MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&ohSamEXClient::m_interval),
                   MakeTimeChecker ())
   .AddAttribute ("SetRole",
					  "The role of the client (reader/writer)",
					  UintegerValue (0),
					  MakeUintegerAccessor (&ohSamEXClient::m_prType),
					  MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("LocalAddress",
					  "The local Address of the current node",
					  AddressValue (),
					  MakeAddressAccessor (&ohSamEXClient::m_myAddress),
					  MakeAddressChecker ())
	.AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&ohSamEXClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ohSamEXClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
					UintegerValue (9),
					MakeUintegerAccessor (&ohSamEXClient::m_port),
					MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ohSamEXClient::SetDataSize,
                                         &ohSamEXClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&ohSamEXClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&ohSamEXClient::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Clients", 
                     "Number of Clients",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&ohSamEXClient::m_numClients),
                  	 MakeUintegerChecker<uint32_t> ())
	 .AddAttribute ("RandomInterval",
					 "Apply randomness on the invocation interval",
					 UintegerValue (0),
					 MakeUintegerAccessor (&ohSamEXClient::m_randInt),
					 MakeUintegerChecker<uint16_t> ())
	 .AddAttribute ("Seed",
					 "Seed for the pseudorandom generator",
					 UintegerValue (0),
					 MakeUintegerAccessor (&ohSamEXClient::m_seed),
					 MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("Verbose",
					 "Verbose for debug mode",
					 UintegerValue (0),
					 MakeUintegerAccessor (&ohSamEXClient::m_verbose),
					 MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

/**************************************************************************************
 * Constructors
 **************************************************************************************/
ohSamEXClient::ohSamEXClient ()
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
	m_done = 0;
}

ohSamEXClient::~ohSamEXClient()
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
	m_done = 0;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/
void 
ohSamEXClient::StartApplication (void)
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
	m_insocket->SetRecvCallback (MakeCallback (&ohSamEXClient::HandleRecv, this));

	// Accept new connection
	m_insocket->SetAcceptCallback (
			MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
			MakeCallback (&ohSamEXClient::HandleAccept, this));
	// Peer socket close handles
	m_insocket->SetCloseCallbacks (
			MakeCallback (&ohSamEXClient::HandlePeerClose, this),
			MakeCallback (&ohSamEXClient::HandlePeerError, this));

	if ( m_socket.empty() )
	{
		//Set the number of sockets we need
		m_socket.resize( m_serverAddress.size() );

		for (uint32_t i = 0; i < m_serverAddress.size(); i++ )
		{
			if (m_verbose)
			{
				AsmCommon::Reset(sstm);
				sstm << "Connecting to SERVER (" << Ipv4Address::ConvertFrom(m_serverAddress[i]) << ")";
				LogInfo(sstm);
			}

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_socket[i] = Socket::CreateSocket (GetNode (), tid);

			m_socket[i]->Bind();
			//m_socket[i]->Listen ();
			m_socket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress[i]), m_peerPort));

			m_socket[i]->SetRecvCallback (MakeCallback (&ohSamEXClient::HandleRecv, this));
			m_socket[i]->SetAllowBroadcast (false);

			m_socket[i]->SetConnectCallback (
				        MakeCallback (&ohSamEXClient::ConnectionSucceeded, this),
				        MakeCallback (&ohSamEXClient::ConnectionFailed, this));
		}
	}

	
	AsmCommon::Reset(sstm);
	sstm << "Started Succesfully: #S=" << m_numServers <<", #F=" << m_fail << ", opInt=" << m_interval << ",debug="<<m_verbose;
	LogInfo(sstm);

}

void 
ohSamEXClient::StopApplication ()
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

  

  float avg_time=0;
  float real_avg_time=0;
  if(m_opCount==0){
  	avg_time = 0;
  	real_avg_time=0;
  }else{
  	avg_time = ((m_opAve.GetSeconds()) /m_opCount);
  	real_avg_time = (m_real_opAve.count()/m_opCount);
  }

  switch(m_prType)
  {
  case WRITER:
	  sstm << "** WRITER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent <<", #InvokedWrites=" << m_opCount <<", #CompletedWrites="<<m_completeOps <<", AveOpTime="<< avg_time+real_avg_time <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **";
	  std::cout << "** WRITER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent <<", #InvokedWrites=" << m_opCount <<", #CompletedWrites="<<m_completeOps <<", AveOpTime="<< avg_time+real_avg_time <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **"<<std::endl;
	  LogInfo(sstm);
	  break;
  case READER:
	  sstm << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads=" << m_opCount <<", #CompletedReads="<<m_completeOps <<", #3EXCH_reads="<< m_slowOpCount << ", #2EXCH_reads="<<m_fastOpCount<<", AveOpTime="<< avg_time+real_avg_time <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **";
	  std::cout << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads=" << m_opCount <<", #CompletedReads="<<m_completeOps <<", #3EXCH_reads="<< m_slowOpCount << ", #2EXCH_reads="<<m_fastOpCount<<", AveOpTime="<< avg_time+real_avg_time <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **"<<std::endl;
	  LogInfo(sstm);
	  break;
  }
  
  if(m_personalID==m_numClients-1){
  	exit(0);
  }

  Simulator::Cancel (m_sendEvent);
}

void
ohSamEXClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void ohSamEXClient::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void ohSamEXClient::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void ohSamEXClient::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&ohSamEXClient::HandleRecv, this));
	m_socketList.push_back (s);
}



void ohSamEXClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Address from;
  socket->GetPeerName (from);

  m_serversConnected++;

  if (m_verbose)
  {
	  std::stringstream sstm;
	  sstm << "Connected to SERVER (" << InetSocketAddress::ConvertFrom (from).GetIpv4() <<")";
	  LogInfo(sstm);
  }

  // Check if connected to the all the servers start operations
  if (m_serversConnected == m_serverAddress.size() )
  {
	  ScheduleOperation (m_interval);
  }
}

void ohSamEXClient::ConnectionFailed (Ptr<Socket> socket)
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
ohSamEXClient::SetServers (std::vector<Address> ip)
{
	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}

void
ohSamEXClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

/**************************************************************************************
 * PACKET Handlers
 **************************************************************************************/
void 
ohSamEXClient::SetDataSize (uint32_t dataSize)
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
ohSamEXClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
ohSamEXClient::SetFill (std::string fill)
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
ohSamEXClient::ScheduleOperation (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  std::stringstream sstm;

  // if rndomness is set - choose a random interval
  if ( m_randInt )
  {
	  dt = Time::From( (rand() % (m_interval.GetMilliSeconds()-1000))+1000, Time::MS );
  }

  AsmCommon::Reset(sstm);
  sstm << "** NEXT OPERATION: in " << dt.GetSeconds() <<"s";
  LogInfo(sstm);

  if (m_prType == READER )
  {
	  m_sendEvent = Simulator::Schedule (dt, &ohSamEXClient::InvokeRead, this);
  }
  else
  {
	  m_sendEvent = Simulator::Schedule (dt, &ohSamEXClient::InvokeWrite, this);
  }
}

/**************************************************************************************
 * ohSamEX Read/Write Handlers
 **************************************************************************************/
void
ohSamEXClient::InvokeRead (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;
	m_opStart = Now();
	m_real_start = std::chrono::system_clock::now();

	//check if we still have operations to perfrom
	if ( m_opCount <  m_count )
	{
		//Phase 1
		m_opStatus = PHASE1;
		m_msgType = READ;
		m_opCount++;

		m_MINts = 10000000;
		m_MINId = 10000000;
		
		//Reset fast operations vectors
		ts_timestamps.clear();
		ts_values.clear();
		ts_counters.clear();
		m_done=0;

		//Send msg to all
		m_replies = 0;		//reset replies
		AsmCommon::Reset(sstm);
		sstm << "** READ INVOKED: " << m_personalID << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
		HandleSend();
	}
}

void
ohSamEXClient::InvokeWrite (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	m_opCount ++;
	m_opStart = Now();
	m_real_start = std::chrono::system_clock::now();
	

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		m_opStatus = PHASE1;
		m_msgType = WRITE;
		m_ts ++;

		m_value = m_opCount + 900;
		m_replies = 0;

		AsmCommon::Reset(sstm);
		sstm << "** WRITE INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
		HandleSend();
	}
}

void 
ohSamEXClient::HandleSend (void)
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
  		pkts << WRITE << " " << m_ts << " " << m_value;
  		message_type = "write";
	}
  else 
    {
		pkts << READ << " "<< m_opCount;
		message_type = "read";
    }

  SetFill(pkts.str());

  // Create packet
  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size, "ohSamEXClient::HandleSend(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "ohSamEXClient::HandleSend(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      p = Create<Packet> (m_size);
    }


  //random server to start from
  int current = rand()%m_serverAddress.size();

  //Send a single packet to each server
  for (uint32_t i=0; i<m_serverAddress.size(); i++)
  {
  	  m_sent++; //count the messages sent
	  //m_txTrace (p);
      m_socket[current]->Send (p);

	  if (m_verbose)
	  {
		  std::stringstream sstm;
          sstm << "Sent " << message_type <<" "<< p->GetSize() << " bytes to " << Ipv4Address::ConvertFrom (m_serverAddress[current])
		  << " port " << m_peerPort << " data " << pkts.str();
		  LogInfo ( sstm );
	  }

      // move to the next server
      current = (current+1)%m_serverAddress.size();
  }
  	//p->RemoveAllPacketTags ();
	//p->RemoveAllByteTags ();
}

void
ohSamEXClient::HandleRecv (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  uint32_t msgT, msgTs, msgV, msgOp, msgInit;

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
	  else if (msgT==WRITEACK)
	  {
			message_type = "writeAck";
			istm >> msgTs >> msgV;
	  }
	  //EX: now client gets back also read Relays
	  else if (msgT==READRELAY)
	  {
			message_type = "readrelay";
			istm >> msgTs >> msgV >> msgInit >> msgOp;
	  }
	 

	  if (m_verbose)
	  {
		  sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
				  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				  InetSocketAddress::ConvertFrom (from).GetPort () << ", msgOp = " << msgOp <<", opCount = " << m_opCount << " data " << buf;
		  LogInfo (sstm);
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
       else if ((msgOp == m_opCount) && (msgT==READRELAY) && (m_opStatus != IDLE))
       {
       		sstm << "Received fresh Relay.";
		 	LogInfo (sstm);
		 	ProcessReply(msgT, msgTs, msgV);
		 	//For every diff timestamp i receive i need to have 
		 	//a counter. How many different timestamps i can have? s/2 floor.
		 	// 
       }
    
    }
}

void
ohSamEXClient::ProcessReply(uint32_t type, uint32_t ts, uint32_t val)
{
	NS_LOG_FUNCTION (this);

	std::stringstream sstm;

	

	if (m_prType == WRITER)
	{
		/////////////////////////////////
		//////////// WRITEACK ///////////
		/////////////////////////////////
		//increment the number of replies received
		m_replies ++;

		if (m_replies >= (m_numServers - m_fail))
		{
			m_completeOps++;
			m_opStatus = IDLE;
			ScheduleOperation (m_interval);
			m_real_end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;

			m_opEnd = Now();
			m_opAve += m_opEnd - m_opStart;
			m_real_opAve += elapsed_seconds;  //
			AsmCommon::Reset(sstm);
			sstm << "** WRITE COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), <ts, value>: [" << m_ts << "," << m_value << "], @ 2 EXCH **";
			LogInfo(sstm);
			m_replies = 0;
		}
	}
	else
	{	////////////////////////////////////////////////
		//////////////////// READER ////////////////////
		////////////////////////////////////////////////
		if(type == READRELAY && m_done==0){
			///Do nothing (for the moment)
			// We have to add the timestamp in the vector or increase the counter 
			// if it's already in!
			// Therefore, we first check if vector containts that timestamp 
			
			//This will return the index of the timestamp ts in the ts_timestamps vector 
			//so we can use it in the counter vector.
			int index = -1; 
			int return_timestamp=0;
			int return_value=0;
			index=std::find(ts_timestamps.begin(), ts_timestamps.end(), ts) - ts_timestamps.begin();
			

			// We have to check the index with the ts_timestamps size to be sure that was
			// found. Otherwise means it was not found.
			if (index < ts_timestamps.size())
			{
				// Timestamp found at Index index. Go to counter vector and increase at that index
				// position by one the counter. 
				ts_counters[index]++;
				// Check if we are done
				if (ts_counters[index] >= (m_numServers/2)+1){
					//we are done and we have to return
					m_done = 1;
					return_timestamp = ts_timestamps[index];
					return_value = ts_values[index];
				}
			}
			else
			{
				// Not Found. We have to insert the new timestamp ts to the vector.
				// And also add counter 1 to the appropriate counter vector.
				ts_timestamps.push_back(ts);
				ts_counters.push_back(1);
				ts_values.push_back(val);
			}
				
			// If we are done
			if (m_done==1)
			{
				// HERE WE WILL NEED AN IF STATEMENT TO COUNT THE SLOW 
				// ONES IN THE NEXT ALGORITHM
				// Increase Fast ops and the number of complete ops
				m_completeOps++;
				m_fastOpCount++;

      			m_opEnd = Now();
      			m_opAve += m_opEnd - m_opStart;
      			m_real_end = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;
				m_real_opAve += elapsed_seconds;  //

   				m_opStatus = IDLE;
				ScheduleOperation (m_interval);
      			AsmCommon::Reset(sstm);
				sstm << "** FAST READ COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), [<ts,value>]: [<" << return_timestamp << "," << return_value << ">] - @ 2 EXCH **";
				LogInfo(sstm);
			}
		}
		else if (type == READACK && m_done==0)
		{
			//increment the number of replies received
			m_replies ++;
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
				m_done=1;
				m_completeOps++;
				m_slowOpCount++;

      			m_opEnd = Now();
      			m_opAve += m_opEnd - m_opStart;
      			m_real_end = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;
				m_real_opAve += elapsed_seconds;  //

   				m_opStatus = IDLE;
				ScheduleOperation (m_interval);
      			AsmCommon::Reset(sstm);
				sstm << "** READ COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), [<ts,value>]: [<" << m_MINts << "," << m_MINvalue << ">] - @ 3 EXCH **";
				LogInfo(sstm);
				m_replies =0;
			}
		}
	}
}


} // Namespace ns3