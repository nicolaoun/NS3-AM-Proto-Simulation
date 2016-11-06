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
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "SwImp-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SwImpClientApplication");

NS_OBJECT_ENSURE_REGISTERED (SwImpClient);


void
SwImpClient::LogInfo( std::stringstream& s)
{
	NS_LOG_FUNCTION (this);

	NS_LOG_INFO("[CLIENT " << m_personalID << " - "<< Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}



TypeId
SwImpClient::GetTypeId (void)
{
  //static vector<Address> tmp_address;
  static TypeId tid = TypeId ("ns3::SwImpClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<SwImpClient> ()
    .AddAttribute ("MaxOperations",
                   "The maximum number of operations to be invoked",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SwImpClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("MaxFailures",
					  "The maximum number of server failures",
					  UintegerValue (100),
					  MakeUintegerAccessor (&SwImpClient::m_fail),
					  MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&SwImpClient::m_interval),
                   MakeTimeChecker ())
   .AddAttribute ("SetRole",
					  "The role of the client (reader/writer)",
					  UintegerValue (0),
					  MakeUintegerAccessor (&SwImpClient::m_prType),
					  MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("LocalAddress",
					  "The local Address of the current node",
					  AddressValue (),
					  MakeAddressAccessor (&SwImpClient::m_myAddress),
					  MakeAddressChecker ())
	.AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&SwImpClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SwImpClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SwImpClient::SetDataSize,
                                         &SwImpClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&SwImpClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&SwImpClient::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
	 .AddAttribute ("RandomInterval",
					 "Apply randomness on the invocation interval",
					 UintegerValue (0),
					 MakeUintegerAccessor (&SwImpClient::m_randInt),
					 MakeUintegerChecker<uint16_t> ())
	 .AddAttribute ("Seed",
					 "Seed for the pseudorandom generator",
					 UintegerValue (0),
					 MakeUintegerAccessor (&SwImpClient::m_seed),
					 MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("Verbose",
					 "Verbose for debug mode",
					 UintegerValue (0),
					 MakeUintegerAccessor (&SwImpClient::m_verbose),
					 MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("Clients", 
                     "Number of Clients",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&SwImpClient::m_numClients),
                  	 MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

/**************************************************************************************
 * Constructors
 **************************************************************************************/
SwImpClient::SwImpClient ()
{
	NS_LOG_FUNCTION (this);
	m_sent = 0;
	//m_socket = 0;
	m_sendEvent = EventId ();
	m_data = 0;
	m_dataSize = 0;
	m_serversConnected = 0;

	m_ts = 0;					//initialize the local timestamp
	m_value = 0;				//initialize local value
	m_opStatus = PHASE1; 		//initialize status

	m_fail = 0;
	m_opCount = 0;
	m_completeOps = 0;
}

SwImpClient::~SwImpClient()
{
	NS_LOG_FUNCTION (this);
	//m_socket = 0;

	delete [] m_data;
	m_data = 0;
	m_dataSize = 0;
	m_serversConnected = 0;

	m_ts = 0;					//initialize the local timestamp
	m_value = 0;				//initialize local value
	m_opStatus = PHASE1; 		//initialize status

	m_fail = 0;
	m_opCount = 0;
	m_completeOps = 0;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/
void 
SwImpClient::StartApplication (void)
{

	NS_LOG_FUNCTION (this);

	std::stringstream sstm;

	// seed pseudo-randomness
	srand(m_seed);

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
			m_socket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress[i]), m_peerPort));

			m_socket[i]->SetRecvCallback (MakeCallback (&SwImpClient::HandleRecv, this));
			m_socket[i]->SetAllowBroadcast (false);

			m_socket[i]->SetConnectCallback (
				        MakeCallback (&SwImpClient::ConnectionSucceeded, this),
				        MakeCallback (&SwImpClient::ConnectionFailed, this));
		}
	}

	AsmCommon::Reset(sstm);
	sstm << "Started Succesfully: #S=" << m_numServers <<", #F=" << m_fail << ", opInt=" << m_interval << ",debug="<<m_verbose;
	LogInfo(sstm);

}

void 
SwImpClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  std::stringstream sstm;

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
      sstm << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads="<<m_opCount<<", #CompletedReads=" << m_completeOps << ", #2EXCH_reads="<< m_completeOps <<", AveOpTime="<< (avg_time+real_avg_time) <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **";
      std::cout << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads="<<m_opCount<<", #CompletedReads=" << m_completeOps << ", #2EXCH_reads="<< m_completeOps <<", AveOpTime="<< (avg_time+real_avg_time) <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **"<<std::endl;
	  LogInfo(sstm);
	  break;
  }

  if(m_personalID==m_numClients-1){
  	exit(0);
  }
  
  Simulator::Cancel (m_sendEvent);

}

void
SwImpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void SwImpClient::ConnectionSucceeded (Ptr<Socket> socket)
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

void SwImpClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

/**************************************************************************************
 * Functions to Set Variables
 **************************************************************************************/
void
SwImpClient::SetServers (std::vector<Address> ip)
{
	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}

void
SwImpClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

/**************************************************************************************
 * PACKET Handlers
 **************************************************************************************/
void 
SwImpClient::SetDataSize (uint32_t dataSize)
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
SwImpClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
SwImpClient::SetFill (std::string fill)
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

/**
 * OPERATION SCHEDULER
 */
void 
SwImpClient::ScheduleOperation (Time dt)
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
	  m_sendEvent = Simulator::Schedule (dt, &SwImpClient::InvokeRead, this);
  }
  else
  {
	  m_sendEvent = Simulator::Schedule (dt, &SwImpClient::InvokeWrite, this);
  }
}

/**************************************************************************************
 * SwImp Read/Write Handlers
 **************************************************************************************/
void
SwImpClient::InvokeRead (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	m_opCount ++;
	m_opStart = Now();
    m_real_start = std::chrono::system_clock::now();

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		//Phase 1
		m_opStatus = PHASE1;
		m_msgType = READ;

		//Send msg to all
		m_replies = 0;		//reset replies
		AsmCommon::Reset(sstm);
		sstm << "** READ INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
		HandleSend();
	}
}

void
SwImpClient::InvokeWrite (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	m_opCount ++;
	m_opStart = Now();
    //m_real_start = std::chrono::system_clock::now();

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		//Phase 1
		m_opStatus = PHASE1;
		m_msgType = WRITE;

		//increment the ts and generate a random value
		m_ts ++;
		m_value = rand()%1000;

		//Send msg to all
		m_replies = 0;		//reset replies
		AsmCommon::Reset(sstm);
		sstm << "** WRITE INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
		HandleSend();
	}
}

void 
SwImpClient::HandleSend (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  ++m_sent;

  // Prepare packet content
  std::stringstream pkts;
  // serialize <msgType, ts, value, counter>
  pkts << m_msgType << " " << m_ts << " " << m_value << " " << m_sent;

  SetFill(pkts.str());

  // Create packet
  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size, "SwImpClient::HandleSend(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "SwImpClient::HandleSend(): m_dataSize but no m_data");
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
	  // call to the trace sinks before the packet is actually sent
	  m_txTrace (p);
      m_socket[current]->Send (p);

	  if (m_verbose)
	  {
		  std::stringstream sstm;
          sstm << "Sent " << m_size << " bytes to " << Ipv4Address::ConvertFrom (m_serverAddress[current]) << " port " << m_peerPort;
		  LogInfo ( sstm );
	  }

      // move to the next server
      current = (current+1)%m_serverAddress.size();
  }
}

void
SwImpClient::HandleRecv (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  uint32_t msgT, msgTs, msgV, msgC;

  while ((packet = socket->RecvFrom (from)))
    {
	  //deserialize the contents of the packet
	  uint8_t buf[1024];
	  packet->CopyData(buf,1024);
	  std::stringbuf sb;
	  sb.str(std::string((char*) buf));
	  std::istream istm(&sb);
	  istm >> msgT >> msgTs >> msgV >> msgC;


	  if (m_verbose)
	  {
		  std::stringstream sstm;
		  sstm << "Received " << packet->GetSize () << " bytes from " <<
				  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				  InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
		  LogInfo (sstm);
	  }

      // check message freshness and if client is waiting
      if ( msgC == m_sent && m_opStatus != IDLE)
      {
    	  ProcessReply(msgT, msgTs, msgV);
      }
    }
}

void
SwImpClient::ProcessReply(uint32_t type, uint32_t ts, uint32_t val)
{
	NS_LOG_FUNCTION (this);

	std::stringstream sstm;

	//increment the number of replies received
	m_replies ++;

	switch(m_prType)
	{
	case WRITER:
		if (m_replies >= (m_numServers - m_fail))
		{
            // computation time of the writer once it gets the replies
            m_real_start = std::chrono::system_clock::now();

			m_opStatus = IDLE;
			m_completeOps++;
			ScheduleOperation (m_interval);

			m_opEnd = Now();
			m_real_end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;
			
			AsmCommon::Reset(sstm);
			sstm << "** WRITE COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), <ts, value>: [" << m_ts << "," << m_value << "], @ 2 EXCH **";
			//std::cout << "** WRITE COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), <ts, value>: [" << m_ts << "," << m_value << "], @ 2 EXCH **"<<std::endl;
			LogInfo(sstm);

			m_real_opAve += elapsed_seconds;  //
			m_opAve += m_opEnd - m_opStart;
		}
		break;

	case READER:
		switch(m_opStatus)
		{
		case PHASE1:
			if (m_replies >= (m_numServers - m_fail))
			{
				m_opStatus = IDLE;
				m_completeOps++;
				ScheduleOperation (m_interval);
				m_opEnd = Now();
				m_real_end = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;
				

				AsmCommon::Reset(sstm);
				sstm << "** READ COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), <ts, value>: [" << m_ts << "," << m_value << "], @ 2 EXCH **";
				LogInfo(sstm);

				m_opAve += m_opEnd - m_opStart;
				m_real_opAve += elapsed_seconds;  //
			}
			break;
		default:
			break;
		}
	}
}


} // Namespace ns3
