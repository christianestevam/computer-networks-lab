#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VideoStreamingQoS");

class PacketMarker {
public:
    PacketMarker(uint8_t dscp) : m_dscp(dscp) {}
    
    void Mark(Ptr<const Packet> packet) {  // Alterado para Ptr<const Packet>
        Ptr<Packet> pCopy = packet->Copy(); // Cria uma cópia não-const
        SocketIpTosTag tosTag;
        tosTag.SetTos(m_dscp << 2);
        pCopy->ReplacePacketTag(tosTag);
    }
    
private:
    uint8_t m_dscp;
};

int main(int argc, char *argv[]) {
    Time::SetResolution(Time::NS);
    LogComponentEnable("VideoStreamingQoS", LOG_LEVEL_INFO);

    // Create nodes
    NodeContainer clients, router, server;
    clients.Create(2);
    router.Create(1);
    server.Create(1);

    // Configure point-to-point links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

    // Create network devices
    NetDeviceContainer client1Router = p2p.Install(clients.Get(0), router.Get(0));
    NetDeviceContainer client2Router = p2p.Install(clients.Get(1), router.Get(0));
    NetDeviceContainer routerServer = p2p.Install(router.Get(0), server.Get(0));

    // Install internet stack
    InternetStackHelper stack;
    stack.InstallAll();

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer client1RouterIf = address.Assign(client1Router);
    
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer client2RouterIf = address.Assign(client2Router);
    
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer routerServerIf = address.Assign(routerServer);

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure QoS using FqCoDel
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");

    // Apply to router devices
    NetDeviceContainer routerDevices;
    routerDevices.Add(client1Router.Get(1));
    routerDevices.Add(client2Router.Get(1));
    routerDevices.Add(routerServer.Get(0));
    
    // Uninstall any existing queue discs
    TrafficControlHelper tchUninstall;
    tchUninstall.Uninstall(routerDevices);
    
    // Install new queue disc
    QueueDiscContainer qdiscs = tch.Install(routerDevices);

    // Setup video traffic (UDP - high priority)
    OnOffHelper videoSource("ns3::UdpSocketFactory", 
                          InetSocketAddress(routerServerIf.GetAddress(1), 5000));
    videoSource.SetAttribute("DataRate", DataRateValue(DataRate("2Mbps")));
    videoSource.SetAttribute("PacketSize", UintegerValue(1000));
    
    ApplicationContainer videoApps = videoSource.Install(clients.Get(0));
    videoApps.Start(Seconds(1.0));
    videoApps.Stop(Seconds(10.0));
    
    // Mark video packets with DSCP EF (46)
    PacketMarker videoMarker(46);
    Ptr<OnOffApplication> videoApp = DynamicCast<OnOffApplication>(videoApps.Get(0));
    videoApp->TraceConnectWithoutContext("Tx", MakeCallback(&PacketMarker::Mark, &videoMarker));

    // Setup FTP traffic (TCP - low priority)
    BulkSendHelper ftpSource("ns3::TcpSocketFactory", 
                           InetSocketAddress(routerServerIf.GetAddress(1), 5001));
    ftpSource.SetAttribute("MaxBytes", UintegerValue(0));
    
    ApplicationContainer ftpApps = ftpSource.Install(clients.Get(1));
    ftpApps.Start(Seconds(1.0));
    ftpApps.Stop(Seconds(10.0));
    
    // Mark FTP packets with DSCP BE (0)
    PacketMarker ftpMarker(0);
    Ptr<BulkSendApplication> ftpApp = DynamicCast<BulkSendApplication>(ftpApps.Get(0));
    ftpApp->TraceConnectWithoutContext("Tx", MakeCallback(&PacketMarker::Mark, &ftpMarker));

    // Setup packet sinks
    PacketSinkHelper videoSink("ns3::UdpSocketFactory", 
                             InetSocketAddress(Ipv4Address::GetAny(), 5000));
    PacketSinkHelper ftpSink("ns3::TcpSocketFactory", 
                           InetSocketAddress(Ipv4Address::GetAny(), 5001));
    
    ApplicationContainer sinkApps = videoSink.Install(server.Get(0));
    sinkApps.Add(ftpSink.Install(server.Get(0)));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(11.0));

    // Enable flow monitoring
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    NS_LOG_INFO("Starting simulation...");
    Simulator::Stop(Seconds(11.0));
    Simulator::Run();

    // Analyze results
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
        
        std::cout << "\nFlow " << it->first << " (" << t.sourceAddress << ":" << t.sourcePort 
                  << " -> " << t.destinationAddress << ":" << t.destinationPort << ")\n";
        
        if (it->second.rxPackets > 0) {
            double throughput = it->second.rxBytes * 8.0 / 
                              (it->second.timeLastRxPacket - it->second.timeFirstRxPacket).GetSeconds() / 1e6;
            double avgDelay = it->second.delaySum.GetSeconds() / it->second.rxPackets;
            double lossRate = (it->second.txPackets - it->second.rxPackets) * 100.0 / it->second.txPackets;
            
            std::cout << "  Throughput: " << throughput << " Mbps\n";
            std::cout << "  Average Delay: " << avgDelay * 1000 << " ms\n";
            std::cout << "  Packet Loss Rate: " << lossRate << "%\n";
        }
    }

    Simulator::Destroy();
    NS_LOG_INFO("Simulation completed.");
    return 0;
}