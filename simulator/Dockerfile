FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Install basic dependencies, including lsb-release, openvswitch, and help2man
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
    lsb-release \
    openvswitch-switch \
    openvswitch-common \
    python3-pyparsing \
    python3-pexpect \
    iperf \
    perl \
    help2man \
    && apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Create symbolic link for python
RUN ln -sf /usr/bin/python3 /usr/bin/python

# Copy the local Mininet repository
COPY mininet /opt/mininet

# Verify that install.sh exists and is executable
RUN test -f /opt/mininet/util/install.sh || (echo "Error: install.sh not found in /opt/mininet/util" && exit 1) && \
    chmod +x /opt/mininet/util/install.sh

# Install Mininet manually with make install
RUN cd /opt/mininet && \
    make install && \
    rm -rf /opt/mininet/.git

# Copy the local POX repository
COPY pox /opt/pox

# Install additional Python dependencies (incluindo seaborn)
RUN pip3 install --no-cache-dir matplotlib numpy scipy networkx seaborn

# Create directory for results
RUN mkdir -p /opt/results

# Copy the simulation script
COPY start_simulation.py /opt/start_simulation.py

# Create a startup script to initialize OVS
RUN echo '#!/bin/bash\n\
service openvswitch-switch start\n\
python3 /opt/start_simulation.py' > /start.sh && \
    chmod +x /start.sh

# Expose port for the server
EXPOSE 8000

# Command to start OVS and the simulation
CMD ["/start.sh"]