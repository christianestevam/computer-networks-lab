#include "ns3/core-module.h"         
#include "ns3/network-module.h"       
#include "ns3/internet-module.h"      
#include "ns3/lr-wpan-module.h"     
#include "ns3/sixlowpan-module.h"     
#include "ns3/mobility-module.h"      
#include "ns3/applications-module.h"  
#include "ns3/flow-monitor-module.h"  
#include <fstream>                 
#include <cstring>                 
#include <iomanip>                   
#include <sstream>                    

using namespace ns3;  // Usa o namespace ns3 para evitar prefixos longos como ns3::Simulator.

// Define um componente de log para a classe MqttPublisher, permitindo logs detalhados no terminal.
NS_LOG_COMPONENT_DEFINE("MqttPublisher");

// Declara um arquivo de log global para o CSV, que será usado para salvar os logs da classe MqttPublisher.
std::ofstream logCsvFile;

// Função para inicializar o arquivo CSV onde os logs serão salvos.
void InitializeLogCsv() {
    // Abre o arquivo logs.csv no diretório /ns-3-dev/output, sobrescrevendo se já existir (std::ios::trunc).
    logCsvFile.open("/ns-3-dev/output/logs.csv", std::ios::trunc);
    if (!logCsvFile.is_open()) {
        // Se o arquivo não puder ser aberto, registra um erro no log do terminal.
        NS_LOG_ERROR("Failed to open logs.csv for writing");
        return;
    }
    // Escreve o cabeçalho do CSV com as colunas: Timestamp, NodeID, Event e Details.
    logCsvFile << "Timestamp,NodeID,Event,Details\n";
}

// Função para escrever uma entrada no arquivo CSV.
void WriteToLogCsv(double timestamp, uint32_t nodeId, const std::string& event, const std::string& details) {
    // Verifica se o arquivo CSV está aberto; se não estiver, registra um erro e retorna.
    if (!logCsvFile.is_open()) {
        NS_LOG_ERROR("logs.csv is not open for writing");
        return;
    }
    std::stringstream ss;  // Cria um stringstream para formatar a linha do CSV.
    // Formata a linha com o timestamp (com 6 casas decimais), o ID do nó, o evento e os detalhes.
    // Os detalhes são colocados entre aspas para lidar com possíveis vírgulas no conteúdo.
    ss << std::fixed << std::setprecision(6) << timestamp << "," 
       << nodeId << "," 
       << event << "," 
       << "\"" << details << "\"\n";
    // Escreve a linha formatada no arquivo CSV.
    logCsvFile << ss.str();
    // Garante que o conteúdo seja escrito no arquivo imediatamente, evitando perda de dados.
    logCsvFile.flush();
}

// Função para fechar o arquivo CSV.
void CloseLogCsv() {
    // Verifica se o arquivo está aberto e, se estiver, fecha-o.
    if (logCsvFile.is_open()) {
        logCsvFile.close();
    }
}

// Declaração da classe MqttPublisher, que herda de Application para simular um publisher MQTT.
class MqttPublisher : public Application {
public:
    MqttPublisher();  // Construtor.
    virtual ~MqttPublisher();  // Destrutor.
    // Função para configurar o aplicativo com endereço de destino, porta e ID do nó.
    void Setup(Ipv6Address address, uint16_t port, uint32_t nodeId);
    // Função para lidar com pacotes recebidos pelo socket.
    void HandleReceive(Ptr<Socket> socket);
    // Métodos para obter o número de pacotes enviados e recebidos.
    uint32_t GetPacketsSent() const { return m_packetCount; }
    uint32_t GetPacketsReceived() const { return m_packetsReceived; }
private:
    // Funções virtuais sobrescritas de Application para iniciar e parar a aplicação.
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    // Função para o gateway enviar uma resposta (ACK) ao nó sensor que enviou um pacote.
    void SendResponse(Ptr<Socket> socket, Ipv6Address sourceAddr, uint16_t sourcePort);
    Ipv6Address m_peerAddress;  // Endereço IPv6 do destino (gateway para sensores, ou "any" para o gateway).
    uint16_t m_port;  // Porta de comunicação (1883 para MQTT).
    Ptr<Socket> m_socket;  // Socket UDP para envio e recebimento de pacotes.
    uint32_t m_nodeId;  // ID do nó (0-9 para sensores, 10 para gateway).
    uint32_t m_packetCount;  // Contador de pacotes enviados pelo nó.
    uint32_t m_packetsReceived;  // Contador de pacotes recebidos pelo nó.
    bool m_running;  // Flag para indicar se a aplicação está ativa.
    static const uint32_t MAX_PACKETS = 10;  // Número máximo de pacotes que cada nó pode enviar.
};

// Construtor: inicializa variáveis com valores padrão.
MqttPublisher::MqttPublisher() 
    : m_port(0), 
      m_socket(0), 
      m_nodeId(0), 
      m_packetCount(0), 
      m_packetsReceived(0), 
      m_running(false) {}

// Destrutor: limpa o socket (seta como nulo para evitar dangling pointers).
MqttPublisher::~MqttPublisher() { 
    m_socket = 0; 
}

// Configura o aplicativo com o endereço de destino, porta e ID do nó.
void MqttPublisher::Setup(Ipv6Address address, uint16_t port, uint32_t nodeId) {
    m_peerAddress = address;  // Endereço do gateway (ou "any" para o gateway).
    m_port = port;  // Porta MQTT (1883).
    m_nodeId = nodeId;  // ID do nó (0-9 para sensores, 10 para gateway).
}

// Função para lidar com pacotes recebidos.
void MqttPublisher::HandleReceive(Ptr<Socket> socket) {
    Ptr<Packet> packet;  // Ponteiro para o pacote recebido.
    Address from;  // Endereço de origem do pacote.
    // Loop para processar todos os pacotes disponíveis no socket.
    while ((packet = socket->RecvFrom(from))) {
        m_packetsReceived++;  // Incrementa o contador de pacotes recebidos.
        // Registra o evento no CSV.
        std::stringstream details;  // Cria um stringstream para formatar os detalhes.
        details << "Size: " << packet->GetSize();  // Detalhe: tamanho do pacote.
        WriteToLogCsv(Simulator::Now().GetSeconds(), m_nodeId, "Received Packet", details.str());
        
        // Se o nó for o gateway (ID 10), envia uma resposta (ACK) ao remetente.
        if (m_nodeId == 10) {
            // Extrai o endereço IPv6 e a porta do remetente.
            Ipv6Address sourceAddr = Inet6SocketAddress::ConvertFrom(from).GetIpv6();
            uint16_t sourcePort = Inet6SocketAddress::ConvertFrom(from).GetPort();
            SendResponse(socket, sourceAddr, sourcePort);  // Envia o ACK.
        }
    }
}

// Função para o gateway enviar uma resposta (ACK) ao nó sensor.
void MqttPublisher::SendResponse(Ptr<Socket> socket, Ipv6Address sourceAddr, uint16_t sourcePort) {
    char response[] = "ACK";  // Mensagem de resposta simples.
    // Cria um pacote com a mensagem "ACK".
    Ptr<Packet> packet = Create<Packet>((uint8_t*)response, strlen(response));
    // Envia o pacote de volta ao remetente.
    socket->SendTo(packet, 0, Inet6SocketAddress(sourceAddr, sourcePort));
    // Registra o evento no CSV.
    std::stringstream details;  // Cria um stringstream para formatar os detalhes.
    details << "To: " << sourceAddr;  // Detalhe: endereço de destino do ACK.
    WriteToLogCsv(Simulator::Now().GetSeconds(), m_nodeId, "Gateway Response", details.str());
}

// Função chamada para iniciar a aplicação (enviar pacotes).
void MqttPublisher::StartApplication(void) {
    // Verifica se o limite de pacotes foi atingido.
    if (m_packetCount >= MAX_PACKETS) {
        // Se atingiu o limite, registra o evento no CSV e retorna.
        WriteToLogCsv(Simulator::Now().GetSeconds(), m_nodeId, "Reached Max Packets", "Max Packets: " + std::to_string(MAX_PACKETS));
        return;
    }

    m_running = true;  // Marca a aplicação como ativa.
    // Se o socket ainda não foi criado, cria e configura.
    if (!m_socket) {
        // Cria um socket UDP para o nó.
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        // Configuração específica para o gateway (nó 10).
        if (m_nodeId == 10) {
            // Vincula o socket à porta 1883 para escutar pacotes (endereço "any").
            if (m_socket->Bind(Inet6SocketAddress(Ipv6Address::GetAny(), m_port)) == -1) {
                NS_LOG_ERROR("Gateway (Node " << m_nodeId << ") failed to bind socket to port " << m_port);
                return;
            }
        } else {
            // Para sensores, vincula o socket a uma porta local qualquer.
            if (m_socket->Bind() == -1) {
                NS_LOG_ERROR("Node " << m_nodeId << " failed to bind socket");
                return;
            }
            // Conecta o socket ao endereço e porta do gateway.
            m_socket->Connect(Inet6SocketAddress(m_peerAddress, m_port));
        }
        // Configura a função de callback para receber pacotes.
        m_socket->SetRecvCallback(MakeCallback(&MqttPublisher::HandleReceive, this));
    }

    // Apenas os sensores (nós 0-9) enviam pacotes.
    if (m_nodeId != 10) {
        // Cria uma mensagem simulada de temperatura e umidade.
        char message[32];
        double temp = 20.0 + (rand() % 100) / 10.0;  // Temperatura entre 20 e 30°C.
        int hum = 50 + rand() % 30;  // Umidade entre 50 e 80%.
        snprintf(message, sizeof(message), "Temp: %.1f C, Hum: %d%%", temp, hum);
        uint32_t messageLength = strlen(message);

        // Aloca um buffer para a mensagem e copia o conteúdo.
        uint8_t* buffer = new uint8_t[messageLength];
        std::memcpy(buffer, message, messageLength);
        // Cria um pacote com a mensagem.
        Ptr<Packet> packet = Create<Packet>(buffer, messageLength);
        delete[] buffer;  // Libera o buffer.

        // Envia o pacote e registra o evento no CSV.
        int result = m_socket->Send(packet);
        std::stringstream details;  // Cria um stringstream para formatar os detalhes.
        details << "Packet: " << m_packetCount << ", Message: " << message << ", Send Result: " << result << ", Length: " << messageLength;
        WriteToLogCsv(Simulator::Now().GetSeconds(), m_nodeId, "Sent Packet", details.str());
        m_packetCount++;  // Incrementa o contador de pacotes enviados.

        // Se ainda não atingiu o limite de pacotes, agenda o próximo envio.
        if (m_running && m_packetCount < MAX_PACKETS) {
            // Intervalo variado entre 0,5 e 2,5 segundos para introduzir aleatoriedade.
            double interval = 0.5 + (rand() % 20) / 10.0;
            Simulator::Schedule(Seconds(interval), &MqttPublisher::StartApplication, this);
        }
    }
}

// Função chamada para parar a aplicação.
void MqttPublisher::StopApplication(void) {
    m_running = false;  // Marca a aplicação como inativa.
    // Fecha o socket, se existente.
    if (m_socket) {
        m_socket->Close();
        m_socket = 0;
    }
}

// Função principal da simulação.
int main(int argc, char *argv[]) {
    // Inicializa o arquivo CSV para logs.
    InitializeLogCsv();

    // Habilita logs detalhados para vários componentes, mas mantém no terminal apenas para depuração.
    LogComponentEnable("MqttPublisher", LOG_LEVEL_INFO);  // Logs da classe MqttPublisher (mantidos para depuração).
    LogComponentEnable("Ipv6AddressHelper", LOG_LEVEL_INFO);  // Logs para configuração de endereços IPv6.
    LogComponentEnable("SixLowPanNetDevice", LOG_LEVEL_INFO);  // Logs para dispositivos 6LoWPAN.
    LogComponentEnable("LrWpanNetDevice", LOG_LEVEL_INFO);  // Logs para dispositivos LrWpan.
    LogComponentEnable("LrWpanMac", LOG_LEVEL_INFO);  // Logs para a camada MAC LrWpan (reduzido para evitar excesso).

    // Cria 11 nós: 10 sensores (0-9) e 1 gateway (10).
    NodeContainer nodes;
    nodes.Create(11);

    // Configura a camada LrWpan (IEEE 802.15.4).
    LrWpanHelper lrWpanHelper;
    // Instala dispositivos LrWpan nos nós.
    NetDeviceContainer devices = lrWpanHelper.Install(nodes);

    // Configura endereços MAC únicos e PAN ID.
    uint16_t panId = 0x1234;  // ID do PAN (Personal Area Network).
    Mac16Address coordShortAddr("00:00");  // Endereço curto do coordenador (nó 0).
    // Loop para configurar cada dispositivo LrWpan.
    for (uint32_t i = 0; i < devices.GetN(); ++i) {
        Ptr<lrwpan::LrWpanNetDevice> lrWpanDev = DynamicCast<lrwpan::LrWpanNetDevice>(devices.Get(i));
        // Define um endereço curto único para cada dispositivo.
        uint8_t macAddr[2] = {0x00, static_cast<uint8_t>(i)};
        Mac16Address shortAddr;
        shortAddr.CopyFrom(macAddr);
        lrWpanDev->SetAddress(shortAddr);
        lrWpanDev->GetMac()->SetPanId(panId);  // Associa ao PAN.
        // Define um endereço estendido único.
        std::ostringstream oss;
        oss << "00:00:00:00:00:00:00:" << std::setfill('0') << std::setw(2) << std::hex << i;
        lrWpanDev->GetMac()->SetExtendedAddress(Mac64Address(oss.str().c_str()));
        // Loga os endereços configurados.
        NS_LOG_INFO("LrWpan Device " << i << " Short MAC: " << lrWpanDev->GetAddress() << ", Extended MAC: " << lrWpanDev->GetMac()->GetExtendedAddress() << ", PAN ID: " << lrWpanDev->GetMac()->GetPanId());
    }

    // Configura o nó 0 como coordenador do PAN.
    Ptr<lrwpan::LrWpanNetDevice> coordDev = DynamicCast<lrwpan::LrWpanNetDevice>(devices.Get(0));
    Ptr<lrwpan::LrWpanMac> coordMac = coordDev->GetMac();
    coordMac->SetShortAddress(coordShortAddr);
    lrwpan::MlmeStartRequestParams startParams;  // Parâmetros para iniciar o PAN.
    startParams.m_PanId = panId;
    startParams.m_bcnOrd = 15;  // Modo sem beacon (non-beacon enabled).
    startParams.m_sfrmOrd = 15;  // Modo sem superframe.
    coordMac->MlmeStartRequest(startParams);
    NS_LOG_INFO("Started PAN on coordinator with Short MAC: " << coordShortAddr << ", PAN ID: " << panId);

    // Agenda a associação dos nós 1-10 ao PAN coordinator.
    for (uint32_t i = 1; i < devices.GetN(); ++i) {
        Ptr<lrwpan::LrWpanNetDevice> lrWpanDev = DynamicCast<lrwpan::LrWpanNetDevice>(devices.Get(i));
        Ptr<lrwpan::LrWpanMac> mac = lrWpanDev->GetMac();
        lrwpan::MlmeAssociateRequestParams assocParams;  // Parâmetros para a associação.
        assocParams.m_coordAddrMode = 2;  // Usa endereço curto para o coordenador.
        assocParams.m_coordShortAddr = coordShortAddr;  // Endereço do coordenador.
        assocParams.m_coordPanId = panId;  // PAN ID.
        assocParams.m_chNum = 11;  // Canal padrão (2,4 GHz).
        assocParams.m_chPage = 0;  // Página de canal padrão.
        assocParams.m_capabilityInfo = 0x80;  // Dispositivo FFD, capaz de ser coordenador.
        // Agenda a associação em tempos escalonados (0,1s * i) para evitar colisões.
        Simulator::Schedule(Seconds(0.1 * i), &lrwpan::LrWpanMac::MlmeAssociateRequest, mac, assocParams);
        NS_LOG_INFO("Scheduled association for MAC: " << mac->GetShortAddress() << " to PAN ID: " << panId << " with coordinator: " << coordShortAddr << " at time " << Seconds(0.1 * i));
    }

    // Habilita captura de pacotes (PCAP) para depuração.
    lrWpanHelper.EnablePcap("lrwpan", devices);

    // Configura a mobilidade dos nós.
    MobilityHelper mobility;
    // Usa um alocador de posições em disco (centro em X=50, Y=50, raio variável até 10).
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue("50.0"),
                                 "Y", StringValue("50.0"),
                                 "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=10]"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");  // Nós fixos.
    mobility.Install(nodes);

    // Instala a pilha de protocolos de internet (IPv6, UDP, etc.).
    InternetStackHelper internet;
    internet.Install(nodes);

    // Configura a camada 6LoWPAN sobre LrWpan.
    SixLowPanHelper sixlowpan;
    NetDeviceContainer sixlowpanDevices = sixlowpan.Install(devices);

    // Loga informações dos dispositivos 6LoWPAN para depuração.
    for (uint32_t i = 0; i < sixlowpanDevices.GetN(); ++i) {
        Ptr<SixLowPanNetDevice> dev = DynamicCast<SixLowPanNetDevice>(sixlowpanDevices.Get(i));
        NS_LOG_INFO("6LoWPAN Device " << i << ": " << dev->GetAddress() << ", Link: " << (dev->IsLinkUp() ? "Up" : "Down"));
    }

    // Configura endereços IPv6 para os dispositivos.
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:db8::"), Ipv6Prefix(64));  // Prefixo de rede.
    Ipv6InterfaceContainer interfaces = ipv6.Assign(sixlowpanDevices);
    interfaces.SetForwarding(10, true);  // Habilita encaminhamento no gateway.
    interfaces.SetDefaultRouteInAllNodes(10);  // Define o gateway como rota padrão.

    // Loga os endereços IPv6 atribuídos.
    for (uint32_t i = 0; i < interfaces.GetN(); ++i) {
        NS_LOG_INFO("Node " << i << " Address: " << interfaces.GetAddress(i, 1));
    }

    // Configura a aplicação MQTT.
    uint16_t port = 1883;  // Porta padrão MQTT.
    Ipv6Address gatewayAddress = interfaces.GetAddress(10, 1);  // Endereço do gateway.
    NS_LOG_INFO("Gateway Address: " << gatewayAddress);
    // Configura os nós sensores (0-9) para enviar pacotes ao gateway.
    for (uint32_t i = 0; i < 10; ++i) {
        Ptr<MqttPublisher> app = CreateObject<MqttPublisher>();
        app->Setup(gatewayAddress, port, i);
        app->SetStartTime(Seconds(2.0));  // Inicia após 2s para permitir associação.
        app->SetStopTime(Seconds(15.0));  // Para após 15s.
        nodes.Get(i)->AddApplication(app);
    }

    // Configura o gateway (nó 10) para receber pacotes.
    Ptr<MqttPublisher> gatewayApp = CreateObject<MqttPublisher>();
    gatewayApp->Setup(Ipv6Address::GetAny(), port, 10);  // Escuta em qualquer endereço.
    gatewayApp->SetStartTime(Seconds(0.0));  // Inicia imediatamente.
    gatewayApp->SetStopTime(Seconds(15.0));  // Para após 15s.
    nodes.Get(10)->AddApplication(gatewayApp);

    // Configura o monitoramento de fluxo para coletar métricas de latência.
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Inicia a simulação.
    NS_LOG_INFO("Simulation starting at " << Simulator::Now().GetSeconds() << "s");
    Simulator::Stop(Seconds(15.0));  // Para após 15s.
    Simulator::Run();  // Executa a simulação.
    NS_LOG_INFO("Simulation completed at " << Simulator::Now().GetSeconds() << "s");

    // Salva as métricas existentes em arquivos.
    std::ofstream latencyFile("/ns-3-dev/output/latency.txt", std::ios::trunc);  // Arquivo para latência.
    std::ofstream messagesSentFile("/ns-3-dev/output/messages_sent.txt", std::ios::trunc);  // Arquivo para mensagens enviadas.
    std::ofstream energyFile("/ns-3-dev/output/energy_consumption.txt", std::ios::trunc);  // Arquivo para energia consumida.

    // Coleta métricas de latência usando FlowMonitor.
    monitor->CheckForLostPackets();
    Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    NS_LOG_INFO("Flow Monitor Stats: " << stats.size() << " flows detected");
    // Loop para processar cada fluxo e calcular a latência média.
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->second.rxPackets > 0) {
            // Calcula a latência média por fluxo.
            double avgDelay = it->second.delaySum.GetSeconds() / it->second.rxPackets;
            NS_LOG_INFO("Flow ID: " << it->first << ", Rx Packets: " << it->second.rxPackets << ", Avg Delay: " << avgDelay);
            latencyFile << avgDelay << "\n";  // Salva no arquivo.
        }
    }
    latencyFile.close();

    // Coleta métricas de mensagens enviadas e energia consumida.
    for (uint32_t i = 0; i < 10; ++i) {
        Ptr<MqttPublisher> app = DynamicCast<MqttPublisher>(nodes.Get(i)->GetApplication(0));
        uint32_t sent = app->GetPacketsSent();  // Número de pacotes enviados.
        uint32_t received = app->GetPacketsReceived();  // Número de pacotes recebidos.
        // Calcula energia: 0,01 J por pacote enviado, 0,005 J por pacote recebido.
        double energy = (sent * 0.01) + (received * 0.005);
        messagesSentFile << sent << "\n";  // Salva mensagens enviadas.
        energyFile << energy << "\n";  // Salva energia consumida.
        NS_LOG_INFO("Node " << i << " Sent: " << sent << ", Received: " << received << ", Energy: " << energy << " Joules");
    }
    messagesSentFile.close();
    energyFile.close();

    // Fecha o arquivo CSV de logs.
    CloseLogCsv();

    // Finaliza a simulação, liberando recursos.
    Simulator::Destroy();
    return 0;
}