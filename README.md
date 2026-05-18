# F4MP - Fallout 4 Multiplayer

A multiplayer mod for Fallout 4, ported to work with Next-Gen (1.10.980+) and Anniversary Edition (1.11.x).

> **Original project was deprecated in 2021.** This is a revived and improved version.

## Changes from Original

| Feature | Original | Ported |
|---------|----------|--------|
| Game version | Pre-Next-Gen (1.10.163) | Next-Gen 1.10.980+ / Anniversary 1.11.x |
| Address resolution | Hardcoded offsets (breaks on update) | Pattern scanning (version-independent) |
| Networking | Headers present, zero code | Full GameNetworkingSockets implementation |
| Server | Empty stubs (Start/Update return nothing) | Functional with connection management, broadcast, chat |
| Print hook | Bug: printed va_list pointer | Fixed: properly formats and prints strings |
| Server fork() | Syntax error: `pid_t Pid fork();` | Fixed: `pid_t pid = fork();` |
| Server main loop | `UMain()` returns 0 immediately | Full game loop with 30 tick rate |
| ImGui | 1.67 WIP demo window only | Integrated menu with connect/disconnect UI |
| CMake | `dxgi.dll` linked as lib | Fixed to `dxgi.lib` |

## Building

### Prerequisites

- Visual Studio 2019/2022 with C++ desktop development
- CMake 3.7+
- All dependencies are included in `lib/` and `include/`

### Client

1. Open Visual Studio
2. File -> Open -> CMake -> select `client/CMakeLists.txt`
3. Build -> Build All (x64-Release)
4. Output: `out/build/x64-Release/F4MPClient.dll`

### Server

1. Open Visual Studio
2. File -> Open -> CMake -> select `server/CMakeLists.txt`
3. Build -> Build All (x64-Release)
4. Output: `out/build/x64-Release/F4MPServer.exe`

## Installation

### Client

1. Place `F4MPClient.dll` in your Fallout 4 game directory
2. Inject using your preferred DLL injector, or set up as a load library mod
3. Launch Fallout 4
4. Press **Delete** to toggle the F4MP menu

### Server

1. Run `F4MPServer.exe`
2. A `config.json` will be created automatically on first run
3. Edit `config.json` to change port, player limit, etc.
4. Default: `127.0.0.1:7779`

## Usage

1. Start the server first
2. Launch Fallout 4 with the client DLL loaded
3. Press **Delete** to open the F4MP menu
4. Enter server address and click **Connect**
5. Status indicator shows connection state

## Architecture

### Client
- **DLL injection** into Fallout4.exe
- **D3D11 Present hook** via Microsoft Detours for ImGui overlay
- **Pattern scanning** to resolve game addresses at runtime (no hardcoded offsets)
- **GameNetworkingSockets** for reliable UDP networking
- **ImGui** for in-game UI

### Server
- **Standalone executable** using GameNetworkingSockets
- **Listen socket** for accepting client connections
- **Message routing** - broadcasts player positions, chat, etc.
- **Client management** - tracks connected players, handles disconnects

### Protocol
Messages use a binary format:
```
[MessageHeader: type(2) + size(4) + timestamp(8)]
[Payload: variable]
```

Message types: ConnectionRequest, ConnectionAccepted, PlayerPosition, ChatMessage, DamageDealt, etc.

## Known Issues

- Pattern signatures may need updating for future game patches
- No entity/NPC synchronization yet (players only)
- No inventory or quest synchronization
- Anti-cheat not implemented

## License

Original F4MP code: Public Domain (Unlicense)
This port: Same license as original

## Credits

- Original F4MP team (Alin Octavian, Benjamin Kyd)
- Microsoft Detours
- Valve GameNetworkingSockets
- Dear ImGui (ocornut)
- spdlog (gabime)
- nlohmann/json
