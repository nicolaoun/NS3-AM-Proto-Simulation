/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

// Network topology
//
//      w   s0    c1   ...   ci
//      |   |     |    ...    |
//  r1 =======================
//   |           LAN
//   |
//   |  s1  c(i+1)   ...   c(2i)
//   |  |     |      ...    |
//  r2 =======================
//   |           LAN
//   |
//   .
//	 .
//   .
//   |  sn  c((n-1)*i)  ...   c(ni)
//   |  |       |       ...    |
//  rn =========================
//              LAN
//
// - Links between r_i and r_{i+1}: Point to point 1.5Mpbs, 10ms delay
// - Links between nodes in LAN: CSMA 5Mpbs, 2ms delay
// - DropTail queues 


#include <fstream>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/asm-common.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ohSamExample");

int 
main (int argc, char *argv[])
{
	int numServers = 3;
	int numReaders = 2;
	int numWriters = 1;
	int numFail = -1;
	float readInterval = 2;	//read interval in seconds
	float writeInterval = 3;	//read interval in seconds
	int numClients = 0;
	int version=0;
	int seed = 0;
  int verbose=0;

//
// Users may find it convenient to turn on explicit debugging
// for selected modules; the below lines suggest how to do this
//
#if 1
  LogComponentEnable ("ohSamExample", LOG_LEVEL_INFO);
  LogComponentEnable ("ohSamClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("ohSamServerApplication", LOG_LEVEL_INFO);
#endif
//
// Allow the user to override any of the defaults and the above Bind() at
// run-time, via command-line arguments
//
  CommandLine cmd;
  //cmd.AddValue ("useIpv6", "Use Ipv6", useV6);
  cmd.AddValue ("servers", "Number of servers", numServers);
  cmd.AddValue ("readers", "Number of readers", numReaders);
  cmd.AddValue ("writers", "Number of writers", numWriters);
  cmd.AddValue ("failures", "Number of server Failures", numFail);
  cmd.AddValue ("rInterval", "Read interval in seconds", readInterval);
  cmd.AddValue ("wInterval", "Write interval in seconds", writeInterval);
  cmd.AddValue ("version", "Version 0 for FixInt, 1 for randInt", version);
  cmd.AddValue ("seed", "Randomness Seed", seed);
  cmd.AddValue ("verbose", "Debug Mode", verbose);
  cmd.Parse (argc, argv);

  // By default set the failures equal to the minority
  if ( numFail < 0 || numFail > numServers/2 )
  {
	  numFail = (numServers-1)/2;
  }

  //set the number of clients (all together)
  numClients = numReaders+numWriters;

  /********************************************************************
  	 ********************************************************************
  	 *                        CREATE TOPOLOGY							*
  	 ********************************************************************
  	 ********************************************************************/

  	//
  	// Explicitly create the nodes required by the topology (shown above).
  	//
  	NS_LOG_INFO ("Create nodes.");
  	NodeContainer serverNodes;
  	NodeContainer readerNodes;
  	NodeContainer writerNode;
  	NodeContainer routers;
  	routers.Create(numServers);
  	serverNodes.Create(numServers);
  	readerNodes.Create(numReaders);
  	writerNode.Create(1);
  	NodeContainer clientNodes = NodeContainer(readerNodes, writerNode);
  	NodeContainer allNodes = NodeContainer ( routers, serverNodes, clientNodes);

  	NS_LOG_INFO ("Create channels");

  	//
  	// Explicitly create the channels required by the topology (shown above).
  	//
  	InternetStackHelper internet;
  	internet.Install (allNodes);

  	//Create LANs
  	std::vector<NodeContainer> nodeLanList;
  	int clientsPerLan = std::ceil((float) numClients/ (float) numServers);

  	NS_LOG_INFO ("Clients per lan: "<< clientsPerLan);

  	// Create one lan around each server
  	for ( int i=0; i<numServers; i++)
  	{
  		NodeContainer lanClients;
  		for ( int j=i*clientsPerLan; j < numClients && j < (i+1)*clientsPerLan; j++ )
  		{
  			lanClients.Add ( clientNodes.Get(j) );
  		}

  		nodeLanList.push_back( NodeContainer (routers.Get(i), serverNodes.Get(i), lanClients) );
  	}


  	std::vector<NodeContainer> routerAdjacencyList;

  	//connect the routers with p2p
  	for(int i=0; i<numServers-1; ++i)
  	{
  		routerAdjacencyList.push_back ( NodeContainer (routers.Get(i), routers.Get(i+1)) );
  	}


  	CsmaHelper csma;
  	csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  	csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));

  	std::vector<NetDeviceContainer> csmaDeviceAdjacencyList;

  	for(uint32_t i=0; i<nodeLanList.size (); ++i)
  	{
  		csmaDeviceAdjacencyList.push_back( csma.Install (nodeLanList[i]) );
  	}

  	PointToPointHelper p2p;
  	p2p.SetDeviceAttribute ("DataRate", StringValue ("1.5Mbps"));
  	p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  	std::vector<NetDeviceContainer> p2pDeviceAdjacencyList;

  	for(uint32_t i=0; i<routerAdjacencyList.size (); ++i)
  	{
  		p2pDeviceAdjacencyList.push_back( p2p.Install (routerAdjacencyList[i]) );
  	}

  	//
  	// We've got the "hardware" in place.  Now we need to add IP addresses.
  	//
  	NS_LOG_INFO ("Assign IP Addresses.");

  	Ipv4AddressHelper ipv4;
  	std::vector<Ipv4InterfaceContainer> csmaInterfaceAdjacencyList;

  	//assign LAN addresses
  	for(uint32_t i=0; i<csmaDeviceAdjacencyList.size (); ++i)
  	{
  		std::ostringstream subnet;
  		subnet<<"192.168."<<i+1<<".0";
  		ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
  		csmaInterfaceAdjacencyList.push_back ( ipv4.Assign (csmaDeviceAdjacencyList[i]) );
  	}

  	std::vector<Ipv4InterfaceContainer> p2pInterfaceAdjacencyList;
  	//assign Router addresses
  	for(uint32_t i=0; i<p2pDeviceAdjacencyList.size (); ++i)
  	{
  		std::ostringstream subnet;
  		subnet<<"10.1."<<i+1<<".0";
  		ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
  		p2pInterfaceAdjacencyList.push_back ( ipv4.Assign (p2pDeviceAdjacencyList[i]) );
  	}

  	//Turn on global static routing
  	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  	//collect the server adresses
  	std::vector<Address> serverAddress;
  	for(uint32_t i=0; i<csmaInterfaceAdjacencyList.size (); ++i)
  	{
  		serverAddress.push_back(csmaInterfaceAdjacencyList[i].GetAddress(1));
  	}

  	/********************************************************************
  	 *                        ./TOPOLOGY_CREATED						*
  	 ********************************************************************/
//
// Create a ohSamServer application on node one.
//
  NS_LOG_INFO ("Create Servers.");

  uint16_t port = 44400;  // well-known echo port number
  ApplicationContainer s_apps;

  for (int i=0; i<numServers; i++)
  {
	  ohSamServerHelper server (port);
	  server.SetAttribute("PacketSize", UintegerValue (1024) );
	  server.SetAttribute ("ID", UintegerValue (i));
    server.SetAttribute ("Verbose", UintegerValue (verbose));
	  server.SetAttribute("LocalAddress", AddressValue (csmaInterfaceAdjacencyList[i].GetAddress(1)));
	  server.SetAttribute ("MaxFailures", UintegerValue (numFail));
	  //SEt the servers
	  Ptr<Application> app = ((server.Install(serverNodes.Get (i))).Get(0));
	  server.SetServers(app, serverAddress);
	  //server.SetClients(app, clientAddress);
	  s_apps.Add (app);
  }

  s_apps.Start (Seconds (1.0));
  s_apps.Stop (Seconds (30.0));


//
// Create a ohSamClient application to send UDP datagrams from node zero to
// node one.
//
  Time interPacketInterval;
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 10;
  ApplicationContainer c_apps;

  // Create the reader processes
  NS_LOG_INFO ("Create Clients (Writer+Readers).");

  for (int i=0; i<numClients; i++)
  {
	  int lan = (int) (i/clientsPerLan);

	  ohSamClientHelper client (Address(csmaInterfaceAdjacencyList[lan].GetAddress ((i%clientsPerLan)+2)), port);

	  // if this is the writer - set role and interval
	  if(i == 0 )
	  {
		  interPacketInterval = Seconds (writeInterval);
		  client.SetAttribute ("SetRole", UintegerValue(WRITER));				//set writer role
	  }
	  else
	  {
		  interPacketInterval = Seconds (readInterval);
		  client.SetAttribute ("SetRole", UintegerValue(READER));				//set reader role
	  }

	  client.SetAttribute ("MaxOperations", UintegerValue (maxPacketCount));
	  client.SetAttribute ("Port", UintegerValue (port));               // Incoming packets port
	  client.SetAttribute ("ID", UintegerValue (i));    //we want them to start from Writers
	  client.SetAttribute ("MaxFailures", UintegerValue (numFail));
    client.SetAttribute ("Clients", UintegerValue (numClients));
	  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
	  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
	  client.SetAttribute("RandomInterval", UintegerValue (version));
	  client.SetAttribute("Seed", UintegerValue (seed));
    client.SetAttribute ("Verbose", UintegerValue (verbose));
	  Ptr<Application> app = (client.Install (clientNodes.Get (i))).Get(0);
	  client.SetServers(app, serverAddress);
	  c_apps.Add(app);
  }

  c_apps.Start (Seconds (2.0));
  c_apps.Stop (Seconds (30.0));


  //AsciiTraceHelper ascii;
  //csma.EnableAsciiAll (ascii.CreateFileStream ("am-ohSam.tr"));
  //csma.EnablePcapAll ("am-ohSam", false);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation: oh-Sam.");
  //std::cout<<"Run Simulation: oh-Sam."<<std::endl;
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO (">>>> oh-Sam Scenario - Servers:"<<numServers<<", Readers:"<<numReaders<<", Writers:1, Failures:"<<numFail<<", ReadInterval:"<<readInterval<<", WriteInterval:"<<writeInterval<<", <<<<");
  NS_LOG_INFO ("Scenario Succesfully completed.");
  NS_LOG_INFO ("Exiting...");
}
