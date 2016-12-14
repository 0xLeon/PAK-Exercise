#include "ack-server.h"

#include "ns3/trace-source-accessor.h"
#include "ns3/traced-callback.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"
#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/udp-socket-factory.h"

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED(AckServerApplication);

TypeId AckServerApplication::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::AckServerApplication")
		.SetParent<Application>()
		.SetGroupName("Applications")
		.AddConstructor<AckServerApplication>()
		;

	return tid;
}

AckServerApplication::AckServerApplication(void)
	: m_socket(NULL)
	, m_totalPacketsReceived(0)
	, m_totalBytesReceived(0)
	, m_meanDataRate(0)
	, m_dataRate0(0)
	, m_dataRate1(0)
	, m_dataRate2(0)
	{ }

AckServerApplication::~AckServerApplication(void) {}



Time AckServerApplication::GetStartTime(void) {
	return m_startTime;
}

uint64_t AckServerApplication::GetTotalPacketsReceived(void) {
	return m_totalPacketsReceived;
}

uint64_t AckServerApplication::GetTotalBytesReceived(void) {
	return m_totalBytesReceived;
}

uint64_t AckServerApplication::GetMeanDataRate(void) {
	return m_meanDataRate;
}

void AckServerApplication::Reset(void) {
	ResetStartTime();
	ResetTotalPacketsReceived();
	ResetTotalBytesReceived();
	ResetDataRate();
}

void AckServerApplication::ResetStartTime(void) {
	m_startTime = Simulator::Now();
}

void AckServerApplication::ResetTotalPacketsReceived(void) {
	m_totalPacketsReceived = 0;
}

void AckServerApplication::ResetTotalBytesReceived(void) {
	m_totalBytesReceived = 0;
}

void AckServerApplication::ResetDataRate(void) {
	m_meanDataRate = 0;
	m_dataRate0 = 0;
	m_dataRate1 = 0;
	m_dataRate2 = 0;
}

void AckServerApplication::StartApplication(void) {
	m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
	InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 9);
	m_socket->Bind(local);
	m_socket->SetRecvCallback(MakeCallback(&AckServerApplication::HandleRead, this));

	m_startTime = Simulator::Now();
	m_lastPacketTime = Simulator::Now();
}

void AckServerApplication::StopApplication(void) {
	m_socket->Close();
}

void AckServerApplication::HandleRead(Ptr<Socket> socket) {
	Ptr<Packet> packet;
	Address from;

	while ((packet = socket->RecvFrom(from))) {
		++m_totalPacketsReceived;
		m_totalBytesReceived += packet->GetSize();

		CalcMeanDataRate(packet);

		// use the UUID as sequence number and send it back as ACK
		PacketSeqHeader h(packet->GetUid());

		Ptr<Packet> reply = Create<Packet>();
		reply->AddHeader(h);
		socket->SendTo(reply, 0, from);
	}
}

uint64_t AckServerApplication::CalcMeanDataRate(Ptr<Packet> currentPacket) {
	// TODO: implement moving average

	m_lastPacketTime = Simulator::Now();

	return 0;
}


NS_OBJECT_ENSURE_REGISTERED(PacketSeqHeader);

PacketSeqHeader::~PacketSeqHeader() {}

PacketSeqHeader::PacketSeqHeader(uint64_t seq)
	: m_seq(seq) {}

uint32_t PacketSeqHeader::Deserialize(Buffer::Iterator start) {
	m_seq = start.ReadU64();

	return 8; // read 8 bytes
}

uint32_t PacketSeqHeader::GetSerializedSize(void) const {
	return 8;
}

void PacketSeqHeader::Print(std::ostream &os) const {
	os << m_seq;
}

void PacketSeqHeader::Serialize(Buffer::Iterator start) const {
	start.WriteU64(m_seq);
}

TypeId PacketSeqHeader::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::PacketSeqHeader")
		.SetParent<Header>()
		.SetGroupName("Headers")
		.AddConstructor<PacketSeqHeader>()
		;

	return tid;
}

TypeId PacketSeqHeader::GetInstanceTypeId(void) const {
	return GetTypeId();
}
