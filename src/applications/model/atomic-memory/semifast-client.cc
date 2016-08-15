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
#include "semifast-client.h"
#include <unistd.h>
#include <chrono>
#include <thread>
#include <time.h>
#include <iostream>



// std::chrono::time_point<std::chrono::system_clock> start, end;
// start = std::chrono::system_clock::now();
// end = std::chrono::system_clock::now();
// std::chrono::duration<double> elapsed_seconds = end-start;
// std::cout << elapsed_seconds.count() << "s\n";


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SemifastClientApplication");

NS_OBJECT_ENSURE_REGISTERED (SemifastClient);


void
SemifastClient::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[CLIENT " << m_personalID << " - "<< Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}



TypeId
SemifastClient::GetTypeId (void)
{
  //static vector<Address> tmp_address;
  static TypeId tid = TypeId ("ns3::SemifastClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<SemifastClient> ()
    .AddAttribute ("MaxOperations",
                   "The maximum number of operations to be invoked",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SemifastClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("MaxFailures",
					  "The maximum number of server failures",
					  UintegerValue (100),
					  MakeUintegerAccessor (&SemifastClient::m_fail),
					  MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&SemifastClient::m_interval),
                   MakeTimeChecker ())
   .AddAttribute ("SetRole",
					  "The role of the client (reader/writer)",
					  UintegerValue (0),
					  MakeUintegerAccessor (&SemifastClient::m_prType),
					  MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("LocalAddress",
					  "The local Address of the current node",
					  AddressValue (),
					  MakeAddressAccessor (&SemifastClient::m_myAddress),
					  MakeAddressChecker ())
	.AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&SemifastClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SemifastClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SemifastClient::SetDataSize,
                                         &SemifastClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&SemifastClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&SemifastClient::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
	 .AddAttribute ("RandomInterval",
					 "Apply randomness on the invocation interval",
					 UintegerValue (0),
					 MakeUintegerAccessor (&SemifastClient::m_randInt),
					 MakeUintegerChecker<uint16_t> ())
	 .AddAttribute ("Seed",
					 "Seed for the pseudorandom generator",
					 UintegerValue (0),
					 MakeUintegerAccessor (&SemifastClient::m_seed),
					 MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("Verbose",
					 "Verbose for debug mode",
					 UintegerValue (0),
					 MakeUintegerAccessor (&SemifastClient::m_verbose),
					 MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("Clients", 
                     "Number of Clients",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&SemifastClient::m_numClients),
                  	 MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

/**************************************************************************************
 * Constructors
 **************************************************************************************/
SemifastClient::SemifastClient ()
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
	m_pvalue = 0;
	m_opCount=0;
	m_opStatus = PHASE1; 		//initialize status

	m_fail = 0;

	m_twoExOps = 0;
	m_fourExOps = 0;
}

SemifastClient::~SemifastClient()
{
	NS_LOG_FUNCTION (this);
	//m_socket = 0;

	delete [] m_data;
	m_data = 0;
	m_dataSize = 0;
	m_serversConnected = 0;

	m_ts = 0;					//initialize the local timestamp
	m_value = 0;				//initialize local value
	m_pvalue = 0;
	m_opCount=0;
	m_opStatus = PHASE1; 		//initialize status

	m_fail = 0;

	m_twoExOps = 0;
	m_fourExOps = 0;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/
void 
SemifastClient::StartApplication (void)
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
			AsmCommon::Reset(sstm);
			sstm << "Connecting to SERVER (" << Ipv4Address::ConvertFrom(m_serverAddress[i]) << ")";
			LogInfo(sstm);

			TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
			m_socket[i] = Socket::CreateSocket (GetNode (), tid);

			m_socket[i]->Bind();
			m_socket[i]->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_serverAddress[i]), m_peerPort));

			m_socket[i]->SetRecvCallback (MakeCallback (&SemifastClient::HandleRecv, this));
			m_socket[i]->SetAllowBroadcast (false);

			m_socket[i]->SetConnectCallback (
				        MakeCallback (&SemifastClient::ConnectionSucceeded, this),
				        MakeCallback (&SemifastClient::ConnectionFailed, this));
		}
	}

	AsmCommon::Reset(sstm);
	sstm << "Started Succesfully: #S=" << m_numServers <<", #F=" << m_fail << " #vNodes= "<< m_numVNodes <<", Vid=" << m_virtualId << ", opInt=" << m_interval<<", debug="<<m_verbose;
	LogInfo(sstm);

}

void 
SemifastClient::StopApplication ()
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
	  //sstm << "** WRITER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent <<", #InvokedWrites=" << m_opCount <<", #CompletedWrites="<<m_twoExOps+m_fourExOps <<", AveOpTime="<< avg_time <<"s **";
	  sstm << "** WRITER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent <<", #InvokedWrites=" << m_opCount <<", #CompletedWrites="<<m_twoExOps+m_fourExOps <<", AveOpTime="<< avg_time+real_avg_time <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **";
	  std::cout << "** WRITER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent <<", #InvokedWrites=" << m_opCount <<", #CompletedWrites="<<m_twoExOps+m_fourExOps <<", AveOpTime="<< avg_time+real_avg_time <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **"<<std::endl;
	  LogInfo(sstm);
	  break;
  case READER:
	  //sstm << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads=" << m_opCount <<", #CompletedReads="<<m_twoExOps+m_fourExOps <<", #4EXCH_reads="<< m_fourExOps << ", #2EXCH_reads="<<m_twoExOps<<", AveOpTime="<< avg_time <<"s **";
	  sstm << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads=" << m_opCount <<", #CompletedReads="<<m_twoExOps+m_fourExOps <<", #4EXCH_reads="<< m_fourExOps << ", #2EXCH_reads="<<m_twoExOps<<", AveOpTime="<< avg_time+real_avg_time <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **";
	  std::cout << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads=" << m_opCount <<", #CompletedReads="<<m_twoExOps+m_fourExOps <<", #4EXCH_reads="<< m_fourExOps << ", #2EXCH_reads="<<m_twoExOps<<", AveOpTime="<< avg_time+real_avg_time <<"s, AveCommTime="<<avg_time<<"s, AvgCompTime="<<real_avg_time<<"s **"<<std::endl;
	  LogInfo(sstm);
	  break;
  }

 if(m_personalID==m_numClients-1){
  	exit(0);
 }	

  Simulator::Cancel (m_sendEvent);

  // switch(m_prType)
  // {
  // case WRITER:
	 //  sstm << "** WRITER TERMINATED: #writes=" << m_opCount << ", #2ExOps=" << m_twoExOps << ", #4ExOps=" << m_fourExOps << ", AveOpTime="<< ( (m_opAve.GetSeconds()) /m_opCount) <<"s **";
	 //  LogInfo(sstm);
	 //  break;
  // case READER:
	 //  sstm << "** READER TERMINATED: #reads=" << m_opCount << ", #2ExOps=" << m_twoExOps << ", #4ExOps=" << m_fourExOps << ", AveOpTime="<< ((m_opAve.GetSeconds())/m_opCount) <<"s **";
	 //  LogInfo(sstm);
	 //  break;
  // }
}

void
SemifastClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void SemifastClient::ConnectionSucceeded (Ptr<Socket> socket)
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

void SemifastClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

/**************************************************************************************
 * Functions to Set Variables
 **************************************************************************************/
void
SemifastClient::SetServers (std::vector<Address> ip)
{
	int bound;

	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	bound = (m_numServers/m_fail) - 3;

	// compute number of virtual nodes
	m_numVNodes = (bound > 1)? bound : 1;

	//compute client's vID
	if ( m_prType == WRITER )
	{
		m_virtualId = 0;
	}
	else
	{
		m_virtualId = ((m_personalID-1) % m_numVNodes) + 1;
	}

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}

void
SemifastClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

/**************************************************************************************
 * PACKET Handlers
 **************************************************************************************/
void 
SemifastClient::SetDataSize (uint32_t dataSize)
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
SemifastClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
SemifastClient::SetFill (std::string fill)
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
SemifastClient::ScheduleOperation (Time dt)
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
	  m_sendEvent = Simulator::Schedule (dt, &SemifastClient::InvokeRead, this);
  }
  else
  {
	  m_sendEvent = Simulator::Schedule (dt, &SemifastClient::InvokeWrite, this);
  }
}

/**************************************************************************************
 * Semifast Read/Write Handlers
 **************************************************************************************/
void
SemifastClient::InvokeRead (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	m_opCount ++;
	m_opStart = Now();
	//REAL TIME
	
	
	m_real_start = std::chrono::system_clock::now();


	AsmCommon::Reset(sstm);
	sstm << "** READ INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
	LogInfo(sstm);

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		//Phase 1
		m_opStatus = PHASE1;
		m_msgType = READ;

		//Send msg to all
		m_replies = 0;		//reset replies
		m_maxAckSet.clear();
		m_propSet.clear();
		HandleSend();
	}
}

void
SemifastClient::InvokeWrite (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	m_opCount ++;
	m_opStart = Now();

	m_real_start = std::chrono::system_clock::now();

	AsmCommon::Reset(sstm);
	sstm << "** WRITE INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
	LogInfo(sstm);

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		//Phase 1
		m_opStatus = PHASE1;
		m_msgType = WRITE;

		//increment the ts and generate a random value
		m_ts ++;
		m_pvalue = m_value;
		m_value = 900+m_opCount;

		//Send msg to all
		m_replies = 0;		//reset replies
		HandleSend();
	}
}

void 
SemifastClient::HandleSend (void)
{
  NS_LOG_FUNCTION (this);

  //NS_ASSERT (m_sendEvent.IsExpired ());

  // Prepare packet content
  std::stringstream pkts;
  // serialize <counter, msgType, ts, value, pvalue, vid>
  pkts << m_opCount << " " << m_msgType << " " << m_ts << " " << m_value << " " << m_pvalue << " " << m_virtualId;

  SetFill(pkts.str());

  // Create packet
  Ptr<Packet> p;
  if (m_dataSize)
    {
      NS_ASSERT_MSG (m_dataSize == m_size, "SemifastClient::HandleSend(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "SemifastClient::HandleSend(): m_dataSize but no m_data");
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
	  ++m_sent;

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
SemifastClient::HandleRecv (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  //uint32_t msgT, msgTs, msgV, msgVp, msgC, msgViews;
  uint32_t msgC;

  while ((packet = socket->RecvFrom (from)))
  {
	  //deserialize the contents of the packet
	  uint8_t buf[1024];
	  packet->CopyData(buf,1024);
	  std::stringbuf sb;
	  sb.str(std::string((char*) buf));
	  std::istream istm(&sb);
	  istm >> msgC;

	  if (m_verbose)
	  {
		  std::stringstream sstm;
		  sstm << "Received " << packet->GetSize () << " bytes from " <<
				  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
				  InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
		  LogInfo (sstm);
	  }

	  // check message freshness and if client is waiting
	  if ( msgC == m_opCount && m_opStatus != IDLE)
	  {
		  ProcessReply(istm, from);
	  }
  }
}

void
SemifastClient::ProcessReply(std::istream& istm, Address sender)
{
	NS_LOG_FUNCTION (this);

	std::stringstream sstm;
	uint32_t msgT, msgTs, msgV, msgVp, msgPs, msgViews, vId;
	std::set<uint32_t> msgSeen;


	//increment the number of replies received
	m_replies ++;

	// deserialize the message <T, <ts,v,vp>, ps, views>
	istm >> msgT >> msgTs >> msgV >> msgVp >> msgPs >> msgViews;

	switch(m_prType)
	{
	case WRITER:
		if (m_replies >= (m_numServers - m_fail))
		{
			m_opStatus = IDLE;
			ScheduleOperation (m_interval);

			m_real_end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;

			m_opEnd = Now();
			// AsmCommon::Reset(sstm);
			// sstm << "** WRITE COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, <ts, value>: [" << m_ts << "," << m_value << "], @ 2 EXCH **";
			// LogInfo(sstm);

			AsmCommon::Reset(sstm);
			sstm << "** WRITE COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), <ts, value>: [" << m_ts << "," << m_value << "], @ 2 EXCH **";
			LogInfo(sstm);			

			m_opAve += m_opEnd - m_opStart;
			m_real_opAve += elapsed_seconds;  //

			m_twoExOps++;
		}
		break;

	case READER:
		switch(m_opStatus)
		{
		case PHASE1:
			
			// deserialize the seen set of the message
			for (uint32_t i=0; i<msgViews; i++)
			{
				istm >> vId;
				msgSeen.insert(vId);
			}

			//if new max ts discovered - update the local <ts, value, pvalue>
			if(m_ts < msgTs)
			{
				m_ts = msgTs;
				m_value = msgV;
				m_pvalue = msgVp;

				if (m_verbose)
				{
					AsmCommon::Reset(sstm);
					sstm << "Updated local <ts,value> pair to: [" << m_ts << "," << m_value << "," << m_pvalue <<"]";
					LogInfo(sstm);
				}

				//reset the maxAck set and maxViews variables
				m_maxAckSet.clear();
				m_propSet.clear();
			}

			// enclosed ts == maxTs - include the msg in the maxAck set and update maxViews variable
			if ( m_ts == msgTs )
			{
				// add the sender and the
				m_maxAckSet.insert(std::make_pair(sender, msgSeen));

				//max views
				m_maxViews = (m_maxViews < msgViews) ? msgViews : m_maxViews;

				//check if the postscript of the server is equal to maxTS and save him
				if(m_ts == msgPs)
				{
					m_propSet.push_back(sender);
				}

			}

			//if we received enough replies go to the next phase
			if (m_replies >= (m_numServers - m_fail))
			{
				if (m_verbose)
				{
					AsmCommon::Reset(sstm);
					sstm << "Waiting for " << (m_numServers-m_fail) << " replies, received " << m_replies;
					LogInfo(sstm);
				}

					int predRes;
					predRes = IsPredicateValid ();
					

					//check the predicate to return in one round
					if ( predRes == 1 || m_propSet.size() >= m_fail+1 )
					{
						m_opStatus = IDLE;
						m_opEnd = Now();

						m_real_end = std::chrono::system_clock::now();									///
						std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;		///

						AsmCommon::Reset(sstm);
						sstm << "** READ COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), Return Value: "<< m_value <<
								", <ts, value, pvalue>: ["<< m_ts << "," << m_value << ","<< m_pvalue <<"] - @ 2 EXCH **";
						LogInfo(sstm);

						//AsmCommon::Reset(sstm);
						//sstm << "** READ COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, Return Value: "<< m_value <<
						//		", <ts, value, pvalue>: ["<< m_ts << "," << m_value << ","<< m_pvalue <<"] - @ 2 EXCH **";
						//LogInfo(sstm);

						m_opAve += m_opEnd - m_opStart;
						m_real_opAve += elapsed_seconds;  //
						ScheduleOperation (m_interval);
						//increase four exchange counter
						m_twoExOps++;
					}
                    // if predicate is valid with |seen| = a or maxPs=maxTs but < f+1 servers have it - Go to Phase2
					else if (predRes == 2 || m_propSet.size() > 0)
					{
						AsmCommon::Reset(sstm);
						sstm << "Proceeding to PHASE2, maxViews " << m_maxViews << ", bound "<< ((m_numServers/m_fail) - 2);
						LogInfo(sstm);

						//Phase 2
						m_opStatus = PHASE2;
						m_msgType = INFORM;

						//Send msg to all
						m_replies = 0;		//reset replies
						m_maxAckSet.clear();
						m_propSet.clear();
						HandleSend();
					}
					else
					{
						//else return mxTs-1
                        m_opStatus = IDLE;
						m_opEnd = Now();

						m_real_end = std::chrono::system_clock::now();								///
						std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;	///

						
						AsmCommon::Reset(sstm);
						sstm << "** READ COMPLETED: "  << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), Return PValue: "<< m_pvalue <<
								", <ts, value, pvalue>: ["<< m_ts << "," << m_value << ","<< m_pvalue <<"] - @ 2 EXCH **";
						LogInfo(sstm);

						// AsmCommon::Reset(sstm);
						// sstm << "** READ COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, Return PValue: "<< m_pvalue <<
						// 		", <ts, value, pvalue>: ["<< m_ts << "," << m_value << ","<< m_pvalue <<"] - @ 2 EXCH **";
						// LogInfo(sstm);

						m_opAve += m_opEnd - m_opStart;
						m_real_opAve += elapsed_seconds;  //
						ScheduleOperation (m_interval);
						//increase four exchange counter
						m_twoExOps++;
					}
				}
			break;
		case PHASE2:
			if (m_replies >= (m_numServers - m_fail))
			{
				m_opStatus = IDLE;

				m_opEnd = Now();

				m_real_end = std::chrono::system_clock::now();								///
				std::chrono::duration<double> elapsed_seconds = m_real_end-m_real_start;	///

				AsmCommon::Reset(sstm);
				//sstm << "** READ COMPLETED: " << m_opCount << " in COMM "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) << "s, and in COMPUTATION "<< elapsed_seconds.count() <<"s, <ts, value, pvalue>: [" << m_ts << "," << m_value << "," << m_pvalue << "] - @ 4 EXCH **";
				sstm << "** READ COMPLETED: " << m_opCount << " in "<< ((m_opEnd.GetSeconds() - m_opStart.GetSeconds()) + elapsed_seconds.count()) << "s (<"<<(m_opEnd.GetSeconds() - m_opStart.GetSeconds())<<"> + <"<< elapsed_seconds.count() <<">), <ts, value, pvalue>: [" << m_ts << "," << m_value << "," << m_pvalue << "] - @ 4 EXCH **";
				LogInfo(sstm);

				// AsmCommon::Reset(sstm);
				// sstm << "** READ COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, <ts, value, pvalue>: [" << m_ts << "," << m_value << "," << m_pvalue << "] - @ 4 EXCH **";
				// LogInfo(sstm);
				
				m_opAve += m_opEnd - m_opStart;
				m_real_opAve += elapsed_seconds;  //
				ScheduleOperation (m_interval);
				//increase four exchange counter
				m_fourExOps++;
			}
			break;
		default:
			break;
		}
	}
}

/**
 * \brief Checks if the predicate condition is valid
 *Inputs : None
 *Outputs: 0:Condition is NOT valid
 *         1:Condition is valid and |m.seen|>alpha
 *         2:Condition is valid but |m.seen|=alpha
 */
int
SemifastClient::IsPredicateValid()
{
	NS_LOG_FUNCTION (this);

	int msSize, combinations;
	uint32_t a;
	std::stringstream sstm;
	//int f1,f2,f3;
	std::set< std::pair< Address, std::set<uint32_t> > > maxSet;
	std::set< std::pair< Address, std::set<uint32_t> > >::iterator it;
	std::set<uint32_t> intersection;

	//f1 = SetOperation<int>::Factorial(m_maxAckSet.size());

	for(a = 1; a < m_numVNodes+1; a++)
	{
		msSize = m_numServers - a*m_fail;

		if (m_verbose)
		{
			AsmCommon::Reset(sstm);
			sstm << "PREDICATE LOOP: a=" << a << ", msSize="<< msSize << " maxAckSize="<< m_maxAckSet.size() <<", bound=" << (m_numServers - a*m_fail);
			LogInfo(sstm);
		}

		// Choose(maxAck.size(), msSize) possible different sets MS
		//f2 = SetOperation<int>::Factorial(msSize);
		//f3 = SetOperation<int>::Factorial(m_maxAckSet.size()-msSize);
		//combinations = f1/(f2*f3);

		combinations = SetOperation<int>::Choose(m_maxAckSet.size(), msSize);

		// check each maxSet of size msSize
		for(int i=0; i<combinations; i++)
		{
			maxSet = SetOperation< std::pair< Address, std::set<uint32_t> > >::IthSubsetSizeK(m_maxAckSet, msSize, i);
			intersection.clear();

			if (m_verbose)
			{
				AsmCommon::Reset(sstm);
				sstm << "MAX SET:{ ";
				for ( it = maxSet.begin(); it != maxSet.end(); it++)
				{
					sstm << InetSocketAddress::ConvertFrom((*it).first).GetIpv4() << " ";
				}
				sstm << "}";
				LogInfo(sstm);
			}


			// compute the intersection of the seen sets in maxSet
			for (it = maxSet.begin(); it != maxSet.end(); it++)
			{
				if(intersection.empty())
				{
					intersection = (*it).second;
				}
				else
				{
					intersection = SetOperation<uint32_t>::intersect(intersection, (*it).second);
				}

				if (m_verbose)
				{
					AsmCommon::Reset(sstm);
					sstm << "INTERSECTION LOOP: " << SetOperation<uint32_t>::printSet(intersection, "Intersection", "vid_");
					LogInfo(sstm);
				}
			}

			AsmCommon::Reset(sstm);
			sstm << "INTERSECTION: " << SetOperation<uint32_t>::printSet(intersection, "", "vid_");
			LogInfo(sstm);

			//check the intersection size
			if(intersection.size() > a)
			{
				return 1;
			}
			else if (intersection.size() == a)
			{
				return 2;
			}
		}
	}

	return 0;
}


} // Namespace ns3
