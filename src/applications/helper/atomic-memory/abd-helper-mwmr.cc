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
#include "abd-helper-mwmr.h"

#include "ns3/abd-server-mwmr.h"
#include "ns3/abd-client-mwmr.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

AbdServerHelperMWMR::AbdServerHelperMWMR (uint16_t port)
{
  m_factory.SetTypeId (AbdServerMWMR::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
AbdServerHelperMWMR::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

// void
// AbdServerHelperMWMR::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
// {
//   app->GetObject<AbdServerHelperMWMR>()->SetServers (serverIps);
// }

ApplicationContainer
AbdServerHelperMWMR::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
AbdServerHelperMWMR::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
AbdServerHelperMWMR::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
AbdServerHelperMWMR::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<AbdServerMWMR> ();
  node->AddApplication (app);

  return app;
}

AbdClientHelperMWMR::AbdClientHelperMWMR (Address address, uint16_t port)
{
  m_factory.SetTypeId (AbdClientMWMR::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

AbdClientHelperMWMR::AbdClientHelperMWMR (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (AbdClientMWMR::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
AbdClientHelperMWMR::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
AbdClientHelperMWMR::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<AbdClientMWMR>()->SetServers (serverIps);
}

void
AbdClientHelperMWMR::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<AbdClientMWMR>()->SetFill (fill);
}

/*
void
AbdClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<AbdClient>()->SetFill (fill, dataLength);
}
*/

ApplicationContainer
AbdClientHelperMWMR::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
AbdClientHelperMWMR::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
AbdClientHelperMWMR::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
AbdClientHelperMWMR::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<AbdClientMWMR> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
