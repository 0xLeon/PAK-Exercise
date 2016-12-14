#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/header.h"
#include "ns3/simulator.h"

#ifndef ACK_SERVER_APPLICATION_H
#define ACK_SERVER_APPLICATION_H

namespace ns3 {

	class AckServerApplication : public Application {
	public:
		static TypeId GetTypeId(void);

		AckServerApplication();
		virtual ~AckServerApplication();

		Time GetStartTime(void);
		uint64_t GetTotalPacketsReceived(void);
		uint64_t GetTotalBytesReceived(void);
		uint64_t GetMeanDataRate(void);

		void Reset(void);
		void ResetStartTime(void);
		void ResetTotalPacketsReceived(void);
		void ResetTotalBytesReceived(void);
		void ResetDataRate(void);
	private:
		virtual void StartApplication(void);
		virtual void StopApplication(void);

		void HandleRead(Ptr<Socket> socket);
		uint64_t CalcMeanDataRate(Ptr<Packet> currentPacket);

		Ptr<Socket>     m_socket;
		Time		m_startTime;
		uint64_t        m_totalPacketsReceived;
		uint64_t	m_totalBytesReceived;

		Time		m_lastPacketTime;
		uint64_t	m_meanDataRate;
		uint64_t	m_dataRate0;
		uint64_t	m_dataRate1;
		uint64_t	m_dataRate2;
	};

	class PacketSeqHeader : public Header {
	public:
		PacketSeqHeader() {}
		PacketSeqHeader(uint64_t seq);
		virtual ~PacketSeqHeader();
		virtual uint32_t Deserialize(Buffer::Iterator start);
		virtual uint32_t GetSerializedSize(void) const;
		virtual void Print(std::ostream &os) const;
		virtual void Serialize(Buffer::Iterator start) const;

		virtual uint64_t GetSeq() {
			return m_seq;
		}

		virtual void SetSeq(uint64_t seq) {
			m_seq = seq;
		}

		static TypeId GetTypeId(void);
		virtual TypeId GetInstanceTypeId(void) const;
	private:
		uint64_t m_seq;
	};
}
#endif
