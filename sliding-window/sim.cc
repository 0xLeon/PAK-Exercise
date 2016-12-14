#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-star.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/int64x64-128.h"

#include "ack-server.h"
#include "sliding-client.h"

#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PakSliding");

int main (int argc, char *argv[]) {
	/*
	 * A (0) ------- R1 (1) ------- R2 (2) ------- R3 (3) ------- R4 (4) ------- B (5)
	 */

	LogComponentEnable("PakSliding", LOG_LEVEL_INFO);
	
	uint64_t packetSize = 1400;
	uint64_t windowSize = 25000;
	uint64_t runtime = 60;
	
	CommandLine cmd;
	cmd.AddValue("runtime", "Length in seconds to run this simulation", runtime);
	cmd.AddValue("packetSize", "Size of one packet in byte", packetSize);
	cmd.AddValue("windowSize", "Window size in byte", windowSize);
	cmd.Parse(argc, argv);
	
	NodeContainer nodes;
	nodes.Create(6);

	NodeContainer nAR1  = NodeContainer(nodes.Get(0), nodes.Get(1));
	NodeContainer nR1R2 = NodeContainer(nodes.Get(1), nodes.Get(2));
	NodeContainer nR2R3 = NodeContainer(nodes.Get(2), nodes.Get(3));
	NodeContainer nR3R4 = NodeContainer(nodes.Get(3), nodes.Get(4));
	NodeContainer nR4B  = NodeContainer(nodes.Get(4), nodes.Get(5));

	InternetStackHelper internet;
	internet.Install(nodes);


	PointToPointHelper p2p;
	p2p.SetChannelAttribute("Delay", StringValue("4ms"));

	p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
	NetDeviceContainer dAR1 = p2p.Install(nAR1);

	p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
	NetDeviceContainer dR1R2 = p2p.Install(nR1R2);

	p2p.SetDeviceAttribute("DataRate", StringValue("3Mbps"));
	NetDeviceContainer dR2R3 = p2p.Install(nR2R3);
	NetDeviceContainer dR3R4 = p2p.Install(nR3R4);

	p2p.SetDeviceAttribute("DataRate", StringValue("6Mbps"));
	NetDeviceContainer dR4B = p2p.Install(nR4B);


	Ipv4AddressHelper ipv4;

	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer iAR1 = ipv4.Assign(dAR1);

	ipv4.SetBase("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer iR1R2 = ipv4.Assign(dR1R2);

	ipv4.SetBase("10.1.3.0", "255.255.255.0");
	Ipv4InterfaceContainer iR2R3 = ipv4.Assign(dR2R3);

	ipv4.SetBase("10.1.4.0", "255.255.255.0");
	Ipv4InterfaceContainer iR3R4 = ipv4.Assign(dR3R4);

	ipv4.SetBase("10.1.5.0", "255.255.255.0");
	Ipv4InterfaceContainer iR4B = ipv4.Assign(dR4B);


	Ipv4GlobalRoutingHelper::PopulateRoutingTables();


	Ptr<AckServerApplication> ackSrv = Create<AckServerApplication>();
	nodes.Get(5)->AddApplication(ackSrv);
	ApplicationContainer ackSrvAppContainer(ackSrv);
	
	Ptr<SlidingClient> sldCln = CreateObjectWithAttributes<SlidingClient>(
		"PacketSize",	UintegerValue(packetSize),
		"WindowSize",	UintegerValue(windowSize),
		"Remote",	AddressValue(InetSocketAddress(iR4B.GetAddress(1), 9)),
		"DataRate",	DataRateValue(DataRate("10Mbps"))
	);
	nodes.Get(0)->AddApplication(sldCln);
	ApplicationContainer sldClnAppContainer(sldCln);

	ackSrvAppContainer.Start(Seconds(0));
	ackSrvAppContainer.Stop(Seconds(runtime + 10));
	sldClnAppContainer.Start(Seconds(0));
	sldClnAppContainer.Stop(Seconds(runtime));

	p2p.EnablePcapAll("pak-sliding");

	Simulator::Stop(Seconds(runtime + 10));
	Simulator::Run();

	std::cout << std::fixed;
	std::cout << std::endl << std::endl;
	std::cout << "Simulation run time: " << runtime << "s" << std::endl;
	std::cout << "Total packets sent: " << sldCln->GetSentPackets() << std::endl;
	std::cout << "Total packets received: " << ackSrv->GetTotalPacketsReceived() << std::endl;
	std::cout << "Total packets acknowledged: " << sldCln->GetAckedPackets() << std::endl;
	std::cout << "Total bytes received: " << ackSrv->GetTotalBytesReceived() << " Byte" << std::endl;
	std::cout << "Server run time: " << (Simulator::Now() - ackSrv->GetStartTime()).To(Time::S).GetDouble() << "s" << std::endl;
	std::cout << "Total mean data rate: " << ((ackSrv->GetTotalBytesReceived() * 8.) / (Simulator::Now() - ackSrv->GetStartTime()).To(Time::S).GetDouble()) << " Bit/s" << std::endl;


	Simulator::Destroy();

	return 0;
}
