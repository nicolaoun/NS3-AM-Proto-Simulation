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
	int readInterval = 2;	//read interval in seconds
	int writeInterval = 3;	//read interval in seconds
  int numClients = 0;

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
  NodeContainer nodes;
  //nodes.Create (numServers+numReaders+1);
  nodes.Create (numServers+numReaders+numWriters);
  //TO - DO

  NS_LOG_INFO ("Create channels for " << nodes.GetN() << " nodes.");
//
// Explicitly create the channels required by the topology (shown above).
//

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  NetDeviceContainer devices = csma.Install (nodes);


  InternetStackHelper internet;
  internet.Install (nodes);

//  NS_LOG_INFO ("Created " << devices.GetN() << " connections.");

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO ("Assign IP Addresses.");

  std::vector<Address> serverAddress;
  std::vector<Address> clientAddress;                            //

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ipIn = ipv4.Assign (devices);
  for(int k=0; k<numServers; k++)
  {
	  serverAddress.push_back(Address(ipIn.GetAddress (k)));
  }

  for(int k=numServers; k<numServers+numReaders+numWriters; k++)       ////
  {
    clientAddress.push_back(Address(ipIn.GetAddress (k)));
  }

// NS_LOG_INFO ("Assigned "<< ipIn.GetN() << " IP Addresses.");

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
	  //server.SetAttribute("LocalAddress", AddressValue (serverAddress[i-1]) );
    server.SetAttribute("LocalAddress", AddressValue (serverAddress[i]) );
    server.SetAttribute ("MaxFailures", UintegerValue (numFail));
    //SEt the servers
    Ptr<Application> app = ((server.Install(nodes.Get (i))).Get(0));
    server.SetServers(app, serverAddress);
    server.SetClients(app, clientAddress);
    s_apps.Add (app);
  }

  s_apps.Start (Seconds (1.0));
  s_apps.Stop (Seconds (15.0));


//
// Create a ohSamClient application to send UDP datagrams from node zero to
// node one.
//
  Time interPacketInterval;
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 10;
  ApplicationContainer c_apps;

  // Create the writer process

  NS_LOG_INFO ("Create the Writers.");

  for (int i=numServers; i<numServers+numWriters; i++)
  {
    interPacketInterval = Seconds (writeInterval);
    ohSamClientHelper client (Address(ipIn.GetAddress (i)), port);
    client.SetAttribute ("MaxOperations", UintegerValue (maxPacketCount));
    client.SetAttribute ("Port", UintegerValue (port));               //
    client.SetAttribute ("ID", UintegerValue (i-numServers));         // We want them to start from Zero
    client.SetAttribute ("MaxFailures", UintegerValue (numFail));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (packetSize));
    client.SetAttribute ("SetRole", UintegerValue(WRITER));       //set writer role
    Ptr<Application> app = (client.Install (nodes.Get (i))).Get(0);
    client.SetServers(app, serverAddress);
    c_apps.Add(app);
  }



  // Create the reader processes
  NS_LOG_INFO ("Create Readers.");

  for (int i=numServers+numWriters; i<numServers+numWriters+numReaders; i++)
  {
	  interPacketInterval = Seconds (readInterval);
	  ohSamClientHelper client (Address(ipIn.GetAddress (i)), port);
	  client.SetAttribute ("MaxOperations", UintegerValue (maxPacketCount));
    client.SetAttribute ("Port", UintegerValue (port));               // Incoming packets port
    client.SetAttribute ("ID", UintegerValue (i-numServers));    //we want them to start from Writers
	  client.SetAttribute ("MaxFailures", UintegerValue (numFail));
	  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
	  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
	  client.SetAttribute ("SetRole", UintegerValue(READER));				//set reader role
	  Ptr<Application> app = (client.Install (nodes.Get (i))).Get(0);
	  client.SetServers(app, serverAddress);
	  c_apps.Add(app);
  }

  c_apps.Start (Seconds (1.0));
  c_apps.Stop (Seconds (15.0));


  //AsciiTraceHelper ascii;
  //csma.EnableAsciiAll (ascii.CreateFileStream ("am-ohSam.tr"));
  //csma.EnablePcapAll ("am-ohSam", false);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation: oh-Sam.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO (">>>> oh-Sam Scenario - Servers:"<<numServers<<", Readers:"<<numReaders<<", Writers:1, Failures:"<<numFail<<", ReadInterval:"<<readInterval<<", WriteInterval:"<<writeInterval<<", <<<<");
  NS_LOG_INFO ("Scenario Succesfully completed.");
  NS_LOG_INFO ("Exiting...");
}
