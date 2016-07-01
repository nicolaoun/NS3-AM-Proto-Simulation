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
#include "ohSam-helper.h"

#include "ns3/ohSam-server.h"
#include "ns3/ohSam-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

ohSamServerHelper::ohSamServerHelper (uint16_t port)
{
  m_factory.SetTypeId (ohSamServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
ohSamServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
ohSamServerHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<ohSamServer>()->SetServers (serverIps);
}

ApplicationContainer
ohSamServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ohSamServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ohSamServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
ohSamServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<ohSamServer> ();
  node->AddApplication (app);

  return app;
}

ohSamClientHelper::ohSamClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (ohSamClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port) );
}

ohSamClientHelper::ohSamClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (ohSamClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port) );
}


void 
ohSamClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
ohSamClientHelper::SetServers (Ptr<Application> app, std::vector<Address> serverIps)
{
  app->GetObject<ohSamClient>()->SetServers (serverIps);
}

void
ohSamClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<ohSamClient>()->SetFill (fill);
}


ApplicationContainer
ohSamClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ohSamClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
ohSamClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
ohSamClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<ohSamClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
