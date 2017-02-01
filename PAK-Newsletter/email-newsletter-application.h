/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
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
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#ifndef EMAIL_NEWSLETTER_APPLICATION_H
#define EMAIL_NEWSLETTER_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Address;
class Socket;

class EmailNewsletterApplication : public Application {
public:
	static TypeId GetTypeId(void);

	EmailNewsletterApplication();
	virtual ~EmailNewsletterApplication();

	void SetRtt(uint64_t rtt);
	void SetReceiverPerServer(uint32_t rps);

	Ptr<Socket> GetSocket(void) const;

protected:
	virtual void DoDispose(void);

private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);

	void ConnectionSucceeded(Ptr<Socket> socket);
	void ConnectionFailed(Ptr<Socket> socket);
	void SendMail(void);
	void SendRemaining(void);
	void ScheduleNextMail(void);
	void CheckMailCount(void);
	void BufferAvailableCb(Ptr<Socket>, uint32_t);

	Ptr<Socket>	m_socket;	//!< Associated socket
	Address		m_peer;		//!< Peer address
	bool		m_connected;	//!< True if connected
	uint32_t	m_sendSize;	//!< Size of data to send each time
	TypeId		m_tid;		//!< The type of protocol to use.

	EventId		m_sendEvent;
	uint64_t	m_rtt;
	uint32_t	m_receiverPerServer;
	uint32_t	m_sentMails;
	uint64_t	m_remaining;

	TracedCallback<Ptr<const Packet>> m_txTrace;
};

} // namespace ns3

#endif /* EMAIL_NEWSLETTER_APPLICATION_H */
