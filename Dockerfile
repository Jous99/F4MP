# ---- Imagen del servidor dedicado F4MP (Linux) ----
# Build:  docker build -t f4mp-server .
# Run:    docker run -d --name f4mp -p 7779:7779/udp -v f4mp-data:/data f4mp-server
#
# Nota: en el config.json monta "ip": "0.0.0.0" para que escuche fuera del
# contenedor (127.0.0.1 solo seria accesible dentro).

# ---------- Etapa de compilacion ----------
FROM debian:bookworm AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake pkg-config \
        libgamenetworkingsockets-dev libspdlog-dev libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY server/ ./server/

RUN cmake -S server -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && cmake --build build -j"$(nproc)"

# ---------- Etapa de ejecucion ----------
FROM debian:bookworm-slim AS runtime

# Librerias de runtime + certificados (para el heartbeat HTTPS via libcurl).
RUN apt-get update && apt-get install -y --no-install-recommends \
        libgamenetworkingsockets-dev libspdlog-dev libcurl4 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/F4MPServer /usr/local/bin/F4MPServer

# /data guarda el config.json y el log (montalo como volumen).
WORKDIR /data
EXPOSE 7779/udp

ENTRYPOINT ["F4MPServer"]
