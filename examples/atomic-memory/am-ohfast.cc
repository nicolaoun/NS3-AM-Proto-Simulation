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
//   w   s0    s1   ...   s_n   c0    c1   ...   c_m
//   |   |     |    ...    |    |     |    ...    |
//   ==============================================
//                          LAN
//
// - UDP flows from n0 to n1 and back
// - DropTail queues 
// - Tracing of queues and packet receptions to file "udp-echo.tr"

// Default Network topology, 9 nodes in a star
/*
          n2 n3 n4
           \ | /
            \|/
       n1---n0---n5
            /| \
           / | \
          n8 n7 n6
*/

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/asm-common.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("OhFastExample");

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
	int version=1;

	//
	// Users may find it convenient to turn on explicit debugging
	// for selected modules; the below lines suggest how to do this
	//
#if 1
	LogComponentEnable ("OhFastExample", LOG_LEVEL_INFO);
	LogComponentEnable ("OhFastClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("OhFastServerApplication", LOG_LEVEL_INFO);
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
	cmd.AddValue ("version", "Version 1 for FixInt, 2 for randInt", version);
	cmd.Parse (argc, argv);

	// By default set the failures equal to the minority
	if ( numFail < 0 || numFail > numServers/2 )
	{
		numFail = (numServers-1)/2;
	}

	//set the number of clients (all together)
	numClients = numReaders+numWriters;

	//
	// Explicitly create the nodes required by the topology (shown above).
	//
	NS_LOG_INFO ("Create nodes.");
	NodeContainer serverNodes;
	NodeContainer readerNodes;
	NodeContainer writerNode;
	serverNodes.Create(numServers);
	readerNodes.Create(numReaders);
	writerNode.Create(1);
	NodeContainer clientNodes = NodeContainer(readerNodes, writerNode);
	NodeContainer allNodes = NodeContainer ( serverNodes, clientNodes);

	NS_LOG_INFO ("Create channels"); // for " << nodes.GetN() << " nodes.");
	//
	// Explicitly create the channels required by the topology (shown above).
	//

	InternetStackHelper internet;
	internet.Install (allNodes);

	//Collect an adjacency list of nodes for the p2p topology
	//connect every client with a single server
	std::vector<NodeContainer> clientAdjacencyList (numClients*numServers);

	//connect the writer with the first server
	//clientAdjacencyList[0] = NodeContainer (writerNode, serverNodes(0));


	//clients to all servers
	for(uint32_t i=0; i<numClients; ++i)
	{
		for(uint32_t j=0; j<numServers; ++j)
		{
			clientAdjacencyList[numServers*i+j] = NodeContainer (serverNodes.Get(j), clientNodes.Get(i));
		}
	}

	std::vector<NodeContainer> serverAdjacencyList;

	//servers to all servers
	for(uint32_t i=0; i<numServers; ++i)
	{
		// connect i to i+1, i+2, ...., n
		for(uint32_t j=i+1; j<numServers; ++j)
		{
			serverAdjacencyList.push_back ( NodeContainer (serverNodes.Get(i), serverNodes.Get(j)) );
		}
	}

	/*
	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
	csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
	NetDeviceContainer devices = csma.Install (nodes);
	*/
	PointToPointHelper p2p;
	p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
	std::vector<NetDeviceContainer> deviceAdjacencyList;

	for(uint32_t i=0; i<clientAdjacencyList.size (); ++i)
	{
		deviceAdjacencyList.push_back( p2p.Install (clientAdjacencyList[i]) );
	}

	for(uint32_t i=0; i<serverAdjacencyList.size (); ++i)
	{
		deviceAdjacencyList.push_back( p2p.Install (serverAdjacencyList[i]) );
	}

	//  NS_LOG_INFO ("Created " << devices.GetN() << " connections.");

	//
	// We've got the "hardware" in place.  Now we need to add IP addresses.
	//
	NS_LOG_INFO ("Assign IP Addresses.");

	Ipv4AddressHelper ipv4;
	std::vector<Ipv4InterfaceContainer> interfaceAdjacencyList;
	for(uint32_t i=0; i<interfaceAdjacencyList.size (); ++i)
	{
		std::ostringstream subnet;
		subnet<<"10.1."<<i+1<<".0";
		ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
		interfaceAdjacencyList.push_back ( ipv4.Assign (deviceAdjacencyList[i]) );
	}

	//Turn on global static routing
	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


	std::vector<Address> tmpAddress;
	std::vector< std::vector <Address> > serverAddress;                            //
	/*
	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ipIn = ipv4.Assign (devices);
	*/

	for(uint32_t i=0; i<numClients; ++i)
	{
		tmpAddress.clear();

		for(uint32_t j=0; j<numServers; ++j)
		{
			// get the address of each server
			tmpAddress.push_back(Address(interfaceAdjacencyList[j+numServers*i].GetAddress (1)));
		}

		serverAddress.push_back(tmpAddress);
	}

	/*
	for(int k=numServers; k<numServers+numReaders+numWriters; k++)       ////
	{
		clientAddress.push_back(Address(ipIn.GetAddress (k)));
	}
	*/

	// NS_LOG_INFO ("Assigned "<< ipIn.GetN() << " IP Addresses.");

	//
	// Create a OhFastServer application on node one.
	//
	NS_LOG_INFO ("Create Servers.");

	uint16_t port = 44400;  // well-known echo port number
	ApplicationContainer s_apps;

	for (int i=0; i<numServers; i++)
	{
		OhFastServerHelper server (port);
		server.SetAttribute("PacketSize", UintegerValue (1024) );
		server.SetAttribute ("ID", UintegerValue (i));
		//server.SetAttribute("LocalAddress", AddressValue (serverAddress[i-1]) );
		//server.SetAttribute("LocalAddress", AddressValue ("0.0.0.0") );
		server.SetAttribute ("MaxFailures", UintegerValue (numFail));
		//Set the servers
		Ptr<Application> app = ((server.Install(serverNodes.Get (i))).Get(0));
		//server.SetServers(app, serverAddress);
		//server.SetClients(app, clientAddress);
		s_apps.Add (app);
	}

	s_apps.Start (Seconds (1.0));
	s_apps.Stop (Seconds (60.0));


	//
	// Create a OhFastClient application to send UDP datagrams from node zero to
	// node one.
	//
	Time interPacketInterval;
	uint32_t packetSize = 1024;
	uint32_t maxPacketCount = 10;
	ApplicationContainer c_apps;

	// Create the writer process

	NS_LOG_INFO ("Create the Writer.");

	interPacketInterval = Seconds (writeInterval);
	OhFastClientHelper client (Address(interfaceAdjacencyList[0].GetAddress (0)), port);
	client.SetAttribute ("MaxOperations", UintegerValue (maxPacketCount));
	client.SetAttribute ("Port", UintegerValue (port));               //
	client.SetAttribute ("ID", UintegerValue (0));         // We want them to start from Zero
	client.SetAttribute ("MaxFailures", UintegerValue (numFail));
	client.SetAttribute ("Interval", TimeValue (interPacketInterval));
	client.SetAttribute ("PacketSize", UintegerValue (packetSize));
	client.SetAttribute ("SetRole", UintegerValue(WRITER));       //set writer role
	Ptr<Application> app = (client.Install (clientNodes.Get(0))).Get(0);
	client.SetServers(app, serverAddress[0]);
	c_apps.Add(app);



	// Create the reader processes
	NS_LOG_INFO ("Create Readers.");

	for (int i=1; i<numReaders; i++)
	{
		interPacketInterval = Seconds (readInterval);
		OhFastClientHelper client (Address(interfaceAdjacencyList[i].GetAddress (0)), port);
		client.SetAttribute ("MaxOperations", UintegerValue (maxPacketCount));
		client.SetAttribute ("Port", UintegerValue (port));               // Incoming packets port
		client.SetAttribute ("ID", UintegerValue (i));    //we want them to start from Writers
		client.SetAttribute ("MaxFailures", UintegerValue (numFail));
		client.SetAttribute ("Interval", TimeValue (interPacketInterval));
		client.SetAttribute ("PacketSize", UintegerValue (packetSize));
		client.SetAttribute ("SetRole", UintegerValue(READER));				//set reader role
		Ptr<Application> app = (client.Install (clientNodes.Get (i))).Get(0);
		client.SetServers(app, serverAddress[i]);
		c_apps.Add(app);
	}

	c_apps.Start (Seconds (1.0));
	c_apps.Stop (Seconds (60.0));


	// AsciiTraceHelper ascii;
	// csma.EnableAsciiAll (ascii.CreateFileStream ("am-OhFast.tr"));
	// csma.EnablePcapAll ("am-OhFast", false);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	//
	// Now, do the actual simulation.
	//
	NS_LOG_INFO ("Run Simulation: oh-fast.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO (">>>> oh-fast Scenario - Servers:"<<numServers<<", Readers:"<<numReaders<<", Writers:"<<numWriters<<", Failures:"<<numFail<<", ReadInterval:"<<readInterval<<", WriteInterval:"<<writeInterval<<", <<<<");
  NS_LOG_INFO ("Scenario Succesfully completed.");
  NS_LOG_INFO ("Exiting...");
}
