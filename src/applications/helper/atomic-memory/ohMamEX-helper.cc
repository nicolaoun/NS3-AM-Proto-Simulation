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
#include "OhMamEX-helper.h"

#include "ns3/OhMamEX-server.h"
#include "ns3/OhMamEX-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

OhMamEXServerHelper::OhMamEXServerHelper (uint16_t port)
{
  m_factory.SetTypeId (OhMamEXServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
OhMamEXServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
OhMamEXServerHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<OhMamEXServer>()->SetServers (serverIps);
}

ApplicationContainer
OhMamEXServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhMamEXServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhMamEXServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
OhMamEXServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<OhMamEXServer> ();
  node->AddApplication (app);

  return app;
}

OhMamEXClientHelper::OhMamEXClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (OhMamEXClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

OhMamEXClientHelper::OhMamEXClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (OhMamEXClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
OhMamEXClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
OhMamEXClientHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<OhMamEXClient>()->SetServers (serverIps);
}

void
OhMamEXClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<OhMamEXClient>()->SetFill (fill);
}


ApplicationContainer
OhMamEXClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhMamEXClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhMamEXClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
OhMamEXClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<OhMamEXClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
