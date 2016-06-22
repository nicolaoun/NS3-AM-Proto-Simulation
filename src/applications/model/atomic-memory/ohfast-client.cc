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
#include "ohfast-client.h"
#include <string>
#include <cstdlib>
#include <iostream>
#include <ctime>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OhFastClientApplication");

NS_OBJECT_ENSURE_REGISTERED (OhFastClient);


void
OhFastClient::LogInfo( std::stringstream& s)
{
	NS_LOG_INFO("[CLIENT " << m_personalID << " - "<< Ipv4Address::ConvertFrom(m_myAddress) << "] (" << Simulator::Now ().GetSeconds () << "s):" << s.str());
}



TypeId
OhFastClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OhFastClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<OhFastClient> ()
    .AddAttribute ("MaxOperations",
                   "The maximum number of operations to be invoked",
                   UintegerValue (100),
                   MakeUintegerAccessor (&OhFastClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("MaxFailures",
					  "The maximum number of server failures",
					  UintegerValue (100),
					  MakeUintegerAccessor (&OhFastClient::m_fail),
					  MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&OhFastClient::m_interval),
                   MakeTimeChecker ())
   .AddAttribute ("SetRole",
					  "The role of the client (reader/writer)",
					  UintegerValue (0),
					  MakeUintegerAccessor (&OhFastClient::m_prType),
					  MakeUintegerChecker<uint16_t> ())
   .AddAttribute ("LocalAddress",
					  "The local Address of the current node",
					  AddressValue (),
					  MakeAddressAccessor (&OhFastClient::m_myAddress),
					  MakeAddressChecker ())
	.AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&OhFastClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", 
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&OhFastClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
					UintegerValue (9),
					MakeUintegerAccessor (&OhFastClient::m_port),
					MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&OhFastClient::SetDataSize,
                                         &OhFastClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&OhFastClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("ID", 
                     "Client ID",
                   	 UintegerValue (100),
                  	 MakeUintegerAccessor (&OhFastClient::m_personalID),
                  	 MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

/**************************************************************************************
 * Constructors
 **************************************************************************************/
OhFastClient::OhFastClient ()
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
	m_fail = 0;
	m_opCount=0;
	m_slowOpCount=0;
	m_fastOpCount=0;
}

OhFastClient::~OhFastClient()
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
	m_fail = 0;
	m_opCount=0;
	m_slowOpCount=0;
	m_fastOpCount=0;
}

/**************************************************************************************
 * APPLICATION START/STOP FUNCTIONS
 **************************************************************************************/
void 
OhFastClient::StartApplication (void)
{

	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

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
	m_insocket->SetRecvCallback (MakeCallback (&OhFastClient::HandleRecv, this));

	// Accept new connection
	m_insocket->SetAcceptCallback (
			MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
			MakeCallback (&OhFastClient::HandleAccept, this));
	// Peer socket close handles
	m_insocket->SetCloseCallbacks (
			MakeCallback (&OhFastClient::HandlePeerClose, this),
			MakeCallback (&OhFastClient::HandlePeerError, this));

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

			m_socket[i]->SetRecvCallback (MakeCallback (&OhFastClient::HandleRecv, this));
			m_socket[i]->SetAllowBroadcast (false);

			m_socket[i]->SetConnectCallback (
				        MakeCallback (&OhFastClient::ConnectionSucceeded, this),
				        MakeCallback (&OhFastClient::ConnectionFailed, this));
		}
	}

	
	AsmCommon::Reset(sstm);
	sstm << "Started Succesfully: #S=" << m_numServers <<", #F=" << m_fail << ", opInt=" << m_interval;
	LogInfo(sstm);

}

void 
OhFastClient::StopApplication ()
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

  // switch(m_prType)
  // {
  // case WRITER:
	 //  sstm << "** WRITER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent <<", #writes=" << m_opCount << ", AveOpTime="<< ( (m_opAve.GetSeconds()) /m_opCount) <<"s **";
	 //  LogInfo(sstm);
	 //  break;
  // case READER:
	 //  sstm << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #reads=" << m_opCount << ", #3EXCH_reads="<< m_slowOpCount << ", #2EXCH_reads="<<m_fastOpCount<<", AveOpTime="<< ((m_opAve.GetSeconds())/m_opCount) <<"s, 3EXC_AveOpTime="<< ((m_opAve.GetSeconds())/m_slowOpCount) <<"s, 2EXC_AveOpTime=0.0s **";
	 //  LogInfo(sstm);
	 //  break;
  // }

  
  switch(m_prType)
  {
  case WRITER:
	  sstm << "** WRITER_"<<m_personalID <<" LOG: #sentMsgs="<<m_sent <<", #InvokedWrites=" << m_opCount <<", #CompletedWrites="<<m_slowOpCount+m_fastOpCount <<", AveOpTime="<< ( (m_opAve.GetSeconds()) /m_opCount) <<"s **";
	  LogInfo(sstm);
	  break;
  case READER:
	  sstm << "** READER_"<<m_personalID << " LOG: #sentMsgs="<<m_sent <<", #InvokedReads=" << m_opCount <<", #CompletedReads="<<m_slowOpCount+m_fastOpCount <<", #3EXCH_reads="<< m_slowOpCount << ", #2EXCH_reads="<<m_fastOpCount<<", AveOpTime="<< ((m_opAve.GetSeconds())/m_opCount) <<"s **";
	  LogInfo(sstm);
	  break;
  }
}

void
OhFastClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

/**************************************************************************************
 * Connection handlers
 **************************************************************************************/
void OhFastClient::HandlePeerClose (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}

void OhFastClient::HandlePeerError (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
}


void OhFastClient::HandleAccept (Ptr<Socket> s, const Address& from)
{
	NS_LOG_FUNCTION (this << s << from);
	s->SetRecvCallback (MakeCallback (&OhFastClient::HandleRecv, this));
	m_socketList.push_back (s);
}



void OhFastClient::ConnectionSucceeded (Ptr<Socket> socket)
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

void OhFastClient::ConnectionFailed (Ptr<Socket> socket)
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
OhFastClient::SetServers (std::vector<Address> ip)
{
	m_serverAddress = ip;
	m_numServers = m_serverAddress.size();

	for (unsigned i=0; i<m_serverAddress.size(); i++)
	{
		NS_LOG_FUNCTION (this << "server" << Ipv4Address::ConvertFrom(m_serverAddress[i]));
	}
}

void
OhFastClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

/**************************************************************************************
 * PACKET Handlers
 **************************************************************************************/
void 
OhFastClient::SetDataSize (uint32_t dataSize)
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
OhFastClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
OhFastClient::SetFill (std::string fill)
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
OhFastClient::ScheduleOperation (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  if (m_prType == READER )
  {
	  m_sendEvent = Simulator::Schedule (dt, &OhFastClient::InvokeRead, this);
  }
  else
  {
	  m_sendEvent = Simulator::Schedule (dt, &OhFastClient::InvokeWrite, this);
  }
}

/**************************************************************************************
 * OhFast Read/Write Handlers
 **************************************************************************************/
void
OhFastClient::InvokeRead (void)
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
		m_opCount ++;
		
		// Reset ts security and 3 phase initiation
		m_initiator = false;
		m_isTsSecured = false;

		AsmCommon::Reset(sstm);
		sstm << "** READ INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
		//Send msg to all
		m_replies = 0;		//reset replies
		HandleSend();
	}
}

void
OhFastClient::InvokeWrite (void)
{
	NS_LOG_FUNCTION (this);
	std::stringstream sstm;

	
	m_opStart = Now();
	
	

	//check if we still have operations to perfrom
	if ( m_opCount <=  m_count )
	{
		m_opStatus = PHASE1;
		m_msgType = WRITE;
		m_ts ++;
		m_opCount ++;
		
		AsmCommon::Reset(sstm);
		sstm << "** WRITE INVOKED: " << m_opCount << " at "<< m_opStart.GetSeconds() <<"s";
		LogInfo(sstm);
		m_value = m_opCount + 900;
		m_replies = 0;
		HandleSend();
	}
}

void 
OhFastClient::HandleSend (void)
{
	NS_LOG_FUNCTION (this);

	NS_ASSERT (m_sendEvent.IsExpired ());


	// Prepare packet content
	std::stringstream pkts;
	std::string message_type;

	// Serialize the appropriate message for READ or WRITE
	if (m_prType == WRITER)
	{
		// serialize <msgType, ts, value, pvalue, counter>
		pkts << m_msgType << " " << m_ts << " " << m_value << " " << m_pvalue << " " << m_opCount;
		message_type = "write";
	}
	else
	{
		// serialize <counter, msgType, ts, value, pvalue, readerID>
		pkts << m_msgType << " " << m_ts << " " << m_value << " " << m_pvalue << " "<< m_personalID << " "<< m_opCount;
		message_type = "read";
	}

	SetFill(pkts.str());

	// Create packet
	Ptr<Packet> p;
	if (m_dataSize)
	{
		NS_ASSERT_MSG (m_dataSize == m_size, "OhFastClient::HandleSend(): m_size and m_dataSize inconsistent");
		NS_ASSERT_MSG (m_data, "OhFastClient::HandleSend(): m_dataSize but no m_data");
		p = Create<Packet> (m_data, m_dataSize);
	}
	else
	{
		p = Create<Packet> (m_size);
	}

	p->RemoveAllPacketTags ();
	p->RemoveAllByteTags ();

	//Send a single packet to each server
	for (int i=0; i<m_serverAddress.size(); i++)
	{
		m_sent++; //count the messages sent
		m_txTrace (p);
		m_socket[i]->Send (p);

		std::stringstream sstm;
		sstm << "Sent " << message_type <<" "<< p->GetSize() << " bytes to " << Ipv4Address::ConvertFrom (m_serverAddress[i])
		<< " port " << m_peerPort << " data " << pkts.str();
		LogInfo ( sstm );
	}
}

void
OhFastClient::HandleRecv (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  uint32_t msgT, msgC;

  while ((packet = socket->RecvFrom (from)))
    {
	  //deserialize the contents of the packet
	  uint8_t buf[1024];
	  packet->CopyData(buf,1024);
	  std::stringbuf sb;
	  sb.str(std::string((char*) buf));
	  std::istream istm(&sb);
	  istm >> msgC >> msgT;

	  std::stringstream sstm;
	  std::string message_type;

	  if (msgT==READACK)
	  {
			message_type = "readAck";
			//istm >> msgTs >> msgV >> msgOp;
	  }
	  else
	  {
			message_type = "writeAck";
			//istm >> msgTs >> msgV;
	  }
	 

	  sstm << "Received " << message_type <<" "<< packet->GetSize () << " bytes from " <<
			  InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
			  InetSocketAddress::ConvertFrom (from).GetPort () << " data " << buf;
	  LogInfo (sstm);

      // check message freshness and if client is waiting
      if ((msgC == m_opCount) && (m_opStatus != IDLE))
       {
    		ProcessReply(istm, from);
       }
    
    }
}

void
OhFastClient::ProcessReply(std::istream& istm, Address sender)
{
	NS_LOG_FUNCTION (this);

	std::stringstream sstm;
	uint32_t msgTs, msgV, msgVp, msgViews, msgTsSecured, msgInit;

	istm >> msgTs >> msgV >> msgVp >> msgViews >> msgTsSecured >> msgInit;

	//increment the number of replies received
	m_replies ++;

	if (m_prType == WRITER)
	{
		/////////////////////////////////
		//////////// WRITER /////////////
		/////////////////////////////////

		if (m_replies >= (m_numServers - m_fail))
		{
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

		//We have to find the max timestamp
		//if new max ts discovered - update the local <ts, value, pvalue>
		if(m_ts < msgTs)
		{
			m_ts = msgTs;
			m_value = msgV;
			m_pvalue = msgVp;

			AsmCommon::Reset(sstm);
			sstm << "Updated local <ts,value> pair to: [" << m_ts << "," << m_value << "," << m_pvalue <<"]";
			LogInfo(sstm);

			//reset the maxAck set and tsSecured variables
			m_repliesSet.clear();
			m_isTsSecured = false;

		}

		// enclosed ts == maxTs - include the msg in the maxAck set and update maxViews variable
		if ( m_ts == msgTs )
		{
			// add the sender and the
			m_repliesSet.push_back(std::make_pair(sender, msgViews));

			//check if the ts was propagated by a reader
			if(msgTsSecured)
			{
				m_isTsSecured = true;

				// this reader initiated 3rd exch
				if ( msgInit )
				{
					m_initiator = true;
				}
			}

		}
		
      	if (m_replies >= (m_numServers - m_fail))
      	{
      		AsmCommon::Reset(sstm);
      		m_opStatus = IDLE;

      		// if ts is secured - return its associated value
      		if ( m_isTsSecured )
      		{
      			m_opEnd = Now();
      			sstm << "** READ COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, Secure Value: "<< m_value <<
      					", <ts, value, pvalue>: ["<< m_ts << "," << m_value << ","<< m_pvalue <<"]";

      			// if read triggered the 3rd phase
      			if( m_initiator )
      			{
      				sstm << "- @ 3 EXCH **";
      				m_slowOpCount++;
      			}
      			else
      			{
      				sstm << "- @ 2 EXCH **";
      				m_fastOpCount++;
      			}
      		}
      		else if ( IsPredicateValid () ) // check the predicate
      		{
      			m_opEnd = Now();
      			sstm << "** READ COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, Return Value: "<< m_value <<
      					", <ts, value, pvalue>: ["<< m_ts << "," << m_value << ","<< m_pvalue <<"] - @ 2 EXCH **";
      			m_fastOpCount++;
      		}
      		else
      		{
      			m_opEnd = Now();
      			sstm << "** READ COMPLETED: " << m_opCount << " in "<< (m_opEnd.GetSeconds() - m_opStart.GetSeconds()) <<"s, Return PValue: "<< m_pvalue <<
      					", <ts, value, pvalue>: ["<< m_ts << "," << m_value << ","<< m_pvalue <<"] - @ 2 EXCH **";
      			m_fastOpCount++;
      		}

			LogInfo(sstm);

      		m_opAve += m_opEnd - m_opStart;
			ScheduleOperation (m_interval);
			m_replies =0;
		}
	}
}

bool
OhFastClient::IsPredicateValid()
{
	NS_LOG_FUNCTION (this);

	std::vector<uint16_t> buckets;
	std::vector< std::pair<Address, uint32_t> >::iterator it;
	int a;
	std::stringstream sstm;

	buckets.resize((int) ((m_numServers/m_fail) - 1));

	// construct the buckets
	for( it = m_repliesSet.begin(); it<m_repliesSet.end(); it++)
	{
		buckets[(*it).second]++;
	}

	for(a = ((m_numServers/m_fail) - 2); a > 0; a--)
	{
		AsmCommon::Reset(sstm);
		sstm << "PREDICATE LOOP: a=" << a << ", b[a]="<< buckets[a] << ", bound=" << (m_numServers - a*m_fail);
		LogInfo(sstm);

		if (buckets[a] >= (m_numServers - a*m_fail))
		{
			return true;
		}
		else
		{
			buckets[a-1] += buckets[a];
		}
	}

	return false;
}

} // Namespace ns3
