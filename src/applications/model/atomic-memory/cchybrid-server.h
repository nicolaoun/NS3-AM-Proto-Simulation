/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#ifndef AM_CCHYBRID_SERVER_H
#define AM_CCHYBRID_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "asm-common.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications 
 * \defgroup CCHybrid CCHybrid
 */

/**
 * \ingroup CCHybrid
 * \brief A Udp Echo server
 *
 * Every packet received is sent back.
 */
class CCHybridServer : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  CCHybridServer ();
  virtual ~CCHybridServer ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  //string stream for ease of output
  void LogInfo(std::stringstream& s);
  /**
   * \brief function to fill the packet with data
   */
  void SetFill (std::string fill);
  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Ptr<Socket> socket);
  /**
   * \brief Handle an incoming connection
   * \param socket the incoming connection socket
   * \param from the address the connection is from
   */
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  /**
   * \brief Handle an connection close
   * \param socket the connected socket
   */
  void HandlePeerClose (Ptr<Socket> socket);
  /**
   * \brief Handle an connection error
   * \param socket the connected socket
   */
  void HandlePeerError (Ptr<Socket> socket);

  uint32_t m_dataSize; 	//!< packet payload size (must be equal to m_size)
  uint8_t *m_data; 		//!< packet payload data

  uint16_t m_port; //!< Port on which we listen for incoming packets.
  uint32_t m_size; //!< The size of the packet
  Ptr<Socket> m_socket; //!< IPv4 Socket
  std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets
  Address m_local; //!< local multicast address
  Address m_myAddress; //!< ip address
  uint32_t m_personalID;        //My Personal ID

  // ccHybrid variables
  uint32_t m_ts; 				//!< latest timestamp
  uint32_t m_value;				//!< value associated with m_ts
  uint32_t m_pvalue;			//!< value associated with m_ts - 1 (previous value)
  std::set< Address > m_seen;	//!< set of IDs storing the IDs of the processes seen our latest ts/value
  bool m_propagated;			//!< optimization flag indicating whether a ts has been propagated by a read
  uint16_t m_optimize;			//!< switch on/off prop optimization

  //uint32_t m_opCount;
  //uint32_t m_completeOps;
  uint32_t m_sent;    //!< Counter for sent packets
  uint16_t m_verbose;   //!< Debug mode
};

} // namespace ns3

#endif /* AM_CCHYBRID_SERVER_H */

