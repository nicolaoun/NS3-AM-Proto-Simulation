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
#include "SwImp-helper.h"

#include "ns3/SwImp-server.h"
#include "ns3/SwImp-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

SwImpServerHelper::SwImpServerHelper (uint16_t port)
{
  m_factory.SetTypeId (SwImpServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
SwImpServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
SwImpServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SwImpServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SwImpServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
SwImpServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<SwImpServer> ();
  node->AddApplication (app);

  return app;
}

SwImpClientHelper::SwImpClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (SwImpClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

SwImpClientHelper::SwImpClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (SwImpClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
SwImpClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
SwImpClientHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<SwImpClient>()->SetServers (serverIps);
}

void
SwImpClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<SwImpClient>()->SetFill (fill);
}

/*
void
SwImpClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<SwImpClient>()->SetFill (fill, dataLength);
}
*/

ApplicationContainer
SwImpClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SwImpClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SwImpClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
SwImpClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<SwImpClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
