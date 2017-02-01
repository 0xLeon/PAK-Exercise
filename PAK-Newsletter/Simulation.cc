#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-star.h"
#include "ns3/applications-module.h"
#include "ns3/int64x64-128.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PakNewsletter");

int main(int argc, char *argv[]) {
	LogComponentEnable("PakNewsletter", LOG_LEVEL_INFO);

	uint64_t nReceiversPerServer = 100;
	uint64_t rtt = 50;
	uint64_t nTcpConnections = 30;
	uint32_t runtime = 300;

	CommandLine cmd;
	cmd.AddValue("rps", "Number of receivers per server", nReceiversPerServer);
	cmd.AddValue("rtt", "RTT value", rtt);
	cmd.AddValue("tcp-count", "Number of simultaneus TCP connections", nTcpConnections);
	cmd.AddValue("runtime", "Length in seconds to run this simulation", runtime);
	cmd.Parse(argc, argv);


	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
	p2p.SetChannelAttribute("Delay", StringValue("50ms")); // TODO: dynamic RTT here


	NodeContainer nodes;
	nodes.Create(2);


	NetDeviceContainer devices;
	devices = p2p.Install(nodes);


	InternetStackHelper stack;
	stack.Install(nodes);


	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");

	Ipv4InterfaceContainer interfaces = address.Assign(devices);


	EmailNewsletterHelper enHelper("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), 2048));
	ApplicationContainer enServerApps;

	PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), 2048));
	ApplicationContainer sink = sinkHelper.Install(nodes.Get(1));

	for (uint32_t i = 0; i < nTcpConnections; ++i) {
		enServerApps.Add(enHelper.Install(nodes.Get(0)));
	}


	sink.Start(Seconds(0));
	sink.Stop(Seconds(runtime));

	enServerApps.Start(Seconds(10));
	enServerApps.Stop(Seconds(runtime));


	p2p.EnablePcapAll("pak-newsletter");


	Simulator::Stop(Seconds(runtime));
	Simulator::Run();
	Simulator::Destroy();


	return 0;
}
