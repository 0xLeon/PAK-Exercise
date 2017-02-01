#include "sliding-client.h"

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
#include "ns3/udp-socket-factory.h"

#include "ack-server.h"

#include <algorithm>

namespace ns3 {

SlidingClientPacketRecord::SlidingClientPacketRecord(Ptr<Packet> packet, uint64_t sentBytes, Time timeoutTime, SlidingClient* sClient)
	: m_packet(packet)
	, m_sentBytes(sentBytes)
	, m_sClient(sClient) {
	m_eventId = Simulator::Schedule(timeoutTime, &SlidingClientPacketRecord::Timeout, this);
}

void SlidingClientPacketRecord::Timeout() {
	if (m_sClient->m_unackedPackets.count(m_packet->GetUid()) != 0) {
		m_sClient->m_windowFillLevel -= m_sentBytes;
		m_sClient->m_unackedPackets.erase(m_packet->GetUid());

		m_sClient->ScheduleNextPacket(m_sClient->m_packetSize);
	}
}


NS_LOG_COMPONENT_DEFINE("SlidingClient");
NS_OBJECT_ENSURE_REGISTERED(SlidingClient);

TypeId SlidingClient::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::SlidingClient")
		.SetParent<Application>()
		.SetGroupName("Applications")
		.AddConstructor<SlidingClient>()
		.AddAttribute("Remote", "Address of the destination",
			AddressValue(),
			MakeAddressAccessor(&SlidingClient::m_peer),
			MakeAddressChecker())
		.AddAttribute("PacketSize", "Size of one packet in byte.",
			UintegerValue(1400),
			MakeUintegerAccessor(&SlidingClient::m_packetSize),
			MakeUintegerChecker<uint64_t>())
		.AddAttribute("WindowSize", "Window size in byte.",
			UintegerValue(25000),
			MakeUintegerAccessor(&SlidingClient::m_windowSize),
			MakeUintegerChecker<uint64_t>())
		.AddAttribute("DataRate", "The data rate to send packets with.",
			DataRateValue(DataRate("500Kbps")),
			MakeDataRateAccessor(&SlidingClient::m_dataRate),
			MakeDataRateChecker())
		.AddTraceSource("Tx", "A new packet is created and is sent",
			MakeTraceSourceAccessor(&SlidingClient::m_txTrace),
			"ns3::Packet::TracedCallback")
		;

	return tid;
}


SlidingClient::SlidingClient()
	: m_socket(0)
	, m_packetSize(1400)
	, m_windowSize(25000)
	, m_remaining(0)
	, m_windowFillLevel(0)
	, m_nextSendSize(1400)
	, m_sentPackets(0)
	, m_ackedPackets(0) {
	NS_LOG_FUNCTION(this);
}

SlidingClient::~SlidingClient() {
	NS_LOG_FUNCTION(this);
}

void SlidingClient::SetPacketSize(uint64_t packetSize) {
	NS_LOG_FUNCTION(this);
	m_packetSize = packetSize;
}

void SlidingClient::SetWindowSize(uint64_t windowSize) {
	NS_LOG_FUNCTION(this);
	m_windowSize = windowSize;
}

Ptr<Socket> SlidingClient::GetSocket(void) const {
	NS_LOG_FUNCTION(this);
	return m_socket;
}

void SlidingClient::DoDispose(void) {
	NS_LOG_FUNCTION(this);

	m_socket = 0;
	Application::DoDispose();
}

void SlidingClient::StartApplication(void) {
	m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());

	if (Inet6SocketAddress::IsMatchingType(m_peer)) {
		m_socket->Bind6();
	}
	else if (InetSocketAddress::IsMatchingType(m_peer)) {
		m_socket->Bind();
	}

	// m_socket->SetSendCallback(MakeCallback(&SlidingClient::BufferAvailableCb, this));
	m_socket->SetRecvCallback(MakeCallback(&SlidingClient::AckAvailableCb, this));

	ScheduleNextPacket(m_packetSize);
}

void SlidingClient::StopApplication(void) {
	NS_LOG_FUNCTION(this);

	if (m_socket != 0) {
		m_socket->Close();
	}
	else {
		NS_LOG_WARN("SlidingClient found null socket to close in StopApplication");
	}
}


void SlidingClient::ScheduleNextPacket(uint64_t sendSize) {
	NS_LOG_FUNCTION(this);

	Simulator::Cancel(m_sendEventId);

	uint64_t bits = m_packetSize * 8;
	Time nextTime(Seconds(bits / static_cast<double>(m_dataRate.GetBitRate())));

	m_nextSendSize = sendSize;
	m_sendEventId = Simulator::Schedule(nextTime, &SlidingClient::SendPacket, this);
}

void SlidingClient::SendPacket() {
	NS_LOG_FUNCTION(this);

	uint64_t availableWindowSize = m_windowSize - m_windowFillLevel;
	uint64_t toSend = std::min(m_nextSendSize, availableWindowSize);

	// TODO: maybe set a better min bytes send threshold
	if (toSend > 0) {
		Ptr<Packet> packet = Create<Packet>(toSend);
		m_txTrace(packet);
		uint64_t actual = (uint64_t) m_socket->SendTo(packet, 0, m_peer);

		m_windowFillLevel += actual;
		m_sentPackets++;

		Ptr<SlidingClientPacketRecord> pRec = Create<SlidingClientPacketRecord>(packet, actual, MilliSeconds(500), this); // TODO: dynamic packet timeout
		m_unackedPackets[packet->GetUid()] = pRec;

		if (actual != toSend) {
			m_remaining = toSend - actual;
			ScheduleNextPacket(m_remaining);
		}
		else {
			m_remaining = 0;
			ScheduleNextPacket(m_packetSize);
		}
	}
	else {
		if (m_remaining != 0) {
			ScheduleNextPacket(m_remaining);
		}
		else {
			ScheduleNextPacket(m_packetSize);
		}
	}
}

void SlidingClient::BufferAvailableCb(Ptr<Socket>, uint32_t) {
	NS_LOG_FUNCTION(this);

	if (m_remaining != 0) {
		ScheduleNextPacket(m_remaining);
	}
	else {
		ScheduleNextPacket(m_packetSize);
	}
}

void SlidingClient::AckAvailableCb(Ptr<Socket> socket) {
	NS_LOG_FUNCTION(this);

	Ptr<Packet> packet;
	Address from;

	while (packet = socket->RecvFrom(from)) {
		PacketSeqHeader pHeader;
		packet->PeekHeader(pHeader);

		if (m_unackedPackets.count(pHeader.GetSeq()) != 0) {
			m_ackedPackets++;
			Simulator::Cancel(m_unackedPackets[pHeader.GetSeq()]->m_eventId);

			m_windowFillLevel -= m_unackedPackets[pHeader.GetSeq()]->m_sentBytes;
			m_unackedPackets.erase(pHeader.GetSeq());

			ScheduleNextPacket(m_packetSize);
		}
	}
}

} // namespace ns3
