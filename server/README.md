<!-- Language: English (top) · Español (below) -->

# F4MPServer

*[English](#english) · [Español](#español)*

<a name="english"></a>

The dedicated server for **F4MP**. A standalone Windows executable that accepts client connections over GameNetworkingSockets and relays messages (positions, chat) between players.

> ⚠️ Work in progress. Connection handling, chat/position relay and the master-server heartbeat work; higher-level game sync is still pending. See [`../CHECKLIST.md`](../CHECKLIST.md).

## Requirements

Runs on **Windows and Linux** (GameNetworkingSockets is cross-platform).

- **Windows** — build with **Visual Studio 2022** ("Desktop development with C++"). Dependencies are vendored in the repo (`../third_party/win-x64`), so **vcpkg is not required**.
- **Linux** — build with **CMake** + a C++17 compiler. System dependencies (Debian/Ubuntu):
  ```bash
  sudo apt install build-essential cmake pkg-config \
       libgamenetworkingsockets-dev libspdlog-dev libcurl4-openssl-dev
  ```
  If your distro doesn't package GameNetworkingSockets, build it from [source](https://github.com/ValveSoftware/GameNetworkingSockets) and install it.

## Building

**Windows** — from the repo root, double-click **`build_server.bat`**. It sets up the Visual Studio environment, builds with Ninja and copies the runtime DLLs next to the binary. Manual:

```bash
cmake -S server -B server/build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build server/build
```

Output: `server/build/F4MPServer.exe`

**Linux** — run **`./build_server_linux.sh`** from the repo root, or manually:

```bash
cmake -S server -B server/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build server/build -j$(nproc)
```

Output: `server/build/F4MPServer`

You can also grab a prebuilt Windows binary from the **Actions** tab (artifact `F4MPServer`) or from a tagged **Release**.

> Cross-platform note: the heartbeat to the master server uses **WinHTTP** on Windows and **libcurl** on Linux (selected automatically at compile time).

## Running

1. Put `F4MPServer.exe` next to the GameNetworkingSockets runtime DLLs (the build scripts and CI artifact bundle them).
2. Run it. On first launch it creates a `config.json` and exits — edit it, then run again.
3. Type `help` in the console for admin commands (`status`, `say`, `kick`, `stop`). Or press **Ctrl+C** to stop.

## Configuration (`config.json`)

| Key | Meaning | Default |
|-----|---------|---------|
| `ip` | Bind/display address | `127.0.0.1` |
| `port` | UDP port | `7779` |
| `player-limit` | Max simultaneous players | `100` |
| `server-name` | Name shown in the server list | `F4MP Server` |
| `master-server` | Master-server URL for the heartbeat (empty = off) | `""` |
| `log-location` | Log file path, or `NONE` | `./logs.log` |

If `master-server` is set, the server sends a `POST /heartbeat` every few seconds so it shows up on the web server list.

## Architecture

GameNetworkingSockets listen socket + poll group · connection state via the global status callback + `RunCallbacks()` · a 30-tick main loop routes messages between clients · optional heartbeat thread. See the root [`README.md`](../README.md) for the full picture.

---

<a name="español"></a>

# F4MPServer (Español)

*[English](#english) · [Español](#español)*

El servidor dedicado de **F4MP**. Un ejecutable de Windows independiente que acepta conexiones de clientes por GameNetworkingSockets y hace de relay de mensajes (posiciones, chat) entre jugadores.

> ⚠️ En desarrollo. Funcionan la gestión de conexiones, el relay de chat/posición y el heartbeat al master server; la sincronización de más alto nivel está pendiente. Ver [`../CHECKLIST.md`](../CHECKLIST.md).

## Requisitos

Funciona en **Windows y Linux** (GameNetworkingSockets es multiplataforma).

- **Windows** — compila con **Visual Studio 2022** ("Desarrollo para el escritorio con C++"). Las dependencias están incluidas en el repo (`../third_party/win-x64`), así que **no hace falta vcpkg**.
- **Linux** — compila con **CMake** + un compilador C++17. Dependencias del sistema (Debian/Ubuntu):
  ```bash
  sudo apt install build-essential cmake pkg-config \
       libgamenetworkingsockets-dev libspdlog-dev libcurl4-openssl-dev
  ```
  Si tu distro no empaqueta GameNetworkingSockets, compílalo desde [código](https://github.com/ValveSoftware/GameNetworkingSockets) e instálalo.

## Compilar

**Windows** — desde la raíz del repo, doble clic en **`build_server.bat`**. Prepara el entorno de Visual Studio, compila con Ninja y copia los DLLs de runtime junto al binario. Manual:

```bash
cmake -S server -B server/build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build server/build
```

Salida: `server/build/F4MPServer.exe`

**Linux** — ejecuta **`./build_server_linux.sh`** desde la raíz del repo, o a mano:

```bash
cmake -S server -B server/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build server/build -j$(nproc)
```

Salida: `server/build/F4MPServer`

También puedes descargar un binario de Windows ya compilado desde la pestaña **Actions** (artifact `F4MPServer`) o desde una **Release** con tag.

> Nota multiplataforma: el heartbeat al master server usa **WinHTTP** en Windows y **libcurl** en Linux (se elige solo al compilar).

## Ejecutar

1. Pon `F4MPServer.exe` junto a los DLLs de runtime de GameNetworkingSockets (los scripts de build y el artifact de CI los incluyen).
2. Ejecútalo. La primera vez crea un `config.json` y sale — edítalo y vuelve a ejecutar.
3. Escribe `help` en la consola para los comandos de administración (`status`, `say`, `kick`, `stop`). O pulsa **Ctrl+C** para parar.

## Configuración (`config.json`)

| Clave | Significado | Por defecto |
|-------|-------------|-------------|
| `ip` | Dirección de bind/display | `127.0.0.1` |
| `port` | Puerto UDP | `7779` |
| `player-limit` | Máximo de jugadores simultáneos | `100` |
| `server-name` | Nombre que se muestra en la lista de servidores | `F4MP Server` |
| `master-server` | URL del master server para el heartbeat (vacío = off) | `""` |
| `log-location` | Ruta del archivo de log, o `NONE` | `./logs.log` |

Si `master-server` está definido, el servidor envía un `POST /heartbeat` cada pocos segundos para aparecer en la lista de servidores de la web.

## Arquitectura

Listen socket de GameNetworkingSockets + poll group · estado de conexión vía el callback global de estado + `RunCallbacks()` · un bucle principal a 30 ticks enruta los mensajes entre clientes · hilo opcional de heartbeat. Ver el [`README.md`](../README.md) raíz para el panorama completo.
