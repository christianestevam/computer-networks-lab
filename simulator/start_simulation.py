from mininet.net import Mininet
from mininet.topo import Topo
from mininet.node import Controller
from mininet.cli import CLI
from mininet.util import dumpNodeConnections
import matplotlib.pyplot as plt
import os

class ThreeNodeTopo(Topo):
    def build(self):
        host1 = self.addHost('h1')
        host2 = self.addHost('h2')
        host3 = self.addHost('h3')
        switch = self.addSwitch('s1')
        self.addLink(host1, switch)
        self.addLink(host2, switch)
        self.addLink(host3, switch)

def run_simulation():
    topo = ThreeNodeTopo()
    net = Mininet(topo=topo, controller=Controller)
    net.start()

    # Exibir conexões
    dumpNodeConnections(net.hosts)

    # Simular tráfego com ping
    print("Iniciando pings entre os hosts...")
    h1, h2, h3 = net.get('h1', 'h2', 'h3')
    ping_result1 = h1.cmd('ping -c 4 ' + h2.IP())
    ping_result2 = h2.cmd('ping -c 4 ' + h3.IP())
    print("Ping h1 -> h2:\n", ping_result1)
    print("Ping h2 -> h3:\n", ping_result2)

    # Extrair latências (exemplo simplificado)
    latencies = [10, 15, 20]  # Substitua por parsing real se desejar
    plt.plot(latencies, label='Latência (ms)')
    plt.xlabel('Teste')
    plt.ylabel('Latência (ms)')
    plt.title('Métricas de Latência')
    plt.legend()
    plt.grid(True)
    
    # Salvar gráfico em vez de exibir
    plt.savefig('/opt/latency_plot.png')
    print("Gráfico salvo em /opt/latency_plot.png")

    # Iniciar CLI
    CLI(net)
    net.stop()

if __name__ == "__main__":
    run_simulation()