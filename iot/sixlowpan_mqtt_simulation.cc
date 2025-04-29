#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <fstream>

using namespace ns3;

// Define LogComponent for MqttPublisher
NS_LOG_COMPONENT_DEFINE("MqttPublisher");

class MqttPublisher : public Application {
public:
    MqttPublisher();
    virtual ~MqttPublisher();
    void Setup(Ipv6Address address, uint16_t port, uint32_t nodeId);
private:
    virtual void StartApplication(void);
    Ipv6Address m_peerAddress;
    uint16_t m_port;
    Ptr<Socket> m_socket;
    uint32_t m_nodeId;
    uint32_t m_packetCount;
};

MqttPublisher::MqttPublisher() : m_port(0), m_socket(0), m_nodeId(0), m_packetCount(0) {}
MqttPublisher::~MqttPublisher() { m_socket = 0; }

void MqttPublisher::Setup(Ipv6Address address, uint16_t port, uint32_t nodeId) {
    m_peerAddress = address;
    m_port = port;
    m_nodeId = nodeId;
}

void MqttPublisher::StartApplication(void) {
    m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
    m_socket->Connect(Inet6SocketAddress(m_peerAddress, m_port));
    std::string message = "Temp: " + std::to_string(20.0 + (rand() % 100) / 10.0) + " °C, Hum: " + std::to_string(50 + rand() % 30) + "%";
    Ptr<Packet> packet = Create<Packet>((uint8_t*)message.c_str(), message.length());
    int result = m_socket->Send(packet);
    NS_LOG_INFO("Node " << m_nodeId << " sent packet " << m_packetCount << ": " << message << ", Send Result: " << result);
    m_packetCount++;
    Simulator::Schedule(Seconds(5.0 + (rand() % 5) / 10.0), &MqttPublisher::StartApplication, this);
}

int main(int argc, char *argv[]) {
    LogComponentEnable("MqttPublisher", LOG_LEVEL_INFO);
    LogComponentEnable("Ipv6AddressHelper", LOG_LEVEL_INFO); // Debug address assignment
    LogComponentEnable("SixLowPanNetDevice", LOG_LEVEL_INFO); // Debug 6LoWPAN
    LogComponentEnable("LrWpanNetDevice", LOG_LEVEL_INFO); // Debug LR-WPAN

    // Criar nós: 10 sensores + 1 gateway
    NodeContainer nodes;
    nodes.Create(11);

    // Configurar LrWpan (IEEE 802.15.4)
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer devices = lrWpanHelper.Install(nodes);

    // Ensure unique MAC addresses for each LrWpanNetDevice
    for (uint32_t i = 0; i < devices.GetN(); ++i) {
        Ptr<lrwpan::LrWpanNetDevice> lrWpanDev = DynamicCast<lrwpan::LrWpanNetDevice>(devices.Get(i));
        uint8_t macAddr[2] = {0x00, static_cast<uint8_t>(i)};
        Mac16Address address;
        address.CopyFrom(macAddr);
        lrWpanDev->SetAddress(address);
        NS_LOG_INFO("LrWpan Device " << i << " MAC: " << lrWpanDev->GetAddress());
    }

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

    // Debug: Log 6LoWPAN devices
    for (uint32_t i = 0; i < sixlowpanDevices.GetN(); ++i) {
        NS_LOG_INFO("6LoWPAN Device " << i << ": " << sixlowpanDevices.Get(i)->GetAddress());
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
        app->SetStartTime(Seconds(1.0));
        app->SetStopTime(Seconds(100.0));
        nodes.Get(i)->AddApplication(app);
    }

    // Configurar monitoramento de fluxo
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Simulação
    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    // Coletar métricas
    std::ofstream latencyFile("/ns-3-dev/output/latency.txt");
    monitor->CheckForLostPackets();
    Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        if (it->second.rxPackets > 0) {
            NS_LOG_INFO("Flow ID: " << it->first << ", Rx Packets: " << it->second.rxPackets << ", Delay: " << it->second.delaySum.GetSeconds());
            latencyFile << it->second.delaySum.GetSeconds() / it->second.rxPackets << "\n";
        }
    }
    latencyFile.close();

    Simulator::Destroy();
    return 0;
}