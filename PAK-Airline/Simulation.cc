/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-star.h"
#include "ns3/applications-module.h"
#include "ns3/int64x64-128.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PakAirlineStreaming");

#define	SERVER_ID	0
#define	SERVER_IDX	"0"
#define FIRST_CLIENT_ID	1

int64x64_t sent = 0;
int64x64_t recv = 0;

void SendCb(const Ptr<const Packet>) {
	sent += int64x64_t(1);
}

void RecvCb(const Ptr<const Packet>, const Address&) {
	recv += int64x64_t(1);
}

int main(int argc, char *argv[]) {
	LogComponentEnable("PakAirlineStreaming", LOG_LEVEL_INFO);

	Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(512));
	Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("1Mbps"));

	uint32_t nClients = 30;
	uint32_t runtime = 2400;
	// double errRate = 0.05;

	CommandLine cmd;
	cmd.AddValue("client-count", "Number of streaming clients", nClients);
	cmd.AddValue("runtime", "Length in seconds to run this simulation", runtime);
	cmd.Parse(argc, argv);


	PointToPointHelper p2p;
	p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
	p2p.SetChannelAttribute("Delay", StringValue("3ms"));

	PointToPointStarHelper star(nClients + 1, p2p);


	InternetStackHelper inetStack;
	star.InstallStack(inetStack);


	star.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"));
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();


	OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address());

	Ptr<RandomVariableStream> onTime = CreateObject<UniformRandomVariable>();
	onTime->SetAttribute("Min", DoubleValue(1.0));
	onTime->SetAttribute("Max", DoubleValue(3.0));
	onOffHelper.SetAttribute("OnTime", PointerValue(onTime));

	Ptr<RandomVariableStream> offTime = CreateObject<UniformRandomVariable>();
	offTime->SetAttribute("Min", DoubleValue(1.0));
	offTime->SetAttribute("Max", DoubleValue(40.0));
	onOffHelper.SetAttribute("OffTime", PointerValue(offTime));

	ApplicationContainer serverApps;

	for (uint32_t i = 1; i <= nClients; ++i) {
		AddressValue seatAddress(InetSocketAddress(star.GetSpokeIpv4Address(i), 9));

		onOffHelper.SetAttribute("Remote", seatAddress);
		serverApps.Add(onOffHelper.Install(star.GetSpokeNode(SERVER_ID)));
	}


	PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", Address());
	ApplicationContainer clientApps;

	for (uint32_t i = 1; i <= nClients; ++i) {
		AddressValue seatLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), 9));

		sinkHelper.SetAttribute("Local", seatLocalAddress);
		clientApps.Add(sinkHelper.Install(star.GetSpokeNode(i)));
	}


	serverApps.Start(Seconds(1.0));
	serverApps.Stop(Seconds(runtime));

	clientApps.Start(Seconds(1.0));
	clientApps.Stop(Seconds(50.0 + runtime));


	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx", MakeCallback(&SendCb));
	Config::ConnectWithoutContext("/NodeList/*/ApplicationList/0/$ns3::PacketSink/Rx", MakeCallback(&RecvCb));


	p2p.EnablePcapAll("pak-airline");


	Simulator::Stop(Seconds(100.0 + runtime));
	Simulator::Run();

	Simulator::Destroy();


	int64x64_t del_ratio = int64x64_t(100) - int64x64_t(100) * recv / sent;

	// printf("recv/sent: %lu/%lu\n", recv.GetHigh(), sent.GetHigh());
	// printf("loss ratio: %lu%%\n", 100 - del_ratio.GetHigh());

	printf("%2.2f\n", del_ratio.GetDouble());


	return 0;
}
