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

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "email-newsletter-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("EmailNewsletterApplication");
NS_OBJECT_ENSURE_REGISTERED(EmailNewsletterApplication);

TypeId EmailNewsletterApplication::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::EmailNewsletterApplication")
		.SetParent<Application>()
		.SetGroupName("Applications") 
		.AddConstructor<EmailNewsletterApplication>()
		.AddAttribute("SendSize", "The amount of data to send each time.",
				UintegerValue(100000),
				MakeUintegerAccessor(&EmailNewsletterApplication::m_sendSize),
				MakeUintegerChecker<uint32_t>(1))
		.AddAttribute("Remote", "The address of the destination",
				AddressValue(),
				MakeAddressAccessor(&EmailNewsletterApplication::m_peer),
				MakeAddressChecker())
		.AddAttribute("Rtt", "The RTT delay value in milli seconds.",
				UintegerValue(50),
				MakeUintegerAccessor(&EmailNewsletterApplication::m_rtt),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("ReceiverPerServer", "The number of mail receivers per mail server.",
				UintegerValue(100),
				MakeUintegerAccessor(&EmailNewsletterApplication::m_rtt),
				MakeUintegerChecker<uint32_t>(1))
		.AddAttribute("Protocol", "The type of protocol to use.",
				TypeIdValue(TcpSocketFactory::GetTypeId()),
				MakeTypeIdAccessor(&EmailNewsletterApplication::m_tid),
				MakeTypeIdChecker())
		.AddTraceSource("Tx", "A new packet is created and is sent",
				MakeTraceSourceAccessor(&EmailNewsletterApplication::m_txTrace),
				"ns3::Packet::TracedCallback");
	
	return tid;
}


EmailNewsletterApplication::EmailNewsletterApplication()
	: m_socket(0),
	  m_connected(false),
	  m_rtt(50),
	  m_receiverPerServer(100),
	  m_sentMails(0),
	  m_remaining(0) {
	NS_LOG_FUNCTION(this);
}

EmailNewsletterApplication::~EmailNewsletterApplication() {
	NS_LOG_FUNCTION(this);
}

void EmailNewsletterApplication::SetRtt(uint64_t rtt) {
	NS_LOG_FUNCTION(this);
	m_rtt = rtt;
}

void EmailNewsletterApplication::SetReceiverPerServer(uint32_t rps) {
	NS_LOG_FUNCTION(this);
	m_receiverPerServer = rps;
}

Ptr<Socket> EmailNewsletterApplication::GetSocket(void) const {
	NS_LOG_FUNCTION(this);
	return m_socket;
}

void EmailNewsletterApplication::DoDispose(void) {
	NS_LOG_FUNCTION(this);

	m_socket = 0;
	// chain up
	Application::DoDispose();
}

// Application Methods

// Called at time specified by Start
void EmailNewsletterApplication::StartApplication(void) {
	NS_LOG_FUNCTION(this);

	// Create the socket if not already
	if (!m_socket) {
		m_socket = Socket::CreateSocket(GetNode(), m_tid);

		// Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
		if (m_socket->GetSocketType() != Socket::NS3_SOCK_STREAM && m_socket->GetSocketType() != Socket::NS3_SOCK_SEQPACKET) {
			 NS_FATAL_ERROR("Using EmailNewsletter with an incompatible socket type. "
					"EmailNewsletter requires SOCK_STREAM or SOCK_SEQPACKET. "
					"In other words, use TCP instead of UDP.");
		}

		if (Inet6SocketAddress::IsMatchingType(m_peer)) {
			m_socket->Bind6();
		}
		else if (InetSocketAddress::IsMatchingType(m_peer)) {
			m_socket->Bind();
		}

		m_socket->Connect(m_peer);
		m_socket->ShutdownRecv();
		m_socket->SetConnectCallback(
			MakeCallback(&EmailNewsletterApplication::ConnectionSucceeded, this),
			MakeCallback(&EmailNewsletterApplication::ConnectionFailed, this)
		);
		m_socket->SetSendCallback(MakeCallback(&EmailNewsletterApplication::BufferAvailableCb, this));
	}
	
	if (m_connected) {
		Time tcpOverheadTime(MilliSeconds(m_rtt));
		m_sendEvent = Simulator::Schedule(tcpOverheadTime, &EmailNewsletterApplication::ScheduleNextMail, this);
	}
}

// Called at time specified by Stop
void EmailNewsletterApplication::StopApplication(void) {
	NS_LOG_FUNCTION(this);

	if (m_socket != 0) {
		m_socket->Close();
		m_connected = false;
	}
	else {
		NS_LOG_WARN("EmailNewsletterApplication found null socket to close in StopApplication");
	}
}


// Private helpers

void EmailNewsletterApplication::SendMail(void) {
	NS_LOG_FUNCTION(this);

	// Time to send more

	// uint64_t to allow the comparison later.
	// the result is in a uint32_t range anyway, because
	// m_sendSize is uint32_t.
	uint64_t toSend = m_sendSize;

	NS_LOG_LOGIC ("sending packet at " << Simulator::Now());
	Ptr<Packet> packet = Create<Packet>(toSend);
	m_txTrace (packet);
	int actual = m_socket->Send(packet);
	
	// We exit this loop when actual < toSend as the send side
	// buffer is full. The "DataSent" callback will pop when
	// some buffer space has freed ip.
	if ((unsigned) actual != toSend) {
		m_remaining = toSend - actual;
	}
	else {
		m_remaining = 0;
		m_sentMails++;
		
		ScheduleNextMail();
	}
	
	CheckMailCount();
}

void EmailNewsletterApplication::SendRemaining(void) {
	NS_LOG_FUNCTION(this);
	NS_LOG_LOGIC("sending packet at " << Simulator::Now());
	
	uint64_t toSend = m_remaining;
	Ptr<Packet> packet = Create<Packet>(toSend);
	m_txTrace(packet);
	int actual = m_socket->Send(packet);
	
	if ((unsigned) actual != toSend) {
		m_remaining = toSend - actual;
	}
	else {
		m_remaining = 0;
		m_sentMails++;
		
		ScheduleNextMail();
	}
	
	CheckMailCount();
}

void EmailNewsletterApplication::ScheduleNextMail(void) {
	NS_LOG_FUNCTION(this);
	
	Time smtpOverheadTime(MilliSeconds(m_rtt * 4));
	m_sendEvent = Simulator::Schedule(smtpOverheadTime, &EmailNewsletterApplication::SendMail, this);
}

void EmailNewsletterApplication::CheckMailCount(void) {
	if (m_sentMails >= m_receiverPerServer) {
		m_sentMails = 0;
		Simulator::Cancel(m_sendEvent);
		
		Time tcpOverheadTime(MilliSeconds(m_rtt * 2));
		m_sendEvent = Simulator::Schedule(tcpOverheadTime, &EmailNewsletterApplication::ScheduleNextMail, this);
	}
}

void EmailNewsletterApplication::ConnectionSucceeded(Ptr<Socket> socket) {
	NS_LOG_FUNCTION(this << socket);
	NS_LOG_LOGIC("EmailNewsletterApplication Connection succeeded");
	
	m_connected = true;
}

void EmailNewsletterApplication::ConnectionFailed(Ptr<Socket> socket) {
	NS_LOG_FUNCTION(this << socket);
	NS_LOG_LOGIC("EmailNewsletterApplication, Connection Failed");
}

void EmailNewsletterApplication::BufferAvailableCb(Ptr<Socket>, uint32_t) {
	NS_LOG_FUNCTION(this);

	if (m_connected && m_remaining != 0) {
		// data left to send, do this immediately
		SendRemaining();
	}
}

} // Namespace ns3
