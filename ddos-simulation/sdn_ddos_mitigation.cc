/*
 * Introdução à Simulação
 * 
 * Este código implementa uma simulação de mitigação de ataques DDoS (Distributed Denial of Service) no simulador de redes NS-3.
 * A simulação modela uma rede com dois switches, três clientes normais, dois clientes maliciosos (atacantes), e um servidor.
 * 
 * Objetivo:
 * - Simular um ataque DDoS em uma rede e observar seu impacto no desempenho (latência, throughput, taxa de perda).
 * - Mitigar o ataque de forma simplificada, parando os aplicativos maliciosos após um tempo específico.
 * - Coletar métricas periódicas e finais para avaliar o desempenho da rede antes e após a mitigação.
 * 
 * Topologia da Rede:
 * - Dois switches conectados por um enlace ponto-a-ponto (1 Gbps, 2 ms de atraso).
 * - Três clientes normais (10.1.1.2 a 10.1.1.4) e dois clientes maliciosos (10.1.1.5 e 10.1.1.6) conectados ao Switch 1 via enlace CSMA (100 Mbps, 1 ms de atraso).
 * - Um servidor (10.1.2.2) conectado ao Switch 2 via enlace CSMA (100 Mbps, 1 ms de atraso).
 * 
 * Tráfego:
 * - Clientes normais: Tráfego HTTP simulado (TCP, 1 Mbps cada), ativo de 1 a 10 segundos.
 * - Clientes maliciosos: Ataque DDoS simulado (UDP, 50 Mbps cada), ativo de 2 a 3 segundos.
 * 
 * Mitigação:
 * - O ataque DDoS é mitigado parando os aplicativos maliciosos após 3 segundos, simulando um bloqueio de tráfego malicioso.
 * 
 * Métricas:
 * - Latência média, throughput (Mbps), e taxa de perda (%) são coletados a cada segundo e no final da simulação usando o FlowMonitor.
 * 
 * Resultados Esperados:
 * - Antes dos 3 segundos: Alta taxa de perda (~50%), throughput dos clientes normais <0.5 Mbps, latência >100ms devido ao ataque DDoS.
 * - Após os 3 segundos: Throughput ~1 Mbps, latência ~10ms, taxa de perda <1%, indicando recuperação após a mitigação.
 */

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
// - clients: Contêiner de nós que representam os clientes normais
// - serverAddress: Endereço IP do servidor que receberá o tráfego
// - port: Porta usada para a comunicação
void SetupNormalTraffic(NodeContainer& clients, Ipv4Address serverAddress, uint16_t port) {
    // Configura um aplicativo OnOff para simular tráfego HTTP usando TCP
    OnOffHelper normal("ns3::TcpSocketFactory", Address(InetSocketAddress(serverAddress, port)));
    normal.SetConstantRate(DataRate("1Mbps")); // Define a taxa de envio de 1 Mbps por cliente
    normal.SetAttribute("PacketSize", UintegerValue(1500)); // Define o tamanho do pacote como 1500 bytes

    // Instala o aplicativo nos clientes e define o tempo de início e parada
    ApplicationContainer normalApps = normal.Install(clients);
    normalApps.Start(Seconds(1.0)); // Inicia o tráfego em t=1s
    normalApps.Stop(Seconds(10.0)); // Para o tráfego em t=10s
    std::cout << "Configurado tráfego normal para " << clients.GetN() << " clientes\n";
    std::cout.flush(); // Garante que a mensagem seja exibida imediatamente no terminal
}

// Função para configurar o tráfego malicioso (ataque DDoS simulado)
// - attackers: Contêiner de nós que representam os clientes maliciosos
// - serverAddress: Endereço IP do servidor que será alvo do ataque
// - port: Porta usada para a comunicação
void SetupMaliciousTraffic(NodeContainer& attackers, Ipv4Address serverAddress, uint16_t port) {
    // Configura um aplicativo OnOff para simular um ataque DDoS usando UDP
    OnOffHelper attack("ns3::UdpSocketFactory", Address(InetSocketAddress(serverAddress, port)));
    attack.SetConstantRate(DataRate("50Mbps")); // Define a taxa de envio de 50 Mbps por atacante
    attack.SetAttribute("PacketSize", UintegerValue(1000)); // Define o tamanho do pacote como 1000 bytes

    // Instala o aplicativo nos atacantes e define o tempo de início e parada
    ApplicationContainer attackApps = attack.Install(attackers);
    attackApps.Start(Seconds(2.0)); // Inicia o ataque em t=2s
    attackApps.Stop(Seconds(3.0)); // Para o ataque em t=3s, simulando mitigação
    std::cout << "Configurado tráfego malicioso para " << attackers.GetN() << " atacantes\n";
    std::cout.flush(); // Garante que a mensagem seja exibida imediatamente no terminal
}

// Função para monitoramento periódico de métricas
// - monitor: Objeto FlowMonitor para coletar estatísticas de fluxo
// - classifier: Classificador para mapear fluxos a endereços IP
// - interval: Intervalo de tempo entre monitoramentos (em segundos)
void MonitorMetrics(Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier, double interval) {
    // Verifica pacotes perdidos e obtém as estatísticas de fluxo
    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    // Exibe o tempo atual da simulação
    std::cout << "Monitorando métricas no tempo: " << Simulator::Now().GetSeconds() << "s\n";
    if (stats.empty()) {
        std::cout << "Nenhum fluxo capturado!\n"; // Mensagem de depuração caso não haja fluxos
    }

    // Itera sobre os fluxos capturados e calcula métricas
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->second.rxPackets > 0) { // Verifica se há pacotes recebidos no fluxo
            double avgDelay = it->second.delaySum.GetSeconds() / it->second.rxPackets; // Calcula a latência média
            double throughput = (it->second.rxBytes * 8.0) / (it->second.timeLastRxPacket.GetSeconds() - it->second.timeFirstRxPacket.GetSeconds()) / 1e6; // Calcula o throughput em Mbps
            double lossRate = (it->second.txPackets - it->second.rxPackets) * 100.0 / it->second.txPackets; // Calcula a taxa de perda em %

            // Mapeia o fluxo para os endereços de origem e destino
            Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
            std::cout << "Tempo: " << Simulator::Now().GetSeconds() << "s - ";
            std::cout << "Fluxo de " << t.sourceAddress << " para " << t.destinationAddress << ":\n";
            std::cout << "  Latência Média: " << avgDelay << "s\n";
            std::cout << "  Throughput: " << throughput << " Mbps\n";
            std::cout << "  Taxa de Perda: " << lossRate << "%\n";
        }
    }
    std::cout.flush(); // Garante que as métricas sejam exibidas imediatamente

    // Reagenda o monitoramento se a simulação não terminou
    if (Simulator::Now().GetSeconds() < 10.0) {
        Simulator::Schedule(Seconds(interval), &MonitorMetrics, monitor, classifier, interval);
    }
}

int main(int argc, char *argv[]) {
    // Criação dos nós da simulação
    NodeContainer switches, normalClients, attackers, server;
    switches.Create(2); // Cria 2 switches (Switch 1 e Switch 2)
    normalClients.Create(3); // Cria 3 clientes normais
    attackers.Create(2); // Cria 2 clientes maliciosos (atacantes DDoS)
    server.Create(1); // Cria 1 servidor

    // Configuração de mobilidade para posicionar os nós
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel"); // Modelo de posição fixa
    mobility.InstallAll();

    // Posicionamento dos nós em coordenadas fixas
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
    // - Usa enlace ponto-a-ponto com 1 Gbps e 2 ms de atraso
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer link = p2p.Install(switches.Get(0), switches.Get(1));

    // Conexão dos clientes e atacantes ao Switch 1
    // - Usa enlace CSMA com 100 Mbps e 1 ms de atraso
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("1ms"));

    NodeContainer clientNodes;
    clientNodes.Add(switches.Get(0)); // Adiciona o Switch 1 ao contêiner
    clientNodes.Add(normalClients); // Adiciona os clientes normais
    clientNodes.Add(attackers); // Adiciona os atacantes
    NetDeviceContainer clientDevs = csma.Install(clientNodes);

    // Conexão do servidor ao Switch 2
    // - Usa enlace CSMA com 100 Mbps e 1 ms de atraso
    NodeContainer serverNodes;
    serverNodes.Add(switches.Get(1)); // Adiciona o Switch 2 ao contêiner
    serverNodes.Add(server); // Adiciona o servidor
    NetDeviceContainer serverDevs = csma.Install(serverNodes);

    // Configuração da pilha de protocolos (TCP/IP)
    InternetStackHelper stack;
    stack.InstallAll();

    // Atribuição de endereços IP
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer clientIfs = address.Assign(clientDevs); // IPs para clientes e Switch 1
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer serverIfs = address.Assign(serverDevs); // IPs para servidor e Switch 2
    address.SetBase("10.1.3.0", "255.255.255.0");
    address.Assign(link); // IPs para o enlace entre switches

    // Configuração de roteamento para permitir comunicação entre sub-redes
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configuração do tráfego
    uint16_t port = 4000;
    SetupNormalTraffic(normalClients, serverIfs.GetAddress(1), port); // Configura tráfego normal
    SetupMaliciousTraffic(attackers, serverIfs.GetAddress(1), port); // Configura tráfego malicioso

    // Configuração do sink no servidor para receber pacotes
    PacketSinkHelper sink("ns3::TcpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    ApplicationContainer sinkApps = sink.Install(server);
    sinkApps.Start(Seconds(0.0)); // Inicia o sink em t=0s
    sinkApps.Stop(Seconds(11.0)); // Para o sink em t=11s

    // Configuração do FlowMonitor para coletar métricas de fluxo
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    // Agenda o monitoramento periódico (a cada 1 segundo)
    Simulator::Schedule(Seconds(1.0), &MonitorMetrics, monitor, classifier, 1.0);

    // Executa a simulação por 11 segundos
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

    // Finaliza a simulação e libera recursos
    Simulator::Destroy();
    return 0;
}