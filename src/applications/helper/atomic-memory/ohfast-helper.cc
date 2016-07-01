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
#include "ohfast-helper.h"
#include "ns3/ohfast-server.h"
#include "ns3/ohfast-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

OhFastServerHelper::OhFastServerHelper (uint16_t port)
{
  m_factory.SetTypeId (OhFastServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
OhFastServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
OhFastServerHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<OhFastServer>()->SetServers (serverIps);
}

ApplicationContainer
OhFastServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhFastServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhFastServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
OhFastServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<OhFastServer> ();
  node->AddApplication (app);

  return app;
}

OhFastClientHelper::OhFastClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (OhFastClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

OhFastClientHelper::OhFastClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (OhFastClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
OhFastClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
OhFastClientHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<OhFastClient>()->SetServers (serverIps);
}

void
OhFastClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<OhFastClient>()->SetFill (fill);
}


ApplicationContainer
OhFastClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhFastClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
OhFastClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
OhFastClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<OhFastClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
