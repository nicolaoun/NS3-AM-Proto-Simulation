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
#include "ohSamEX-helper.h"

#include "ns3/ohSamEX-server.h"
#include "ns3/ohSamEX-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

ohSamEXServerHelper::ohSamEXServerHelper (uint16_t port)
{
  m_factory.SetTypeId (ohSamEXServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
ohSamEXServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
ohSamEXServerHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<ohSamEXServer>()->SetServers (serverIps);
}

ApplicationContainer
ohSamEXServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ohSamEXServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ohSamEXServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
ohSamEXServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<ohSamEXServer> ();
  node->AddApplication (app);

  return app;
}

ohSamEXClientHelper::ohSamEXClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (ohSamEXClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

ohSamEXClientHelper::ohSamEXClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (ohSamEXClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
ohSamEXClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
ohSamEXClientHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<ohSamEXClient>()->SetServers (serverIps);
}

void
ohSamEXClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<ohSamEXClient>()->SetFill (fill);
}


ApplicationContainer
ohSamEXClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ohSamEXClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ohSamEXClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
ohSamEXClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<ohSamEXClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
