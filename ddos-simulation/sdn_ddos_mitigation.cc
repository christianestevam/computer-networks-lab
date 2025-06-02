#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include <iostream>

using namespace ns3;

// Função para configurar o tráfego normal (HTTP simulado)
void SetupNormalTraffic(NodeContainer& clients, Ipv4Address serverAddress, uint16_t port) {
    OnOffHelper normal("ns3::TcpSocketFactory", Address(InetSocketAddress(serverAddress, port)));
    normal.SetConstantRate(DataRate("1Mbps")); // 1Mbps por cliente
    normal.SetAttribute("PacketSize", UintegerValue(1500));

    ApplicationContainer normalApps = normal.Install(clients);
    normalApps.Start(Seconds(1.0));
    normalApps.Stop(Seconds(10.0));
    std::cout << "Configurado tráfego normal para " << clients.GetN() << " clientes\n";
    std::cout.flush();
}

// Função para configurar o tráfego malicioso (ataque DDoS simulado)
void SetupMaliciousTraffic(NodeContainer& attackers, Ipv4Address serverAddress, uint16_t port) {
    OnOffHelper attack("ns3::UdpSocketFactory", Address(InetSocketAddress(serverAddress, port)));
    attack.SetConstantRate(DataRate("50Mbps")); // 50Mbps por atacante
    attack.SetAttribute("PacketSize", UintegerValue(1000));

    ApplicationContainer attackApps = attack.Install(attackers);
    attackApps.Start(Seconds(2.0)); // Ataque começa um pouco depois
    attackApps.Stop(Seconds(3.0)); // Ataque para após 3 segundos, simulando mitigação
    std::cout << "Configurado tráfego malicioso para " << attackers.GetN() << " atacantes\n";
    std::cout.flush();
}

// Função para monitoramento periódico de métricas
void MonitorMetrics(Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier, double interval) {
    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    std::cout << "Monitorando métricas no tempo: " << Simulator::Now().GetSeconds() << "s\n";
    if (stats.empty()) {
        std::cout << "Nenhum fluxo capturado!\n";
    }

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->second.rxPackets > 0) {
            double avgDelay = it->second.delaySum.GetSeconds() / it->second.rxPackets;
            double throughput = (it->second.rxBytes * 8.0) / (it->second.timeLastRxPacket.GetSeconds() - it->second.timeFirstRxPacket.GetSeconds()) / 1e6; // Mbps
            double lossRate = (it->second.txPackets - it->second.rxPackets) * 100.0 / it->second.txPackets;

            Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
            std::cout << "Tempo: " << Simulator::Now().GetSeconds() << "s - ";
            std::cout << "Fluxo de " << t.sourceAddress << " para " << t.destinationAddress << ":\n";
            std::cout << "  Latência Média: " << avgDelay << "s\n";
            std::cout << "  Throughput: " << throughput << " Mbps\n";
            std::cout << "  Taxa de Perda: " << lossRate << "%\n";
        }
    }
    std::cout.flush();

    // Reagenda o monitoramento
    if (Simulator::Now().GetSeconds() < 10.0) {
        Simulator::Schedule(Seconds(interval), &MonitorMetrics, monitor, classifier, interval);
    }
}

int main(int argc, char *argv[]) {
    // Criação dos nós
    NodeContainer switches, normalClients, attackers, server;
    switches.Create(2); // 2 switches
    normalClients.Create(3); // 3 clientes normais
    attackers.Create(2); // 2 clientes maliciosos
    server.Create(1); // 1 servidor

    // Configuração de mobilidade
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.InstallAll();

    // Posicionamento dos nós
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0, 0, 0));   // Switch 1
    positionAlloc->Add(Vector(50, 0, 0));  // Switch 2
    for (int i = 0; i < 3; i++) {
        positionAlloc->Add(Vector(10 + i*10, 10, 0)); // Clientes normais
    }
    positionAlloc->Add(Vector(10, 20, 0)); // Atacante 1
    positionAlloc->Add(Vector(20, 20, 0)); // Atacante 2
    positionAlloc->Add(Vector(100, 0, 0)); // Servidor
    mobility.SetPositionAllocator(positionAlloc);
    mobility.InstallAll();

    // Conexão entre switches (Switch1 <-> Switch2)
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer link = p2p.Install(switches.Get(0), switches.Get(1));

    // Conexão dos clientes e atacantes ao Switch 1
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));

    NodeContainer clientNodes;
    clientNodes.Add(switches.Get(0));
    clientNodes.Add(normalClients);
    clientNodes.Add(attackers);
    NetDeviceContainer clientDevs = csma.Install(clientNodes);

    // Conexão do servidor ao Switch 2
    NodeContainer serverNodes;
    serverNodes.Add(switches.Get(1));
    serverNodes.Add(server);
    NetDeviceContainer serverDevs = csma.Install(serverNodes);

    // Configuração da pilha de protocolos
    InternetStackHelper stack;
    stack.InstallAll();

    // Atribuição de endereços IP
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer clientIfs = address.Assign(clientDevs);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer serverIfs = address.Assign(serverDevs);
    address.SetBase("10.1.3.0", "255.255.255.0");
    address.Assign(link);

    // Configuração de roteamento
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configuração do tráfego
    uint16_t port = 4000;
    SetupNormalTraffic(normalClients, serverIfs.GetAddress(1), port);
    SetupMaliciousTraffic(attackers, serverIfs.GetAddress(1), port);

    // Configuração do sink no servidor
    PacketSinkHelper sink("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    ApplicationContainer sinkApps = sink.Install(server);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(11.0));

    // Configuração do FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    // Agenda o monitoramento periódico (a cada 1 segundo)
    Simulator::Schedule(Seconds(1.0), &MonitorMetrics, monitor, classifier, 1.0);

    // Executa a simulação
    Simulator::Stop(Seconds(11.0));
    Simulator::Run();

    // Exibe métricas finais
    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    std::cout << "Métricas Finais:\n";
    if (stats.empty()) {
        std::cout << "Nenhum fluxo capturado no final da simulação!\n";
    }
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->second.rxPackets > 0) {
            double avgDelay = it->second.delaySum.GetSeconds() / it->second.rxPackets;
            double throughput = (it->second.rxBytes * 8.0) / (it->second.timeLastRxPacket.GetSeconds() - it->second.timeFirstRxPacket.GetSeconds()) / 1e6; // Mbps
            double lossRate = (it->second.txPackets - it->second.rxPackets) * 100.0 / it->second.txPackets;

            Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
            std::cout << "Fluxo de " << t.sourceAddress << " para " << t.destinationAddress << ":\n";
            std::cout << "  Latência Média: " << avgDelay << "s\n";
            std::cout << "  Throughput: " << throughput << " Mbps\n";
            std::cout << "  Taxa de Perda: " << lossRate << "%\n";
        }
    }
    std::cout.flush();

    Simulator::Destroy();
    return 0;
}