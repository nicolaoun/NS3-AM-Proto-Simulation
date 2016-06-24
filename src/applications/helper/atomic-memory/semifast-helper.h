/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef AM_SEMIFAST_HELPER_H
#define AM_SEMIFAST_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

namespace ns3 {

/**
 * \ingroup Semifast
 * \brief Create a server application which waits for input UDP packets
 *        and sends them back to the original sender.
 */
class SemifastServerHelper
{
public:
  /**
   * Create SemifastServerHelper which will make life easier for people trying
   * to set up simulations with echos.
   *
   * \param port The port the server will wait on for incoming packets
   */
  SemifastServerHelper (uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);
  
  /**
   * Create a SemifastServerApplication on the specified Node.
   *
   * \param node The node on which to create the Application.  The node is
   *             specified by a Ptr<Node>.
   *
   * \returns An ApplicationContainer holding the Application created,
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a SemifastServerApplication on specified node
   *
   * \param nodeName The node on which to create the application.  The node
   *                 is specified by a node name previously registered with
   *                 the Object Name Service.
   *
   * \returns An ApplicationContainer holding the Application created.
   */
  ApplicationContainer Install (std::string nodeName) const;

  /**
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   *
   * Create one Semifast server application on each of the Nodes in the
   * NodeContainer.
   *
   * \returns The applications created, one Application per Node in the 
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c) const;

private:
  /**
   * Install an ns3::SemifastServer on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an SemifastServer will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.
};

/**
 * \ingroup Semifast
 * \brief Create an application which sends a UDP packet and waits for an echo of this packet
 */
class SemifastClientHelper
{
public:
 /**
   * Create SemifastClientHelper which will make life easier for people trying
   * to set up simulations with echos.
   *
   * \param ip The IP address of the remote Semifast server
   * \param port The port number of the remote Semifast server
   */
  SemifastClientHelper (Address ip, uint16_t port);
  /**
   * Create SemifastClientHelper which will make life easier for people trying
   * to set up simulations with echos.
   *
   * \param ip The IPv4 address of the remote Semifast server
   * \param port The port number of the remote Semifast server
   */
  SemifastClientHelper (Ipv4Address ip, uint16_t port);
  /**
   * Create SemifastClientHelper which will make life easier for people trying
   * to set up simulations with echos.
   *
   * \param ip The IPv6 address of the remote Semifast server
   * \param port The port number of the remote Semifast server
   */
  SemifastClientHelper (Ipv6Address ip, uint16_t port);

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Given an SemifastClient application and a vector of server ip addresses
   * set the set of destinations at the client
   *
   * \param app Smart pointer to the application
   * \param vector of ip addresses
   */
  void SetServers (Ptr<Application> app, std::vector<Address> serverIps);

  /**
   * Given a pointer to a SemifastClient application, set the data fill of the
   * packet (what is sent as data to the server) to the contents of the fill
   * string (including the trailing zero terminator).
   *
   * \warning The size of resulting echo packets will be automatically adjusted
   * to reflect the size of the fill string -- this means that the PacketSize
   * attribute may be changed as a result of this call.
   *
   * \param app Smart pointer to the application (real type must be SemifastClient).
   * \param fill The string to use as the actual echo data bytes.
   */
  void SetFill (Ptr<Application> app, std::string fill);

  /**
   * Given a pointer to a SemifastClient application, set the data fill of the
   * packet (what is sent as data to the server) to the contents of the fill
   * byte.
   *
   * The fill byte will be used to initialize the contents of the data packet.
   * 
   * \warning The size of resulting echo packets will be automatically adjusted
   * to reflect the dataLength parameter -- this means that the PacketSize
   * attribute may be changed as a result of this call.
   *
   * \param app Smart pointer to the application (real type must be SemifastClient).
   * \param fill The byte to be repeated in constructing the packet data..
   * \param dataLength The desired length of the resulting echo packet data.
   */
  //void SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength);

  /**
   * Create a Semifast client application on the specified node.  The Node
   * is provided as a Ptr<Node>.
   *
   * \param node The Ptr<Node> on which to create the SemifastClientApplication.
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the 
   *          application created
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a Semifast client application on the specified node.  The Node
   * is provided as a string name of a Node that has been previously 
   * associated using the Object Name Service.
   *
   * \param nodeName The name of the node on which to create the SemifastClientApplication
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the 
   *          application created
   */
  ApplicationContainer Install (std::string nodeName) const;

  /**
   * \param c the nodes
   *
   * Create one Semifast client application on each of the input nodes
   *
   * \returns the applications created, one application per input node.
   */
  ApplicationContainer Install (NodeContainer c) const;

private:
  /**
   * Install an ns3::SemifastClient on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an SemifastClient will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* AM_SEMIFAST_HELPER_H */