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


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PakNscReno");

int main(int argc, char* argv[]) {
	/*
	* A (0) ----- R (1) ----- B (2)
	*/

	LogComponentEnable("PakNscReno", LOG_LEVEL_INFO);

	uint32_t runtime = 120;
	bool useNsc = false;
	bool limitQueue = false;
	std::string tcpCong = "reno";

	CommandLine cmd;
	cmd.AddValue("runtime", "Length in seconds to run this simulation", runtime);
	cmd.AddValue("useNsc", "Enable if NSC shoukd be used as TCP stack for nodes A and B", useNsc);
	cmd.AddValue("limitQueue", "Limit the bottleneck queue to 20 kB", limitQueue);
	cmd.AddValue("tcpCong", "Congestion control algorithm", tcpCong);
	cmd.Parse(argc, argv);


	NodeContainer nodes;
	nodes.Create(3);

	NodeContainer nAR = NodeContainer(nodes.Get(0), nodes.Get(1));
	NodeContainer nRB = NodeContainer(nodes.Get(1), nodes.Get(2));


	InternetStackHelper inetStack;

	inetStack.Install(nodes.Get(1));

	if (useNsc) {
		inetStack.SetTcp("ns3::NscTcpL4Protocol", "Library", StringValue("liblinux2.6.26.so"));
	}

	inetStack.Install(nodes.Get(0));
	inetStack.Install(nodes.Get(2));

	if (useNsc) {
		Config::Set("/NodeList/0/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue(tcpCong));
		Config::Set("/NodeList/2/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue(tcpCong));
	}


	PointToPointHelper p2p;
	p2p.SetChannelAttribute("Delay", TimeValue(MilliSeconds(50)));

	p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
	NetDeviceContainer dAR = p2p.Install(nAR);

	p2p.SetDeviceAttribute("DataRate", DataRateValue(DataRate("10Mbps")));
	NetDeviceContainer dRB = p2p.Install(nRB);


	if (limitQueue) {
		PointToPointNetDevice* p2pR = static_cast<PointToPointNetDevice*>(PeekPointer(dRB.Get(0)));
		DropTailQueue* qR = static_cast<DropTailQueue*>(PeekPointer(p2pR->GetQueue()));

		qR->SetAttribute("MaxBytes", UintegerValue(20000));
		qR->SetMode(DropTailQueue::QUEUE_MODE_BYTES);
	}


	Ipv4AddressHelper ipv4;

	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer iAR = ipv4.Assign(dAR);

	ipv4.SetBase("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer iRB = ipv4.Assign(dRB);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();


	PacketSinkHelper sinks("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 8080));
	ApplicationContainer sinkApps = sinks.Install(nodes.Get(2));

	sinkApps.Start(Seconds(0));
	sinkApps.Stop(Seconds(runtime + 30));


	OnOffHelper onOff("ns3::TcpSocketFactory", Address(InetSocketAddress(iRB.GetAddress(1), 8080)));
	onOff.SetConstantRate(DataRate("100Mbps"));
	ApplicationContainer onOffApps = onOff.Install(nodes.Get(0));

	onOffApps.Start(Seconds(0));
	onOffApps.Stop(Seconds(runtime));


	std::string filePath = "scratch/pak-nsc-reno/pak-nsc-reno";
	FlowMonitorHelper flowMon;
	flowMon.InstallAll();

	if (useNsc) {
		filePath = filePath + "_nscOn";
	}
	else {
		filePath = filePath + "_nscOff";
	}

	if (limitQueue) {
		filePath = filePath + "_limitedQueue";
	}
	else {
		filePath = filePath + "_unlimitedQueue";
	}

	p2p.EnablePcap(filePath, NodeContainer(nodes.Get(0)), false);


	Simulator::Stop(Seconds(runtime + 60));
	Simulator::Run();


	flowMon.SerializeToXmlFile(filePath + ".flowmon", false, false);

	Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(0));

	std::cout << std::endl;
	std::cout << "Received bytes:\t" << sink->GetTotalRx() << std::endl;
	std::cout << std::endl;


	Simulator::Destroy();


	return 0;
}
