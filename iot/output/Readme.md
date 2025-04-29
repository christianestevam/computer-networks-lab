Esta simulação modela uma rede IoT utilizando o ns-3, um simulador de redes amplamente usado para pesquisa e desenvolvimento. O objetivo é simular uma rede de sensores que se comunicam com um gateway usando o protocolo MQTT (Message Queuing Telemetry Transport) sobre uma pilha de protocolos que inclui 6LoWPAN (IPv6 over Low-Power Wireless Personal Area Networks) e IEEE 802.15.4 (LrWpan no ns-3). A simulação coleta métricas como latência, número de mensagens enviadas e consumo de energia estimado, salvando os resultados em arquivos e permitindo a geração de gráficos.


Camada física e de enlace: IEEE 802.15.4 (via LrWpanHelper).
Camada de adaptação: 6LoWPAN (via SixLowPanHelper).
Camada de rede: IPv6 (via Ipv6AddressHelper).
Camada de transporte: UDP (usado para simular MQTT).
Camada de aplicação: MQTT simulado pela classe MqttPublisher.