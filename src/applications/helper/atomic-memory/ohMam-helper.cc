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
#include "ohMam-helper.h"

#include "ns3/ohMam-server.h"
#include "ns3/ohMam-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

OhMamServerHelper::OhMamServerHelper (uint16_t port)
{
  m_factory.SetTypeId (OhMamServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
OhMamServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
OhMamServerHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<OhMamServer>()->SetServers (serverIps);
}

void
OhMamServerHelper::SetClients (Ptr<Application> app, std::vector<Address> clientIps)
{
  app->GetObject<OhMamServer>()->SetClients (clientIps);
}

ApplicationContainer
OhMamServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhMamServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhMamServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
OhMamServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<OhMamServer> ();
  node->AddApplication (app);

  return app;
}

OhMamClientHelper::OhMamClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (OhMamClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

OhMamClientHelper::OhMamClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (OhMamClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
OhMamClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
OhMamClientHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<OhMamClient>()->SetServers (serverIps);
}

void
OhMamClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<OhMamClient>()->SetFill (fill);
}


ApplicationContainer
OhMamClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhMamClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhMamClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
OhMamClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<OhMamClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
