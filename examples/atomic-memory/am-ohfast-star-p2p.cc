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
//         w      c1   ...   ci
//         |      |    ...    |
//  s1--r1 =======================
//      |           LAN
//      |
//      |   c(i+1)   ...   c(2i)
//      |     |      ...    |
//  s2--r2 =======================
//      |           LAN
//      |
//      .
//	    .
//      .
//      |   c((n-1)*i)  ...   c(ni)
//      |       |       ...    |
//  sn--rn =========================
//              LAN
//
// - Links between r_i and s_i: Point to point 1.5Mpbs, 10ms delay
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
	int version=0;
	int seed = 0;
	int verbose = 0;

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
                routers.Create(numServers+1);
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

                // Create one lan per server
                for ( int i=0; i<numServers; i++)
                {
                        NodeContainer lanClients;
                        for ( int j=i*clientsPerLan; j < numClients && j < (i+1)*clientsPerLan; j++ )
                        {
                                lanClients.Add ( clientNodes.Get(j) );
                        }

                        nodeLanList.push_back( NodeContainer (routers.Get(i+1), lanClients));
                }


        std::vector<NodeContainer> routerServersAdjacencyList;
        std::vector<NodeContainer> routerClientsAdjacencyList;
        std::vector<NodeContainer> routerAdjacencyList;

                //connect servers to the first router and the routers with p2p
                for(int i=0; i<numServers; ++i)
                {
            //connect servers with the first router
                        routerServersAdjacencyList.push_back( NodeContainer (routers.Get(0), serverNodes.Get(i)) );

            //connect clients to the router
            for ( int j=i*clientsPerLan; j < numClients && j < (i+1)*clientsPerLan; j++ )
            {
                routerClientsAdjacencyList.push_back( NodeContainer (routers.Get(i+1), clientNodes.Get(j)) );
            }

            //connect routers
                        routerAdjacencyList.push_back ( NodeContainer (routers.Get(i), routers.Get(i+1)) );
                }

        PointToPointHelper p2p;
        p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
        p2p.SetChannelAttribute ("Delay", StringValue ("4ms"));
        p2p.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue (1000));
        std::vector<NetDeviceContainer> p2pDeviceAdjacencyList;

        for(uint32_t i=0; i<routerAdjacencyList.size (); ++i)
        {
            p2pDeviceAdjacencyList.push_back( p2p.Install (routerAdjacencyList[i]) );
        }

        PointToPointHelper p2pServers;
        p2pServers.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
        p2pServers.SetChannelAttribute ("Delay", StringValue ("1ms"));
        p2pServers.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue (1000));
        std::vector<NetDeviceContainer> p2pServersDeviceAdjacencyList;

        for(uint32_t i=0; i<routerServersAdjacencyList.size (); ++i)
        {
            p2pServersDeviceAdjacencyList.push_back( p2pServers.Install (routerServersAdjacencyList[i]) );
        }

        PointToPointHelper p2pClients;
        p2pClients.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
        p2pClients.SetChannelAttribute ("Delay", StringValue ("2ms"));
        p2pClients.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue (1000));
        std::vector<NetDeviceContainer> p2pClientsDeviceAdjacencyList;

        for(uint32_t i=0; i<routerClientsAdjacencyList.size (); ++i)
        {
            p2pClientsDeviceAdjacencyList.push_back( p2pClients.Install (routerClientsAdjacencyList[i]) );
        }

        //
        // We've got the "hardware" in place.  Now we need to add IP addresses.
        //
        NS_LOG_INFO ("Assign IP Addresses.");

        Ipv4AddressHelper ipv4;

        //assign Router addresses
        std::vector<Ipv4InterfaceContainer> p2pInterfaceAdjacencyList;
        for(uint32_t i=0; i<p2pDeviceAdjacencyList.size (); ++i)
        {
            std::ostringstream subnet;
            subnet<<"10.1."<<i+1<<".0";
            ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
            p2pInterfaceAdjacencyList.push_back ( ipv4.Assign (p2pDeviceAdjacencyList[i]) );
        }

        //assign Server addresses
        std::vector<Ipv4InterfaceContainer> p2pServersInterfaceAdjacencyList;
        for(uint32_t i=0; i<p2pServersDeviceAdjacencyList.size (); ++i)
        {
            std::ostringstream subnet;
            subnet<<"10.2."<<i+1<<".0";
            ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
            p2pServersInterfaceAdjacencyList.push_back ( ipv4.Assign (p2pServersDeviceAdjacencyList[i]) );
        }

        //assign Client addresses
        std::vector<Ipv4InterfaceContainer> p2pClientsInterfaceAdjacencyList;
        for(uint32_t i=0; i<p2pClientsDeviceAdjacencyList.size (); ++i)
        {
            std::ostringstream subnet;
            subnet<<"192.168."<<i+1<<".0";
            ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
            p2pClientsInterfaceAdjacencyList.push_back ( ipv4.Assign (p2pClientsDeviceAdjacencyList[i]) );
        }

        //Turn on global static routing
        Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


        //collect the server adresses
        std::vector<Address> serverAddress;
        for(uint32_t i=0; i<p2pServersInterfaceAdjacencyList.size (); ++i)
        {
            serverAddress.push_back(p2pServersInterfaceAdjacencyList[i].GetAddress(1));
        }

                /********************************************************************
                 *                        ./TOPOLOGY_CREATED						*
                 ********************************************************************/


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
		server.SetAttribute ("Verbose", UintegerValue (verbose));
		server.SetAttribute("LocalAddress", AddressValue (p2pServersInterfaceAdjacencyList[i].GetAddress(1)) );
		//server.SetAttribute("LocalAddress", AddressValue ("0.0.0.0") );
		server.SetAttribute ("MaxFailures", UintegerValue (numFail));
		//Set the servers
		Ptr<Application> app = ((server.Install(serverNodes.Get (i))).Get(0));
		server.SetServers(app, serverAddress);
		//server.SetClients(app, clientAddress);
		s_apps.Add (app);
	}

	s_apps.Start (Seconds (1.0));
	s_apps.Stop (Seconds (30.0));


	//
	// Create a OhFastClient application to send UDP datagrams from node zero to
	// node one.
	//
	Time interPacketInterval;
	uint32_t packetSize = 1024;
	uint32_t maxPacketCount = 10;
	ApplicationContainer c_apps;

	// Create the writer process
/*
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
*/


	// Create the reader processes
	NS_LOG_INFO ("Create Clients (Writer+Readers).");

	for (int i=0; i<numClients; i++)
	{
        //int lan = (int) (i/clientsPerLan);

        OhFastClientHelper client (Address(p2pClientsInterfaceAdjacencyList[i].GetAddress (1)), port);

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
		client.SetAttribute ("Clients", UintegerValue (numClients));    //we want them to start from Writers
		client.SetAttribute ("MaxFailures", UintegerValue (numFail));
		client.SetAttribute ("Interval", TimeValue (interPacketInterval));
		client.SetAttribute ("PacketSize", UintegerValue (packetSize));
		client.SetAttribute("RandomInterval", UintegerValue (version));
		client.SetAttribute("Seed", UintegerValue (seed));
		client.SetAttribute ("Verbose", UintegerValue (verbose));
		Ptr<Application> app = (client.Install (clientNodes.Get (i))).Get(0);
		client.SetServers(app, serverAddress);
		c_apps.Add(app);
	}

	c_apps.Start (Seconds (1.0));
	c_apps.Stop (Seconds (30.0));


	// AsciiTraceHelper ascii;
	// csma.EnableAsciiAll (ascii.CreateFileStream ("am-OhFast.tr"));
	// csma.EnablePcapAll ("am-OhFast", false);

	//
	// Now, do the actual simulation.
	//
	NS_LOG_INFO ("Run Simulation: oh-fast.");
	//std::cout<<"Run Simulation: oh-fast SPIKE."<<std::endl;
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO (">>>> oh-fast Scenario - Servers:"<<numServers<<", Readers:"<<numReaders<<", Writers:"<<numWriters<<", Failures:"<<numFail<<", ReadInterval:"<<readInterval<<", WriteInterval:"<<writeInterval<<", <<<<");
  NS_LOG_INFO ("Scenario Succesfully completed.");
  NS_LOG_INFO ("Exiting...");
}
