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

#ifndef AM_CCHYBRID_CLIENT_H
#define AM_CCHYBRID_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "asm-common.h"
#include <chrono>

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup CCHybrid
 * \brief A CCHybrid client
 *
 * Every packet sent should be returned by the server and received here.
 */
class CCHybridClient : public Application
{
public:
	/**
	 * \brief Get the type ID.
	 * \return the object TypeId
	 */
	static TypeId GetTypeId (void);

	CCHybridClient ();

	virtual ~CCHybridClient ();

	/**
	 * \brief set the remote address and port
	 * \param ip remote IP address
	 * \param port remote port
	 */
	void SetRemote (Address ip, uint16_t port);

	/**
	 * Set the data size of the packet (the number of bytes that are sent as data
	 * to the server).  The contents of the data are set to unspecified (don't
	 * care) by this call.
	 *
	 * \warning If you have set the fill data for the client using one of the
	 * SetFill calls, this will undo those effects.
	 *
	 * \param dataSize The size of the data you want to sent.
	 */
	void SetDataSize (uint32_t dataSize);

	/**
	 * Get the number of data bytes that will be sent to the server.
	 *
	 * \warning The number of bytes may be modified by calling any one of the
	 * SetFill methods.  If you have called SetFill, then the number of
	 * data bytes will correspond to the size of an initialized data buffer.
	 * If you have not called a SetFill method, the number of data bytes will
	 * correspond to the number of don't care bytes that will be sent.
	 *
	 * \returns The number of data bytes.
	 */
	uint32_t GetDataSize (void) const;

	/**
	 * Set the data fill of the packet (what is sent as data to the server) to
	 * the zero-terminated contents of the fill string string.
	 *
	 * \warning The size of resulting echo packets will be automatically adjusted
	 * to reflect the size of the fill string -- this means that the PacketSize
	 * attribute may be changed as a result of this call.
	 *
	 * \param fill The string to use as the actual echo data bytes. Format: "<msgtype, tag, value, sent>"
	 */
	void SetFill (std::string fill);

	/**
	 * Set the data fill of the packet (what is sent as data to the server) to
	 * the contents of the fill buffer, repeated as many times as is required.
	 *
	 * Initializing the packet to the contents of a provided single buffer is
	 * accomplished by setting the fillSize set to your desired dataSize
	 * (and providing an appropriate buffer).
	 *
	 * \warning The size of resulting echo packets will be automatically adjusted
	 * to reflect the dataSize parameter -- this means that the PacketSize
	 * attribute of the Application may be changed as a result of this call.
	 *
	 * \param fill The fill pattern to use when constructing packets.
	 * \param fillSize The number of bytes in the provided fill pattern.
	 * \param dataSize The desired size of the final echo data.
	 */
	void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);

	void SetServers (std::vector<Address> ip);

protected:
	virtual void DoDispose (void);

private:

	virtual void StartApplication (void);
	virtual void StopApplication (void);

	/**
	 * \bief logging helper to record address and time
	 * \param string stream to be printed on the output
	 */
	void LogInfo(std::stringstream& s);
	/**
	 * \brief clear the string stream
	 */
	//void Reset(stringstream& s);
	/**
	 * \brief Schedule the next packet transmission
	 * \param dt time interval between packets.
	 */
	void ScheduleOperation (Time dt);

	/**
	 * \brief Read operation handler
	 */
	void InvokeRead (void);
	/**
	 * \brief Write operation handler
	 */
	void InvokeWrite (void);

	/**
	 * \brief Send a packet
	 */
	void HandleSend (void);

	/**
	 * \brief Handle a packet reception.
	 *
	 * This function is called by lower layers.
	 *
	 * \param socket the socket the packet was received to.
	 */
	void HandleRecv (Ptr<Socket> socket);
	/**
	 * \brief process the received replies
	 * \param type the type of the received message
	 * \param ts the received timestamp
	 * \param val the value associate with ts
	 */
	void ProcessReply(std::istream& istm, Address s);
	/**
	 * \brief check if the predicate is valid on the collected replies
	 */
	bool IsPredicateValid();
	/**
	 * \brief Handle a Connection Succeed event
	 * \param socket the connected socket
	 */
	void ConnectionSucceeded (Ptr<Socket> socket);
	/**
	 * \brief Handle a Connection Failed event
	 * \param socket the not connected socket
	 */
	void ConnectionFailed (Ptr<Socket> socket);

	uint32_t m_size; 		//!< Size of the sent packet
	uint32_t m_dataSize; 	//!< packet payload size (must be equal to m_size)
	uint8_t *m_data; 		//!< packet payload data

	std::vector< Ptr<Socket> > m_socket; //!< Socket
	std::vector<Address> m_serverAddress; //!< Remote server adresses
	Address m_peerAddress; //!< Remote peer address
	Address m_myAddress; //!< Remote peer address
	uint16_t m_peerPort; //!< Remote peer port
	EventId m_sendEvent;
	uint32_t m_personalID; 				//My Personal ID

	uint16_t m_serversConnected;

	// ccHybrid variables
	uint32_t m_ts; 				//!< latest timestamp
	uint32_t m_value;			//!< value associated with m_ts
	uint32_t m_pvalue;			//!< value associated with m_ts - 1 (previous value)

	uint32_t m_maxViews;		//!< maximum #views received
	std::vector< std::pair<Address, uint32_t> > m_repliesSet;
	std::vector<Address> m_propSet;

	uint32_t m_numServers;		//!< number of servers
	uint32_t m_fail;			//!< max number of failures supported

	Status m_opStatus;			//!< operation status
	MessageType m_msgType; 		//!< type of a message send/received
	ProcessType m_prType;		//!< indicate if the client is a writer or a reader

	//timers
	Time m_interval; 		//!< Operation invocation interval
	Time m_opStart;
	Time m_opEnd;
	Time m_opAve;

	//counters
	uint32_t m_opCount;
	uint32_t m_replies;
	uint32_t m_numClients;
	uint32_t m_sent; 		//!< Counter for sent packets
	uint32_t m_count; 		//!< Maximum number of packets the application will send
	uint32_t m_twoExOps;	//!< operations with 2 message exchanges
	uint32_t m_fourExOps;	//!< operations with 4 message exchanges

	//randomness
	uint16_t m_randInt;		//!< Flag indicating the choose of a random interval for each op invocation
	uint16_t m_seed;		//!< Randomness seed
	uint16_t m_verbose;		//!< Debug mode
	std::chrono::time_point<std::chrono::system_clock> m_real_start;
	std::chrono::time_point<std::chrono::system_clock> m_real_end;
	std::chrono::duration<double> m_real_opAve;


	/// Callbacks for tracing the packet Tx events
	TracedCallback<Ptr<const Packet> > m_txTrace;
};

} // namespace ns3

#endif /* AM_CCHYBRID_CLIENT_H */
