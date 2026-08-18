<!-- Language: English (top) · Español (below) -->
<!-- Idioma: English (arriba) · Español (abajo) -->

# F4MP — Fallout 4 Multiplayer

*[English](#english) · [Español](#español)*

<a name="english"></a>

A multiplayer mod for **Fallout 4**, revived and ported to work with **Next-Gen (1.10.980+)** and **Anniversary Edition (1.11.x)**.

> The original project was deprecated in 2021. This is a revived and improved version.

## Project status

🟢 **Working (early), not yet a full multiplayer experience.** The mod loads in Fallout 4 via F4SE, connects to a dedicated server, syncs your position, and renders other players as **actual in-world bodies (NPCs) that move, interpolate smoothly, and face where the player is looking** — validated with two real clients. The bodies are currently placeholder Protectrons; a human/passive body is the next step.

The client was rebuilt on **CommonLibF4** (see [`client-ng/`](client-ng/)) after the original pattern-scanning approach proved unreliable for locating the player. The old injected/ASI client (`client/`) is deprecated.

See **[CHECKLIST.md](CHECKLIST.md)** for the full roadmap.

| Area | State |
|------|-------|
| Client loads via F4SE (CommonLibF4) | ✅ Working |
| In-game menu (F4SE Menu Framework) | ✅ Working |
| Connect to server + handshake | ✅ Working |
| Read & send local player position | ✅ Working |
| Receive other players + in-world markers | ✅ Working |
| Spawn actual player bodies (NPCs) | ✅ Working (placeholder Protectron) |
| Smooth interpolation + look-direction rotation | ✅ Working |
| Cleanup body on player disconnect | ✅ Working |
| Human / passive body (not a hostile robot) | 🟡 In progress |
| Server (poll groups, admin console, chat/pos relay) | ✅ Working |
| Test simulator (`F4MPSim`) | ✅ Working |

## Requirements

### Client (`client-ng/`) — to play

- **Fallout 4** Next-Gen (1.10.980+) or Anniversary (1.11.x). Game Pass / Microsoft Store won't work (no F4SE).

**Required mods** (install in this order):

| Mod | What for | Where |
|-----|----------|-------|
| **F4SE** (Fallout 4 Script Extender) | Loads the plugin. Launch the game with `f4se_loader.exe`. | https://f4se.silverlock.org |
| **Address Library for F4SE** | Game addresses (drop the `.bin` for your version into `Data\F4SE\Plugins\`). | https://www.nexusmods.com/fallout4/mods/47327 |
| **F4SE Menu Framework** | Draws the in-game menu (key `]`). | https://www.nexusmods.com/fallout4/mods/105090 |
| **F4MP** (this mod) | `F4MPClient.dll` → `Data\F4SE\Plugins\`. Runtime DLLs → next to `Fallout4.exe`. | Actions / Releases |

> Download F4SE for your **exact** game version, and the matching Address Library.

### Client — to build

- **Visual Studio 2022** with "Desktop development with C++".
- **xmake** (`winget install xmake`).
- The **CommonLibF4** submodule (`git submodule update --init --recursive`).
- Optional `XSE_FO4_GAME_PATH` to auto-deploy to the game. See [`client-ng/SETUP.md`](client-ng/SETUP.md).

### Server (`server/`) — to run

- **Windows or Linux**. On Windows, keep the runtime DLLs next to the `.exe` (bundled in the build / artifact).
- A `config.json` (created on first run): ip, port, `player-limit`, `server-name`, `master-server`, `tick-rate`.

### Server — to build

- **Windows**: **Visual Studio 2022** with C++ and **CMake**. Dependencies vendored in `third_party/win-x64/` — **no vcpkg needed**. Double-click `build_server.bat`.
- **Linux**: CMake + C++17 and `libgamenetworkingsockets-dev libspdlog-dev libcurl4-openssl-dev`. Run `./build_server_linux.sh`. See [`server/README.md`](server/README.md).

## Architecture

**Client (`client-ng/`)** — an **F4SE plugin** built on **CommonLibF4**, so it reads the game version-independently (no pattern scanning). It reads your position, sends it to the server ~20×/s from a network thread, receives the others', and spawns/moves a body per remote player. The in-game UI is drawn by **F4SE Menu Framework**.

**Server** — standalone executable using GameNetworkingSockets; opens a listen socket, tracks connected clients, and relays messages (positions, chat) between them.

**Protocol** — binary messages: `[MessageHeader: type(2) + size(4) + timestamp(8)][Payload]`. Types: `ConnectionRequest`, `ConnectionAccepted`, `PlayerPosition`, `ChatMessage`, `DamageDealt`, …

## Known limitations

- Bodies are placeholder Protectrons (can be hostile). Human/passive body pending.
- No animation, inventory or quest synchronization yet.
- No anti-cheat.

## License

Original F4MP code: Public Domain (Unlicense). This port keeps the same license. See `LICENSE`.

## Credits

Original F4MP team (Alin Octavian, Benjamin Kyd) · CommonLibF4 (libxse) · F4SE Menu Framework (DCCStudios) · Valve GameNetworkingSockets · spdlog (gabime) · nlohmann/json.

---

<a name="español"></a>

# F4MP — Fallout 4 Multiplayer (Español)

*[English](#english) · [Español](#español)*

Un mod multijugador para **Fallout 4**, resucitado y portado a **Next-Gen (1.10.980+)** y **Anniversary Edition (1.11.x)**.

> El proyecto original se abandonó en 2021. Esta es una versión resucitada y mejorada.

## Estado del proyecto

🟢 **Funciona (fase temprana), aún no es multijugador completo.** El mod carga en Fallout 4 por F4SE, se conecta a un servidor dedicado, sincroniza tu posición y muestra a los demás jugadores como **cuerpos (NPC) reales en el mundo que se mueven, se interpolan de forma suave y miran hacia donde mira el jugador** — validado con dos clientes reales. Los cuerpos son Protectrons de prueba; el siguiente paso es un cuerpo humano/pasivo.

El cliente se reescribió sobre **CommonLibF4** (ver [`client-ng/`](client-ng/)) tras comprobar que el pattern scanning original no localizaba bien al jugador. El cliente antiguo por inyección/ASI (`client/`) está en desuso.

Consulta **[CHECKLIST.md](CHECKLIST.md)** para la hoja de ruta completa.

| Área | Estado |
|------|--------|
| El cliente carga por F4SE (CommonLibF4) | ✅ Hecho |
| Menú in-game (F4SE Menu Framework) | ✅ Hecho |
| Conectar al servidor + handshake | ✅ Hecho |
| Leer y enviar la posición del jugador local | ✅ Hecho |
| Recibir a los demás + marcadores en el mundo | ✅ Hecho |
| Spawnear cuerpos (NPC) de jugador | ✅ Hecho (Protectron de prueba) |
| Interpolación suave + rotación hacia la vista | ✅ Hecho |
| Eliminar el cuerpo al desconectar el jugador | ✅ Hecho |
| Cuerpo humano/pasivo (no un robot hostil) | 🟡 En progreso |
| Servidor (poll groups, consola admin, relay chat/pos) | ✅ Hecho |
| Simulador de prueba (`F4MPSim`) | ✅ Hecho |

## Requisitos

### Cliente (`client-ng/`) — para jugar

- **Fallout 4** Next-Gen (1.10.980+) o Anniversary (1.11.x). No sirve Game Pass / Microsoft Store (sin F4SE).

**Mods necesarios** (instalar en este orden):

| Mod | Para qué | Dónde |
|-----|----------|-------|
| **F4SE** (Fallout 4 Script Extender) | Carga el plugin. Arranca el juego con `f4se_loader.exe`. | https://f4se.silverlock.org |
| **Address Library for F4SE** | Direcciones del juego (el `.bin` de tu versión → `Data\F4SE\Plugins\`). | https://www.nexusmods.com/fallout4/mods/47327 |
| **F4SE Menu Framework** | Dibuja el menú in-game (tecla `]`). | https://www.nexusmods.com/fallout4/mods/105090 |
| **F4MP** (este mod) | `F4MPClient.dll` → `Data\F4SE\Plugins\`. DLLs de runtime → junto a `Fallout4.exe`. | Actions / Releases |

> Descarga F4SE de tu **versión exacta**, y el Address Library a juego con esa versión.

### Cliente — compilar

- **Visual Studio 2022** con "Desarrollo para el escritorio con C++".
- **xmake** (`winget install xmake`).
- El submódulo **CommonLibF4** (`git submodule update --init --recursive`).
- Variable opcional `XSE_FO4_GAME_PATH` para desplegar al juego. Ver [`client-ng/SETUP.md`](client-ng/SETUP.md).

### Servidor (`server/`) — ejecutar

- **Windows o Linux**. En Windows, junto al `.exe` los DLLs de runtime (van incluidos en el build / artifact).
- Un `config.json` (se crea la primera vez): ip, puerto, `player-limit`, `server-name`, `master-server`, `tick-rate`.

### Servidor — compilar

- **Windows**: **Visual Studio 2022** con C++ y **CMake**. Dependencias ya incluidas en `third_party/win-x64/` — **no hace falta vcpkg**. Doble clic en `build_server.bat`.
- **Linux**: CMake + C++17 y `libgamenetworkingsockets-dev libspdlog-dev libcurl4-openssl-dev`. Ejecuta `./build_server_linux.sh`. Ver [`server/README.md`](server/README.md).

## Arquitectura

**Cliente (`client-ng/`)** — un **plugin de F4SE** sobre **CommonLibF4**, así lee el juego de forma independiente de la versión (sin pattern scanning). Lee tu posición, la envía al servidor ~20 veces/s desde un hilo de red, recibe la de los demás y spawnea/mueve un cuerpo por cada jugador remoto. El menú in-game lo dibuja **F4SE Menu Framework**.

**Servidor** — ejecutable independiente con GameNetworkingSockets; abre un listen socket, gestiona los clientes conectados y hace de relay de mensajes (posiciones, chat) entre ellos.

**Protocolo** — mensajes binarios: `[MessageHeader: type(2) + size(4) + timestamp(8)][Payload]`. Tipos: `ConnectionRequest`, `ConnectionAccepted`, `PlayerPosition`, `ChatMessage`, `DamageDealt`, …

## Limitaciones conocidas

- Los cuerpos son Protectrons de prueba (pueden ser hostiles). Cuerpo humano/pasivo pendiente.
- Aún no hay sincronización de animaciones, inventario ni misiones.
- Sin anti-cheat.

## Licencia

Código original de F4MP: Dominio público (Unlicense). Este port mantiene la misma licencia. Ver `LICENSE`.

## Créditos

Equipo original de F4MP (Alin Octavian, Benjamin Kyd) · CommonLibF4 (libxse) · F4SE Menu Framework (DCCStudios) · Valve GameNetworkingSockets · spdlog (gabime) · nlohmann/json.
