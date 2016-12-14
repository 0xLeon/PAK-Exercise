#ifndef SLIDING_CLIENT_H
#define SLIDING_CLIENT_H

#include <ns3/core-module.h>
#include <ns3/application.h>
#include <ns3/socket.h>

#include "ns3/address.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"

#include <map>

namespace ns3 {

class Address;
class Socket;
class SlidingClient;

class SlidingClientPacketRecord : public SimpleRefCount<SlidingClientPacketRecord> {
friend class SlidingClient;
public:
	SlidingClientPacketRecord(Ptr<Packet> packet, uint64_t sentBytes, Time timeoutTime, SlidingClient* sClient);
	
	void Timeout();

private:
	Ptr<Packet>	m_packet;
	uint64_t	m_sentBytes;
	SlidingClient*	m_sClient;
	EventId		m_eventId;
};

class SlidingClient : public Application {
friend class SlidingClientPacketRecord;
public:
	static TypeId GetTypeId(void);
	
	SlidingClient();
	virtual ~SlidingClient();
	
	void SetPacketSize(uint64_t packetSize);
	void SetWindowSize(uint64_t windowSize);
	
	Ptr<Socket> GetSocket(void) const;

	inline uint64_t GetAckedPackets() {
		return m_ackedPackets;
	}

	inline uint64_t GetSentPackets() {
		return m_sentPackets;
	}

protected:
	virtual void DoDispose(void);

private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);
	
	Ptr<Socket>		m_socket;
	Address			m_peer;
	uint64_t		m_packetSize;
	uint64_t		m_windowSize;
	DataRate		m_dataRate;
	
	uint64_t		m_remaining;
	volatile uint64_t	m_windowFillLevel;
	
	EventId			m_sendEventId;
	uint64_t		m_nextSendSize;

	std::map<uint64_t, Ptr<SlidingClientPacketRecord>> m_unackedPackets;

	uint64_t		m_sentPackets;
	uint64_t		m_ackedPackets;
	
	TracedCallback<Ptr<const Packet>> m_txTrace;

private:
	void ScheduleNextPacket(uint64_t sendSize);
	void SendPacket();
	void BufferAvailableCb(Ptr<Socket>, uint32_t);
	void AckAvailableCb(Ptr<Socket> socket);
};

}

#endif /* SLIDING_CLIENT_H */
