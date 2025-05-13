import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Configurações iniciais
plt.style.use('seaborn')
sns.set_palette("colorblind")

# Carrega os dados
results_file = "/ns-3-dev/output/simulation_results.csv"
df = pd.read_csv(results_file)

# Cria diretório para os gráficos
os.makedirs("/ns-3-dev/output/plots", exist_ok=True)

# Função para salvar gráficos
def save_plot(fig, filename):
    fig.savefig(f"/ns-3-dev/output/plots/{filename}", dpi=300, bbox_inches='tight')
    plt.close()

# Gráfico 1: Throughput por Fluxo
plt.figure(figsize=(10, 6))
sns.barplot(x='FlowID', y='Throughput(Mbps)', hue='Protocol', data=df)
plt.title("Throughput por Fluxo")
plt.xlabel("ID do Fluxo")
plt.ylabel("Throughput (Mbps)")
save_plot(plt, "throughput.png")

# Gráfico 2: Atraso Médio
plt.figure(figsize=(10, 6))
sns.barplot(x='FlowID', y='AvgDelay(ms)', hue='DSCP', data=df)
plt.title("Atraso Médio por Fluxo")
plt.xlabel("ID do Fluxo")
plt.ylabel("Atraso (ms)")
save_plot(plt, "delay.png")

# Gráfico 3: Taxa de Perda de Pacotes
plt.figure(figsize=(10, 6))
sns.barplot(x='FlowID', y='PacketLossRate(%)', hue='DSCP', data=df)
plt.title("Taxa de Perda de Pacotes")
plt.xlabel("ID do Fluxo")
plt.ylabel("Perda (%)")
save_plot(plt, "packet_loss.png")

# Gráfico 4: Comparação Agregada por DSCP
plt.figure(figsize=(12, 8))

metrics = ['Throughput(Mbps)', 'AvgDelay(ms)', 'PacketLossRate(%)']
titles = ['Throughput', 'Atraso Médio', 'Taxa de Perda']

for i, (metric, title) in enumerate(zip(metrics, titles), 1):
    plt.subplot(1, 3, i)
    sns.barplot(x='DSCP', y=metric, data=df)
    plt.title(title)
    if metric == 'Throughput(Mbps)':
        plt.ylabel("Mbps")
    elif metric == 'AvgDelay(ms)':
        plt.ylabel("ms")
    else:
        plt.ylabel("%")

save_plot(plt, "dscp_comparison.png")

print("Análise concluída. Gráficos salvos em /ns-3-dev/output/plots/")