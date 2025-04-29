import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# Carrega os dados dos arquivos txt
try:
    latencies = np.loadtxt("output/latency.txt")
    latencies = latencies[latencies < 1.0]  # Filtra latências improváveis
except (ValueError, FileNotFoundError):
    print("Erro ao carregar latency.txt ou dados inválidos. Usando valores padrão.")
    latencies = np.array([0.0])

try:
    messages_sent = np.loadtxt("output/messages_sent.txt")
except (ValueError, FileNotFoundError):
    print("Erro ao carregar messages_sent.txt. Usando valores padrão.")
    messages_sent = np.zeros(10)

try:
    energy_consumption = np.loadtxt("output/energy_consumption.txt")
except (ValueError, FileNotFoundError):
    print("Erro ao carregar energy_consumption.txt. Usando valores padrão.")
    energy_consumption = np.zeros(10)

# Carrega o arquivo logs.csv usando pandas
try:
    df = pd.read_csv("output/logs.csv")
except FileNotFoundError:
    print("Erro ao carregar logs.csv. Usando DataFrame vazio.")
    df = pd.DataFrame(columns=["Timestamp", "NodeID", "Event", "Details"])

# Define os IDs dos nós (0 a 9)
nodes = np.arange(0, 10)

# Gráfico 1: Taxa de Envio de Pacotes ao Longo do Tempo
# Filtra eventos de envio e arredonda os timestamps
sent_packets = df[df["Event"] == "Sent Packet"].copy()  # Cria uma cópia explícita para evitar o warning
if not sent_packets.empty:
    # Usa .loc para modificar a coluna Timestamp de forma segura
    sent_packets.loc[:, "Timestamp"] = sent_packets["Timestamp"].round()
    packets_per_second = sent_packets.groupby("Timestamp").size()
    plt.figure(figsize=(8, 6))
    plt.plot(packets_per_second.index, packets_per_second.values, marker='o', color='blue', label='Pacotes Enviados')
    plt.title('Taxa de Envio de Pacotes ao Longo do Tempo (15s)')
    plt.xlabel('Tempo (segundos)')
    plt.ylabel('Número de Pacotes Enviados')
    plt.grid(True)
    plt.legend()
    plt.savefig('output/packets_per_second.png')
    plt.close()
else:
    print("Nenhum evento 'Sent Packet' encontrado no logs.csv")

# Gráfico 2: Latência de Resposta do Gateway
# Calcula a latência entre o envio e a resposta do gateway
response_latencies = []
sent = df[df["Event"] == "Sent Packet"].copy()  # Cria uma cópia explícita
responses = df[df["Event"] == "Gateway Response"].copy()

# Verifica o formato real dos endereços IPv6 no logs.csv para depuração
if not responses.empty:
    print("Exemplo de 'Details' em Gateway Response:", responses["Details"].iloc[0])

for i, sent_row in sent.iterrows():
    node_id = sent_row["NodeID"]
    sent_time = sent_row["Timestamp"]
    # Ajusta a busca pelo endereço IPv6 para ser mais flexível
    # Remove espaços e procura apenas pela parte final do endereço (ex.: "2001:db8:0:0:0:0:0:1")
    response = responses[responses["Timestamp"] > sent_time]
    if not response.empty:
        # Procura por qualquer endereço que termine com o node_id+1 (ex.: "0:1" para node_id=0)
        for _, resp_row in response.iterrows():
            details = resp_row["Details"]
            # Normaliza o endereço removendo espaços e verificando a terminação
            if str(node_id + 1) in details.split(":")[-1]:
                response_time = resp_row["Timestamp"]
                latency = response_time - sent_time
                response_latencies.append(latency)
                break

if response_latencies:
    response_latencies = np.array(response_latencies)
    plt.figure(figsize=(8, 6))
    plt.hist(response_latencies, bins=20, color='purple', alpha=0.7, label='Latência de Resposta')
    plt.axvline(x=np.mean(response_latencies), color='r', linestyle='--', label=f'Média ({np.mean(response_latencies):.3f}s)')
    plt.title('Distribuição da Latência de Resposta do Gateway (15s)')
    plt.xlabel('Latência (segundos)')
    plt.ylabel('Frequência')
    plt.grid(True)
    plt.legend()
    plt.savefig('output/response_latency_distribution.png')
    plt.close()
else:
    print("Não foi possível calcular latências de resposta (nenhuma correspondência encontrada).")

# Gráfico 3: Mensagens Enviadas por Nó
colors = plt.cm.Set3(np.linspace(0, 1, len(nodes)))
plt.figure(figsize=(8, 6))
plt.bar(nodes, messages_sent, color=colors, alpha=0.7)
plt.title('Número de Mensagens Enviadas por Nó (15s)')
plt.xlabel('ID do Nó')
plt.ylabel('Mensagens Enviadas')
plt.grid(True)
plt.savefig('output/messages_sent.png')
plt.close()

# Gráfico 4: Consumo de Energia Estimado por Nó
plt.figure(figsize=(8, 6))
plt.bar(nodes, energy_consumption, color=colors, alpha=0.7)
plt.title('Consumo de Energia Estimado por Nó (15s)')
plt.xlabel('ID do Nó')
plt.ylabel('Energia (Joules)')
plt.grid(True)
plt.savefig('output/energy_consumption.png')
plt.close()

# Gráfico 5: Temperatura Média por Nó
# Extrai os valores de temperatura do campo Details
if not sent_packets.empty:
    # Usa .loc para adicionar a nova coluna de forma segura
    sent_packets.loc[:, "Temp"] = sent_packets["Details"].str.extract(r"Temp: (\d+\.\d) C")[0].astype(float)
    avg_temp_per_node = sent_packets.groupby("NodeID")["Temp"].mean()
    plt.figure(figsize=(8, 6))
    plt.bar(avg_temp_per_node.index, avg_temp_per_node.values, color=colors, alpha=0.7)
    plt.title('Temperatura Média Reportada por Nó (15s)')
    plt.xlabel('ID do Nó')
    plt.ylabel('Temperatura Média (°C)')
    plt.grid(True)
    plt.savefig('output/avg_temperature_per_node.png')
    plt.close()
else:
    print("Nenhum dado de temperatura encontrado no logs.csv")

# Gráfico 6: Latência de Resposta vs. Mensagens Enviadas
if len(response_latencies) > 0:
    if len(response_latencies) > len(messages_sent):
        messages_expanded = np.repeat(messages_sent, len(response_latencies) // len(messages_sent) + 1)[:len(response_latencies)]
    else:
        messages_expanded = messages_sent[:len(response_latencies)]
    
    plt.figure(figsize=(8, 6))
    plt.scatter(messages_expanded, response_latencies, color='cyan', alpha=0.7)
    plt.title('Latência de Resposta vs. Mensagens Enviadas (15s)')
    plt.xlabel('Mensagens Enviadas')
    plt.ylabel('Latência de Resposta (segundos)')
    plt.grid(True)
    plt.savefig('output/response_latency_vs_messages.png')
    plt.close()
else:
    print("Não há latências de resposta suficientes para o gráfico de dispersão.")