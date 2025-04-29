# Importa bibliotecas do Mininet para criar e gerenciar redes virtuais
from mininet.net import Mininet  # Classe principal para criar a rede
from mininet.topo import Topo    # Classe base para definir topologias
from mininet.node import RemoteController  # Para conectar a um controlador SDN externo (POX)
from mininet.cli import CLI       # Interface de linha de comando do Mininet
from mininet.util import dumpNodeConnections  # Função para exibir conexões entre nós

# Importa bibliotecas para visualização gráfica
import matplotlib.pyplot as plt   # Para gerar gráficos (latências e topologia)
import networkx as nx             # Para criar e manipular grafos da topologia
import seaborn as sns             # Para estilização avançada do gráfico

# Importa bibliotecas padrão do Python
import os                         # Para operações no sistema operacional (ex.: caminhos de arquivos)
import re                         # Para expressões regulares (análise de saída do ping)
import time                       # Para temporização (espera o POX iniciar)
import subprocess                 # Para executar o controlador POX como subprocesso
import signal                     # Para manipular sinais de término de processos
import socket                     # Para verificar se a porta do controlador está aberta

# Define a classe da topologia da rede, herdando de Topo (Mininet)
class FourNodeTopo(Topo):
    def build(self):
        # Cria 4 hosts (clientes) e 1 host servidor
        h1 = self.addHost('h1')    # Adiciona o host h1
        h2 = self.addHost('h2')    # Adiciona o host h2
        h3 = self.addHost('h3')    # Adiciona o host h3
        h4 = self.addHost('h4')    # Adiciona o host h4
        server = self.addHost('server')  # Adiciona o host servidor

        # Cria um switch SDN chamado s1
        switch = self.addSwitch('s1')

        # Conecta cada host ao switch, formando uma topologia em estrela
        self.addLink(h1, switch)    # Link entre h1 e s1
        self.addLink(h2, switch)    # Link entre h2 e s1
        self.addLink(h3, switch)    # Link entre h3 e s1
        self.addLink(h4, switch)    # Link entre h4 e s1
        self.addLink(server, switch)  # Link entre server e s1

# Função para extrair latências da saída do comando ping
def parse_ping_output(ping_output):
    latencies = []  # Lista para armazenar as latências
    for line in ping_output.splitlines():  # Divide a saída em linhas
        if 'time=' in line:  # Procura por linhas contendo 'time=' (latência do ping)
            match = re.search(r'time=(\d+\.\d+)', line)  # Extrai o valor numérico com regex
            if match:
                latencies.append(float(match.group(1)))  # Converte para float e adiciona à lista
    return latencies  # Retorna a lista de latências

# Função para verificar se uma porta está aberta (usada para confirmar o POX)
def check_port(host, port, timeout=5):
    """Check if a port is open on the given host."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Cria um socket TCP
    sock.settimeout(timeout)  # Define o tempo limite para a conexão
    try:
        result = sock.connect_ex((host, port))  # Tenta conectar ao host:porta
        return result == 0  # Retorna True se a conexão for bem-sucedida (porta aberta)
    finally:
        sock.close()  # Fecha o socket

# Função para visualizar a topologia e salvar como imagem
def plot_topology():
    G = nx.Graph()  # Cria um grafo vazio usando NetworkX
    
    # Adiciona os nós (hosts e switch) ao grafo
    nodes = ['h1', 'h2', 'h3', 'h4', 'server', 's1']
    G.add_nodes_from(nodes)
    
    # Adiciona as arestas (links entre hosts e switch)
    edges = [('h1', 's1'), ('h2', 's1'), ('h3', 's1'), ('h4', 's1'), ('server', 's1')]
    G.add_edges_from(edges)
    
    # Define posições dos nós para um layout em estrela
    pos = {
        's1': (0, 0),      # Switch no centro
        'h1': (0, 1),      # Host h1 acima
        'h2': (1, 0),      # Host h2 à direita
        'h3': (0, -1),     # Host h3 abaixo
        'h4': (-1, 0),     # Host h4 à esquerda
        'server': (0.5, 0.5)  # Servidor em uma posição diagonal
    }
    
    # Cria uma nova figura para o gráfico da topologia
    plt.figure(figsize=(6, 6))
    
    # Desenha os nós, diferenciando hosts (azul) e switch (verde)
    nx.draw_networkx_nodes(G, pos, nodelist=['h1', 'h2', 'h3', 'h4', 'server'], node_color='lightblue', node_size=500, label='Hosts')
    nx.draw_networkx_nodes(G, pos, nodelist=['s1'], node_color='lightgreen', node_size=500, label='Switch')
    
    # Desenha as arestas (conexões entre nós)
    nx.draw_networkx_edges(G, pos)
    
    # Adiciona rótulos aos nós (ex.: h1, s1)
    nx.draw_networkx_labels(G, pos)
    
    # Adiciona título e legenda ao gráfico
    plt.title("Network Topology")
    plt.legend()
    
    # Remove os eixos para melhor visualização
    plt.axis('off')
    
    # Salva o gráfico da topologia em /opt/results/topology.png
    output_path = '/opt/results/topology.png'
    plt.savefig(output_path)
    print(f"Topology plot saved at {output_path}")
    
    # Fecha a figura para liberar memória
    plt.close()

# Função principal que executa a simulação
def run_simulation():
    # Cria a topologia definida em FourNodeTopo
    topo = FourNodeTopo()
    
    # Inicializa a rede Mininet com a topologia e um controlador remoto (POX)
    net = Mininet(topo=topo, controller=lambda name: RemoteController(name, ip='127.0.0.1', port=6633))
    
    print("Starting POX controller...")
    # Inicia o controlador POX em um processo separado, usando o módulo forwarding.l2_learning
    pox_process = subprocess.Popen(
        ['python3', '/opt/pox/pox.py', 'forwarding.l2_learning'],
        stdout=subprocess.PIPE,  # Redireciona a saída padrão para captura
        stderr=subprocess.PIPE,  # Redireciona a saída de erro para captura
        text=True  # Trata a saída como texto (não binário)
    )
    
    # Aguarda até que o POX esteja ouvindo na porta 6633
    max_wait = 15  # Tempo máximo de espera (15 segundos)
    start_time = time.time()  # Marca o tempo inicial
    while time.time() - start_time < max_wait:  # Loop até o tempo limite
        if check_port('127.0.0.1', 6633):  # Verifica se a porta 6633 está aberta
            print("POX controller is listening on port 6633")
            break  # Sai do loop se o POX estiver pronto
        if pox_process.poll() is not None:  # Verifica se o POX terminou inesperadamente
            stdout, stderr = pox_process.communicate()  # Captura a saída e erro
            print(f"Error: POX failed to start. Exit code: {pox_process.returncode}")
            print(f"POX stdout: {stdout}")
            print(f"POX stderr: {stderr}")
            pox_process.terminate()  # Encerra o processo POX
            return  # Sai da função
        time.sleep(1)  # Aguarda 1 segundo antes de verificar novamente
    else:  # Se o tempo máximo for atingido sem sucesso
        print("Error: POX did not start within", max_wait, "seconds")
        pox_process.terminate()  # Encerra o POX
        return  # Sai da função
    
    try:
        # Inicia a rede Mininet (conecta o switch ao POX)
        net.start()
        print("Node connections:")
        # Exibe as conexões entre os hosts (interfaces e IPs)
        dumpNodeConnections(net.hosts)

        # Gera e salva a visualização gráfica da topologia
        plot_topology()

        # Obtém o nó 'server' e inicia um servidor HTTP na porta 8000
        server = net.get('server')
        server.cmd('python3 -m http.server 8000 &')  # Inicia o servidor em segundo plano
        print("Web server started at", server.IP(), ":8000")

        print("Starting pings between hosts...")
        # Obtém os nós h1, h2, h3, h4
        h1, h2, h3, h4 = net.get('h1', 'h2', 'h3', 'h4')
        # Executa ping de h1 para h2 (4 pacotes)
        ping_result1 = h1.cmd('ping -c 4 ' + h2.IP())
        # Executa ping de h3 para h4 (4 pacotes)
        ping_result2 = h3.cmd('ping -c 4 ' + h4.IP())
        print("Ping h1 -> h2:\n", ping_result1)
        print("Ping h3 -> h4:\n", ping_result2)

        # Extrai as latências dos resultados dos pings
        latencies1 = parse_ping_output(ping_result1)
        latencies2 = parse_ping_output(ping_result2)
        print("Latencies h1 -> h2:", latencies1)
        print("Latencies h3 -> h4:", latencies2)

        print("Testing connectivity to the server...")
        # Testa conectividade HTTP de cada host ao servidor
        for host in [h1, h2, h3, h4]:
            result = host.cmd('curl -s http://' + server.IP() + ':8000')  # Envia requisição HTTP
            print(f"Response from {host.name} to server:", "OK" if result else "Failure")

        # Define o estilo do gráfico para um tema escuro e profissional
        sns.set_style("darkgrid")  # Usa o estilo darkgrid do seaborn

        # Cria uma nova figura para o gráfico de latências com proporções ajustadas
        plt.figure(figsize=(10, 6), facecolor='#2E2E2E')  # Fundo escuro para a figura
        ax = plt.gca()  # Obtém o eixo atual
        ax.set_facecolor('#3C3C3C')  # Define o fundo do gráfico como cinza escuro

        # Plota as latências com linhas mais grossas, marcadores e cores contrastantes
        plt.plot(latencies1, label='Latency h1 -> h2 (ms)', color='#1f77b4', linewidth=2.5, marker='o', markersize=8)
        plt.plot(latencies2, label='Latency h3 -> h4 (ms)', color='#ff7f0e', linewidth=2.5, marker='o', markersize=8)

        # Ajusta os rótulos dos eixos e o título
        plt.xlabel('Test (ToS)', fontsize=12, color='white')  # Rótulo do eixo X
        plt.ylabel('Latency (ms)', fontsize=12, color='white')  # Rótulo do eixo Y
        plt.title('Latency Metrics', fontsize=14, color='white', pad=20)  # Título do gráfico

        # Ajusta a legenda para fora do gráfico
        plt.legend(loc='upper right', bbox_to_anchor=(1.15, 1), fontsize=10, facecolor='#2E2E2E', edgecolor='white', labelcolor='white')

        # Ajusta a grade para ser mais suave
        plt.grid(True, which='both', linestyle='--', alpha=0.5, color='gray')

        # Ajusta a cor dos ticks (marcas nos eixos)
        ax.tick_params(axis='both', colors='white')

        # Ajusta a cor das bordas do gráfico
        for spine in ax.spines.values():
            spine.set_color('white')

        # Salva o gráfico de latências em /opt/results/latency_plot.png
        output_path = '/opt/results/latency_plot.png'
        plt.savefig(output_path, bbox_inches='tight', dpi=300)  # Salva com alta resolução
        print(f"Latency plot saved at {output_path}")
        plt.close()  # Fecha a figura para liberar memória

        # Inicia o CLI do Mininet para interação manual
        CLI(net)
    
    finally:
        print("Shutting down simulation...")
        # Para a rede Mininet
        net.stop()
        # Encerra o processo POX
        pox_process.terminate()
        try:
            pox_process.wait(timeout=5)  # Aguarda até 5 segundos para o POX encerrar
        except subprocess.TimeoutExpired:
            pox_process.kill()  # Força o término se o POX não responder
        print("POX controller terminated.")

# Ponto de entrada do script
if __name__ == "__main__":
    run_simulation()  # Executa a função principal