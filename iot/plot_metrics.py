import matplotlib.pyplot as plt
import numpy as np

# Carregar dados de latência
latencies = np.loadtxt("latency.txt")

# Dados simulados para mensagens e energia
nodes = np.arange(1, 11)
messages_sent = np.random.randint(15, 25, size=10)  # Simulado
energy_consumption = np.random.uniform(0.1, 0.5, size=10)  # Simulado

# Gráfico 1: Latência média
plt.figure(figsize=(8, 6))
plt.hist(latencies, bins=20, color='blue', alpha=0.7)
plt.title('Distribuição da Latência Média de Mensagens MQTT')
plt.xlabel('Latência (segundos)')
plt.ylabel('Frequência')
plt.grid(True)
plt.savefig('latency_distribution.png')
plt.close()

# Gráfico 2: Mensagens enviadas por nó
plt.figure(figsize=(8, 6))
plt.bar(nodes, messages_sent, color='green', alpha=0.7)
plt.title('Número de Mensagens Enviadas por Nó')
plt.xlabel('ID do Nó')
plt.ylabel('Mensagens Enviadas')
plt.grid(True)
plt.savefig('messages_sent.png')
plt.close()

# Gráfico 3: Consumo de energia estimado
plt.figure(figsize=(8, 6))
plt.bar(nodes, energy_consumption, color='red', alpha=0.7)
plt.title('Consumo de Energia Estimado por Nó')
plt.xlabel('ID do Nó')
plt.ylabel('Energia (Joules)')
plt.grid(True)
plt.savefig('energy_consumption.png')
plt.close()