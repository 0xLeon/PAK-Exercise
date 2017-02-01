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

// Network topology
//
//       n0 ----------- n1
//            10 Mbps
//             10 ms

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include <string>
#include <fstream>

#define	SWING_IN	30
#define	COOL_DOWN	30


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PakTcpModel");

volatile uint64_t sentCnt = 0;
volatile uint64_t dropCnt = 0;

void enqueueCb() {
	sentCnt++;
}

void dropCb() {
	dropCnt++;
}

void swingInReset() {
	sentCnt = 0;
	dropCnt = 0;
}

int main(int argc, char *argv[]) {
	std::string datarate = "10Mbps";
	std::string delay = "10ms";
	uint32_t runtime = 300;

	CommandLine cmd;
	cmd.AddValue("datarate", "Link datarate value", datarate);
	cmd.AddValue("delay", "Link delay value", delay);
	cmd.AddValue("runtime", "Simulation run time", runtime);
	cmd.Parse(argc, argv);


	NS_LOG_INFO("Create nodes.");
	NodeContainer nodes;
	nodes.Create(2);


	NS_LOG_INFO("Create channels.");
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute("DataRate", StringValue(datarate));
	pointToPoint.SetChannelAttribute("Delay", StringValue(delay));

	NetDeviceContainer devices;
	devices = pointToPoint.Install(nodes);


	InternetStackHelper internet;
	internet.Install(nodes);

	
	NS_LOG_INFO("Assign IP Addresses.");
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i = ipv4.Assign(devices);


	NS_LOG_INFO("Create Applications.");

	
	uint16_t port = 9;

	BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(i.GetAddress(1), port));
	ApplicationContainer sourceApps = source.Install(nodes.Get(0));
	sourceApps.Start(Seconds(0));
	sourceApps.Stop(Seconds(runtime + SWING_IN));

	
	PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
	ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
	sinkApps.Start(Seconds(0));
	sinkApps.Stop(Seconds(runtime + SWING_IN + COOL_DOWN));

	
	pointToPoint.EnablePcapAll("pak-tcp-model", false);


	Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/Enqueue", MakeCallback(&enqueueCb));
	Config::ConnectWithoutContext("/NodeList/1/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback(&dropCb));


	Simulator::Schedule(Seconds(SWING_IN), &swingInReset);


	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(runtime + SWING_IN + COOL_DOWN));
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Done.");


	// Ptr<BulkSendApplication> bulk1 = DynamicCast<BulkSendApplication>(sourceApps.Get(0));
	// Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
	// std::cout << "Total Bytes Received: " << sink1->GetTotalRx() << std::endl;

	std::cout << "Total packages sent: " << sentCnt << std::endl;
	std::cout << "Total packages received: " << (sentCnt - dropCnt) << std::endl;
	std::cout << "Total packages dropped: " << dropCnt << std::endl;


	return 0;
}
