# F4MP — Fallout 4 Multiplayer

A multiplayer mod for **Fallout 4**, revived and ported to work with **Next-Gen (1.10.980+)** and **Anniversary Edition (1.11.x)**.

> The original project was deprecated in 2021. This is a revived and improved version.

---

## Project status

🟢 **Working (early), not yet a full multiplayer experience.** The mod loads in Fallout 4 via F4SE, connects to a dedicated server, syncs your position, and shows other players as in-world markers. Actual player *bodies* (NPCs) are the remaining big step.

The client was rebuilt on **CommonLibF4** (see [`client-ng/`](client-ng/)) after the original pattern-scanning approach proved unreliable for locating the player. The old injected/ASI client (`client/`) is deprecated.

See **[CHECKLIST.md](CHECKLIST.md)** for the full roadmap.

| Area | State |
|------|-------|
| Client loads via F4SE (CommonLibF4) | ✅ Working |
| In-game menu (F4SE Menu Framework) | ✅ Working |
| Connect to server + handshake | ✅ Working |
| Read & send local player position | ✅ Working |
| Receive other players + in-world markers | ✅ Working |
| Spawn actual player bodies (NPCs) | ⬜ Pending (hard — native spawn / Papyrus) |
| Server (poll groups, admin console, chat/pos relay) | ✅ Working |
| Test simulator (`F4MPSim`) | ✅ Working |

## Requisitos

### Cliente (`client-ng/`) — jugar

- **Fallout 4** Next-Gen (1.10.980+) o Anniversary (1.11.x). No sirve Game Pass / Microsoft Store (sin F4SE).

**Mods necesarios** (instalar en este orden):

| Mod | Para qué | Dónde |
|-----|----------|-------|
| **F4SE** (Fallout 4 Script Extender) | Carga el plugin. Arranca el juego con `f4se_loader.exe`. | https://f4se.silverlock.org |
| **Address Library for F4SE** | Direcciones del juego (el `.bin` de tu versión → `Data\F4SE\Plugins\`). | https://www.nexusmods.com/fallout4/mods/47327 |
| **F4SE Menu Framework** | Dibuja el menú in-game (tecla `]`). | https://www.nexusmods.com/fallout4/mods/105090 |
| **F4MP** (este mod) | `F4MPClient.dll` → `Data\F4SE\Plugins\`. DLLs de runtime → junto a `Fallout4.exe`. | Actions / Releases |

> Descarga F4SE de tu **versión exacta** del juego, y el Address Library **a juego** con esa versión.

### Cliente — compilar

- **Visual Studio 2022** con "Desarrollo para el escritorio con C++".
- **xmake** (`winget install xmake`).
- El submódulo **CommonLibF4** (`git submodule update --init --recursive`).
- Variable `XSE_FO4_GAME_PATH` para desplegar al juego (opcional). Ver [`client-ng/SETUP.md`](client-ng/SETUP.md).

### Servidor (`server/`) — ejecutar

- **Windows**. Junto al `.exe`, los DLLs de runtime (van incluidos en el build / artifact).
- Un `config.json` (se crea solo la primera vez): ip, puerto, `player-limit`, `server-name`, `master-server`.

### Servidor — compilar

- **Visual Studio 2022** con C++ y **CMake** (viene con VS).
- Dependencias ya incluidas en `third_party/win-x64/` — **no hace falta vcpkg**. Doble clic en `build_server.bat`.

## What changed from the original

| Feature | Original | This port |
|---------|----------|-----------|
| Game version | Pre-Next-Gen (1.10.163) | Next-Gen 1.10.980+ / Anniversary 1.11.x |
| Address resolution | Hardcoded offsets (break on update) | Pattern scanning (version-independent) |
| Networking | Headers only, zero code | GameNetworkingSockets implementation |
| Server | Empty stubs | Connection management, broadcast, chat |
| Print hook | Printed the `va_list` pointer | Formats and prints strings correctly |
| Server `fork()` | Syntax error | Fixed |
| Server main loop | Returned immediately | 30-tick game loop |
| ImGui | 1.67 demo window only | Menu with connect/disconnect UI |

## Building

You don't have to build it yourself — every tagged release (`v*`) is compiled automatically by GitHub Actions and the binaries are attached under **[Releases](../../releases)**. To build locally:

**Prerequisites**

- Visual Studio 2019/2022 with the "Desktop development with C++" workload
- CMake 3.9+
- All third-party dependencies are already included in `lib/` and `include/`

**Client**

```bash
cmake -S client -B client/build -G "Visual Studio 17 2022" -A x64
cmake --build client/build --config RelWithDebInfo
# Output: client/build/RelWithDebInfo/F4MPClient.dll
```

**Server**

```bash
cmake -S server -B server/build -G "Visual Studio 17 2022" -A x64
cmake --build server/build --config RelWithDebInfo
# Output: server/build/RelWithDebInfo/F4MPServer.exe
```

You can also open either `CMakeLists.txt` directly in Visual Studio and build the `x64-Release` configuration.

## Installation

**Client**

1. Place `F4MPClient.dll` (and `steam_api64.dll`) in your Fallout 4 game directory.
2. Load it with your preferred DLL injector.
3. Launch Fallout 4.
4. Press **Delete** to toggle the F4MP menu.

**Server**

1. Run `F4MPServer.exe`.
2. A `config.json` is created automatically on first run.
3. Edit `config.json` to change IP, port, player limit, etc.
4. Default: `127.0.0.1:7779`.

## Usage

1. Start the server.
2. Launch Fallout 4 with the client DLL loaded.
3. Press **Delete** to open the menu.
4. Enter the server address and click **Connect**.

## Architecture

**Client** — DLL injected into `Fallout4.exe`; hooks `IDXGISwapChain::Present` (D3D11) via Microsoft Detours to draw a Dear ImGui overlay; resolves game addresses at runtime with pattern scanning; talks to the server over Valve's GameNetworkingSockets.

**Server** — standalone executable using GameNetworkingSockets; opens a listen socket, tracks connected clients, and routes messages (positions, chat) between them.

**Protocol** — binary messages:

```
[MessageHeader: type(2) + size(4) + timestamp(8)]
[Payload: variable]
```

Message types: `ConnectionRequest`, `ConnectionAccepted`, `PlayerPosition`, `ChatMessage`, `DamageDealt`, and more.

## Known limitations

- Pattern signatures may need updating for future game patches.
- No entity/NPC, inventory, or quest synchronization yet.
- No anti-cheat.

## License

Original F4MP code: Public Domain (Unlicense). This port keeps the same license. See `LICENSE`.

## Credits

Original F4MP team (Alin Octavian, Benjamin Kyd) · Microsoft Detours · Valve GameNetworkingSockets · Dear ImGui (ocornut) · spdlog (gabime) · nlohmann/json.
