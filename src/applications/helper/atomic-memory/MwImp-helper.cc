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
#include "MwImp-helper.h"

#include "ns3/MwImp-server.h"
#include "ns3/MwImp-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

MwImpServerHelper::MwImpServerHelper (uint16_t port)
{
  m_factory.SetTypeId (MwImpServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
MwImpServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
MwImpServerHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<MwImpServer>()->SetServers (serverIps);
}

ApplicationContainer
MwImpServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MwImpServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MwImpServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
MwImpServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<MwImpServer> ();
  node->AddApplication (app);

  return app;
}

MwImpClientHelper::MwImpClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (MwImpClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

MwImpClientHelper::MwImpClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (MwImpClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
MwImpClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
MwImpClientHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<MwImpClient>()->SetServers (serverIps);
}

void
MwImpClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<MwImpClient>()->SetFill (fill);
}


ApplicationContainer
MwImpClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MwImpClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
MwImpClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
MwImpClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<MwImpClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
