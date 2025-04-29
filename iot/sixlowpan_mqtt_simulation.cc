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

using namespace ns3;

// Define LogComponent for MqttPublisher
NS_LOG_COMPONENT_DEFINE("MqttPublisher");

class MqttPublisher : public Application {
public:
    MqttPublisher();
    virtual ~MqttPublisher();
    void Setup(Ipv6Address address, uint16_t port, uint32_t nodeId);
    void HandleReceive(Ptr<Socket> socket);
private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    Ipv6Address m_peerAddress;
    uint16_t m_port;
    Ptr<Socket> m_socket;
    uint32_t m_nodeId;
    uint32_t m_packetCount;
    bool m_running;
    static const uint32_t MAX_PACKETS = 5; // Reduced for testing
};

MqttPublisher::MqttPublisher() : m_port(0), m_socket(0), m_nodeId(0), m_packetCount(0), m_running(false) {}
MqttPublisher::~MqttPublisher() { m_socket = 0; }

void MqttPublisher::Setup(Ipv6Address address, uint16_t port, uint32_t nodeId) {
    m_peerAddress = address;
    m_port = port;
    m_nodeId = nodeId;
}

void MqttPublisher::HandleReceive(Ptr<Socket> socket) {
    Ptr<Packet> packet;
    while ((packet = socket->Recv())) {
        NS_LOG_INFO("Node " << m_nodeId << " received packet, Size: " << packet->GetSize());
    }
}

void MqttPublisher::StartApplication(void) {
    if (m_packetCount >= MAX_PACKETS) {
        NS_LOG_INFO("Node " << m_nodeId << " reached max packets (" << MAX_PACKETS << ")");
        return;
    }

    m_running = true;
    if (!m_socket) {
        m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        m_socket->Connect(Inet6SocketAddress(m_peerAddress, m_port));
        m_socket->SetRecvCallback(MakeCallback(&MqttPublisher::HandleReceive, this));
    }

    // Use fixed-size buffer for message
    char message[32];
    double temp = 20.0 + (rand() % 100) / 10.0;
    int hum = 50 + rand() % 30;
    snprintf(message, sizeof(message), "Temp: %.1f C, Hum: %d%%", temp, hum);
    uint32_t messageLength = strlen(message);

    // Create packet safely
    uint8_t* buffer = new uint8_t[messageLength];
    std::memcpy(buffer, message, messageLength);
    Ptr<Packet> packet = Create<Packet>(buffer, messageLength);
    delete[] buffer;

    int result = m_socket->Send(packet);
    NS_LOG_INFO("Node " << m_nodeId << " sent packet " << m_packetCount << ": " << message << ", Send Result: " << result << ", Length: " << messageLength);
    m_packetCount++;

    // Schedule next packet if within limit and running
    if (m_running && m_packetCount < MAX_PACKETS) {
        Simulator::Schedule(Seconds(5.0 + (rand() % 5) / 10.0), &MqttPublisher::StartApplication, this);
    }
}

void MqttPublisher::StopApplication(void) {
    m_running = false;
    if (m_socket) {
        m_socket->Close();
        m_socket = 0;
    }
}

int main(int argc, char *argv[]) {
    LogComponentEnable("MqttPublisher", LOG_LEVEL_INFO);
    LogComponentEnable("Ipv6AddressHelper", LOG_LEVEL_INFO); // Debug address assignment
    LogComponentEnable("SixLowPanNetDevice", LOG_LEVEL_INFO); // Debug 6LoWPAN
    LogComponentEnable("LrWpanNetDevice", LOG_LEVEL_INFO); // Debug LR-WPAN
    LogComponentEnable("LrWpanMac", LOG_LEVEL_ALL); // Debug MAC layer

    // Criar nós: 10 sensores + 1 gateway
    NodeContainer nodes;
    nodes.Create(11);

    // Configurar LrWpan (IEEE 802.15.4)
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer devices = lrWpanHelper.Install(nodes);

    // Ensure unique MAC addresses and PAN configuration
    uint16_t panId = 0x1234;
    Mac16Address coordShortAddr("00:00");
    for (uint32_t i = 0; i < devices.GetN(); ++i) {
        Ptr<lrwpan::LrWpanNetDevice> lrWpanDev = DynamicCast<lrwpan::LrWpanNetDevice>(devices.Get(i));
        uint8_t macAddr[2] = {0x00, static_cast<uint8_t>(i)};
        Mac16Address shortAddr;
        shortAddr.CopyFrom(macAddr);
        lrWpanDev->SetAddress(shortAddr);
        lrWpanDev->GetMac()->SetPanId(panId);
        // Set extended address for uniqueness
        std::ostringstream oss;
        oss << "00:00:00:00:00:00:00:" << std::setfill('0') << std::setw(2) << std::hex << i;
        lrWpanDev->GetMac()->SetExtendedAddress(Mac64Address(oss.str().c_str()));
        NS_LOG_INFO("LrWpan Device " << i << " Short MAC: " << lrWpanDev->GetAddress() << ", Extended MAC: " << lrWpanDev->GetMac()->GetExtendedAddress() << ", PAN ID: " << lrWpanDev->GetMac()->GetPanId());
    }

    // Configure node 0 as PAN coordinator
    Ptr<lrwpan::LrWpanNetDevice> coordDev = DynamicCast<lrwpan::LrWpanNetDevice>(devices.Get(0));
    Ptr<lrwpan::LrWpanMac> coordMac = coordDev->GetMac();
    coordMac->SetShortAddress(coordShortAddr);
    lrwpan::MlmeStartRequestParams startParams;
    startParams.m_PanId = panId;
    startParams.m_bcnOrd = 15; // Non-beacon enabled
    startParams.m_sfrmOrd = 15; // Non-beacon enabled
    coordMac->MlmeStartRequest(startParams);
    NS_LOG_INFO("Started PAN on coordinator with Short MAC: " << coordShortAddr << ", PAN ID: " << panId);

    // Schedule association for nodes 1–10
    for (uint32_t i = 1; i < devices.GetN(); ++i) {
        Ptr<lrwpan::LrWpanNetDevice> lrWpanDev = DynamicCast<lrwpan::LrWpanNetDevice>(devices.Get(i));
        Ptr<lrwpan::LrWpanMac> mac = lrWpanDev->GetMac();
        lrwpan::MlmeAssociateRequestParams assocParams;
        assocParams.m_coordAddrMode = 2; // SHORT_ADDR
        assocParams.m_coordShortAddr = coordShortAddr;
        assocParams.m_coordPanId = panId;
        assocParams.m_chNum = 11; // Default channel
        assocParams.m_chPage = 0; // Default channel page
        assocParams.m_capabilityInfo = 0x80; // Device is FFD, capable of being a coordinator
        Simulator::Schedule(Seconds(0.1 * i), &lrwpan::LrWpanMac::MlmeAssociateRequest, mac, assocParams);
        NS_LOG_INFO("Scheduled association for MAC: " << mac->GetShortAddress() << " to PAN ID: " << panId << " with coordinator: " << coordShortAddr << " at time " << Seconds(0.1 * i));
    }

    // Enable PCAP for debugging
    lrWpanHelper.EnablePcap("lrwpan", devices);

    // Configurar mobilidade
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue("50.0"),
                                 "Y", StringValue("50.0"),
                                 "Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=50]"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Instalar pilha de rede
    InternetStackHelper internet;
    internet.Install(nodes);

    // Configurar 6LoWPAN
    SixLowPanHelper sixlowpan;
    NetDeviceContainer sixlowpanDevices = sixlowpan.Install(devices);

    // Debug: Log 6LoWPAN devices and check link status
    for (uint32_t i = 0; i < sixlowpanDevices.GetN(); ++i) {
        Ptr<SixLowPanNetDevice> dev = DynamicCast<SixLowPanNetDevice>(sixlowpanDevices.Get(i));
        NS_LOG_INFO("6LoWPAN Device " << i << ": " << dev->GetAddress() << ", Link: " << (dev->IsLinkUp() ? "Up" : "Down"));
    }

    // Configurar endereços IPv6
    Ipv6AddressHelper ipv6;
    ipv6.NewNetwork(); // Reset address generator state
    ipv6.SetBase("2001:db8:1::", Ipv6Prefix(64)); // Use a unique prefix
    Ipv6InterfaceContainer interfaces = ipv6.Assign(sixlowpanDevices);
    interfaces.SetForwarding(10, true); // Enable forwarding on the gateway
    interfaces.SetDefaultRouteInAllNodes(10); // Set default route to gateway

    // Debug: Log assigned addresses
    for (uint32_t i = 0; i < interfaces.GetN(); ++i) {
        NS_LOG_INFO("Node " << i << " Address: " << interfaces.GetAddress(i, 1));
    }

    // Configurar aplicação MQTT
    uint16_t port = 1883;
    Ipv6Address gatewayAddress = interfaces.GetAddress(10, 1); // Gateway address
    NS_LOG_INFO("Gateway Address: " << gatewayAddress);
    for (uint32_t i = 0; i < 10; ++i) {
        Ptr<MqttPublisher> app = CreateObject<MqttPublisher>();
        app->Setup(gatewayAddress, port, i);
        app->SetStartTime(Seconds(2.0)); // Delay to allow association
        app->SetStopTime(Seconds(50.0)); // Reduced for testing
        nodes.Get(i)->AddApplication(app);
    }

    // Configurar aplicação MQTT no gateway para receber pacotes
    Ptr<MqttPublisher> gatewayApp = CreateObject<MqttPublisher>();
    gatewayApp->Setup(Ipv6Address::GetAny(), port, 10); // Listen on all interfaces
    gatewayApp->SetStartTime(Seconds(0.0));
    gatewayApp->SetStopTime(Seconds(50.0));
    nodes.Get(10)->AddApplication(gatewayApp);

    // Configurar monitoramento de fluxo
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Simulação
    Simulator::Stop(Seconds(50.0)); // Reduced for testing
    Simulator::Run();

    // Coletar métricas
    std::ofstream latencyFile("/ns-3-dev/output/latency.txt");
    monitor->CheckForLostPackets();
    Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    NS_LOG_INFO("Flow Monitor Stats: " << stats.size() << " flows detected");
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->second.rxPackets > 0) {
            double avgDelay = it->second.delaySum.GetSeconds() / it->second.rxPackets;
            NS_LOG_INFO("Flow ID: " << it->first << ", Rx Packets: " << it->second.rxPackets << ", Avg Delay: " << avgDelay);
            latencyFile << avgDelay << "\n";
        }
    }
    latencyFile.close();

    Simulator::Destroy();
    return 0;
}