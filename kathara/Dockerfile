FROM kathara/base
LABEL org.opencontainers.image.authors="Kathara Team <contact@kathara.org>"

ARG DEBIAN_FRONTEND="noninteractive"

# Update and install required packages
RUN apt update && \
    apt upgrade -y && \
    apt install -y libssl-dev haproxy && \
    apt clean && \
    rm -rf /tmp/* /var/lib/apt/lists/* /var/tmp/*

ENV PATH=$PATH:/root/.cargo/bin
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y

RUN git clone https://github.com/NLnetLabs/routinator && \
    cd routinator && cargo build --release --locked && \
    cd .. && rm -Rf routinator/

RUN apt clean && \
    rm -rf /tmp/* /var/lib/apt/lists/* /var/tmp/*
