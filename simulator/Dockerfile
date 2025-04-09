FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Atualizar e instalar dependências básicas
RUN apt-get update -y && \
    apt-get install -y \
    python3 \
    python3-pip \
    git \
    iputils-ping \
    vim \
    curl \
    wget \
    tcpdump \
    iproute2 \
    net-tools \
    sudo \
    telnet \
    build-essential \
    python3-dev \
    openvswitch-switch \
    && apt-get clean

# Criar link simbólico para python
RUN ln -sf /usr/bin/python3 /usr/bin/python

# Instalar Mininet a partir do código-fonte
RUN git clone https://github.com/mininet/mininet.git /opt/mininet && \
    cd /opt/mininet && \
    git checkout 2.3.1 && \
    /opt/mininet/util/install.sh -nfv

# Instalar dependências Python adicionais
RUN pip3 install matplotlib numpy scipy

# Copiar o script de simulação
COPY start_simulation.py /opt/start_simulation.py

# Expor porta (opcional)
EXPOSE 8080

# Comando de inicialização
CMD service openvswitch-switch start && \
    python3 /opt/start_simulation.py