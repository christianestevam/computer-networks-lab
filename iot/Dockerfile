FROM ubuntu:22.04

# Atualizar pacotes e instalar dependências
RUN apt-get update && apt-get install -y \
    g++ \
    python3 \
    python3-dev \
    python3-pip \
    pkg-config \
    sqlite3 \
    git \
    mercurial \
    libxml2-dev \
    libxslt1-dev \
    build-essential \
    cmake \
    libgsl-dev \
    libgtk-3-dev \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

# Instalar matplotlib e outras bibliotecas Python
RUN pip3 install matplotlib numpy pandas

# Clonar e compilar ns-3
RUN git clone https://gitlab.com/nsnam/ns-3-dev.git /ns-3-dev
WORKDIR /ns-3-dev
RUN ./ns3 configure --enable-examples --enable-tests --enable-modules=core,network,internet,lr-wpan,sixlowpan,mobility,applications,flow-monitor
RUN ./ns3 build

# Copiar arquivos de simulação
COPY sixlowpan_mqtt_simulation.cc /ns-3-dev/scratch/
COPY plot_metrics.py /ns-3-dev/

# Configurar diretório de trabalho
WORKDIR /ns-3-dev