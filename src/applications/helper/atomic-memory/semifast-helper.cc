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
#include "semifast-helper.h"

#include "ns3/semifast-server.h"
#include "ns3/semifast-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

SemifastServerHelper::SemifastServerHelper (uint16_t port)
{
  m_factory.SetTypeId (SemifastServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
SemifastServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
SemifastServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SemifastServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SemifastServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
SemifastServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<SemifastServer> ();
  node->AddApplication (app);

  return app;
}

SemifastClientHelper::SemifastClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (SemifastClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

SemifastClientHelper::SemifastClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (SemifastClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
SemifastClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
SemifastClientHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<SemifastClient>()->SetServers (serverIps);
}

void
SemifastClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<SemifastClient>()->SetFill (fill);
}


ApplicationContainer
SemifastClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SemifastClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SemifastClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
SemifastClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<SemifastClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
