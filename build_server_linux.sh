#!/usr/bin/env bash
# Compila F4MPServer en Linux.
# Dependencias (Debian/Ubuntu):
#   sudo apt install build-essential cmake pkg-config \
#        libgamenetworkingsockets-dev libspdlog-dev libcurl4-openssl-dev
# Si tu distro no empaqueta GameNetworkingSockets, compilalo desde:
#   https://github.com/ValveSoftware/GameNetworkingSockets
set -e

cd "$(dirname "$0")"

echo "== F4MP - Compilando servidor (Linux) =="
cmake -S server -B server/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build server/build -j"$(nproc)"

echo
echo "[OK] Binario en: server/build/F4MPServer"
echo "Ejecutalo con: ./server/build/F4MPServer"
