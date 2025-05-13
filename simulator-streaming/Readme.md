Esta simulação demonstra a implementação de **Quality of Service (QoS)** em uma rede de computadores usando o simulador **NS-3**. O objetivo é priorizar o tráfego de **streaming de vídeo (UDP)** sobre o tráfego de **FTP (TCP)**, garantindo melhor desempenho para aplicações sensíveis a atrasos.

---

## **1. Objetivo da Simulação**
- **Comparar o desempenho** de dois tipos de tráfego competindo na mesma rede:
  - **Vídeo (UDP)**: Priorizado com **DSCP 46 (Expedited Forwarding - EF)**, garantindo baixo atraso.
  - **FTP (TCP)**: Tráfego padrão com **DSCP 0 (Best Effort - BE)**.
- **Avaliar métricas de rede**:
  - **Throughput** (taxa de transferência)
  - **Atraso médio** (latência)
  - **Taxa de perda de pacotes**

---

## **2. Topologia da Rede**
A simulação implementa a seguinte topologia:

```
       [Cliente 1: Vídeo (UDP)]
              | (10.1.1.0/24)
       [ Roteador ] 
              | (10.1.3.0/24)
       [Servidor]
              |
       [Cliente 2: FTP (TCP)]
              | (10.1.2.0/24)
```

- **2 Clientes**:
  - **Cliente 1**: Gera tráfego de vídeo (UDP) a **2 Mbps**.
  - **Cliente 2**: Gera tráfego de FTP (TCP) em **máxima velocidade**.
- **1 Roteador**: Gerencia o tráfego usando **FQ-CoDel** (algoritmo de fila justa).
- **1 Servidor**: Recebe os fluxos de vídeo e FTP.

---

## **3. Mecanismo de QoS (DiffServ)**
Para garantir prioridade ao tráfego de vídeo, a simulação usa:

### **a) Marcação DSCP (Differentiated Services Code Point)**
- **Vídeo (UDP)**: Marcado com **DSCP 46 (EF - Expedited Forwarding)**, indicando alta prioridade.
- **FTP (TCP)**: Marcado com **DSCP 0 (BE - Best Effort)**, tratado como tráfego comum.

### **b) Fila FQ-CoDel (Fair Queuing with Controlled Delay)**
- **Justiça entre fluxos**: Evita que um fluxo monopolize a banda.
- **Controle de atraso**: Reduz o atraso para tráfego prioritário.
- **Implementação no roteador**:
  ```cpp
  TrafficControlHelper tch;
  tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc"); // Configura FQ-CoDel
  QueueDiscContainer qdiscs = tch.Install(routerDevices); // Aplica ao roteador
  ```

---

## **4. Aplicações e Tráfego**
| Aplicação       | Protocolo | Porta | Taxa de Dados | Prioridade (DSCP) |
|----------------|-----------|-------|--------------|-------------------|
| Streaming Vídeo | UDP       | 5000  | 2 Mbps       | 46 (EF)           |
| FTP            | TCP       | 5001  | Máxima       | 0 (BE)            |

### **Configuração do Tráfego**
- **Vídeo (OnOffApplication)**:
  ```cpp
  OnOffHelper videoSource("ns3::UdpSocketFactory", InetSocketAddress(serverIp, 5000));
  videoSource.SetAttribute("DataRate", DataRateValue(DataRate("2Mbps")));
  ```
- **FTP (BulkSendApplication)**:
  ```cpp
  BulkSendHelper ftpSource("ns3::TcpSocketFactory", InetSocketAddress(serverIp, 5001));
  ftpSource.SetAttribute("MaxBytes", UintegerValue(0)); // Transmite indefinidamente
  ```

---

## **5. Métricas Coletadas**
Ao final da simulação, são gerados os seguintes dados:

1. **Throughput (Mbps)**:
   - Taxa de transferência efetiva de cada fluxo.
2. **Atraso Médio (ms)**:
   - Tempo que os pacotes levam para trafegar.
3. **Taxa de Perda (%)**:
   - Quantidade de pacotes perdidos no caminho.

### **Exemplo de Saída (CSV)**:
```
FlowID,SourceIP,SourcePort,DestinationIP,DestinationPort,Protocol,Throughput(Mbps),AvgDelay(ms),PacketLossRate(%),DSCP
1,10.1.1.1,49153,10.1.3.2,5000,UDP,1.92,45.3,0.0,46
2,10.1.2.1,49154,10.1.3.2,5001,TCP,3.10,62.1,0.1,0
```

---

## **6. Análise com Gráficos (Python)**
O script `analyze_results.py` gera gráficos comparando:

1. **Throughput por Fluxo**:
   - Mostra a diferença de vazão entre UDP e TCP.
2. **Atraso Médio**:
   - Compara a latência do vídeo (prioritário) vs. FTP.
3. **Taxa de Perda**:
   - Verifica se o QoS reduziu perdas no tráfego de vídeo.
4. **Comparação Agregada por DSCP**:
   - Mostra o impacto da priorização.

**Exemplo de Gráfico:**
```
Throughput (Mbps)
   |
4.0|       TCP (FTP)
   |       *****
3.0|       *****
   |       *****
2.0| UDP (Vídeo)
   |  *****
1.0|  *****
   |__|___|___|___|
      Flow1 Flow2
```

---

## **7. Conclusão**
Esta simulação demonstra como:
- **O QoS (DSCP + FQ-CoDel) melhora o desempenho** do streaming de vídeo.
- **TCP (FTP) não sofre starvation**, pois o FQ-CoDel garante justiça.
- **Priorização funciona**: O vídeo tem menor atraso e perda, mesmo com tráfego concorrente.