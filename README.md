# F4MP - Fallout 4 Multiplayer Revived

A multiplayer mod for Fallout 4, ported and rebuilt for Next-Gen (1.10.980+) and Anniversary Edition (1.11.x).

## Status

This is a complete rewrite of the original F4MP project which was deprecated in 2021. The original codebase was essentially a skeleton with no actual networking or server functionality implemented.

## Features (Planned/In Progress)

- F4SE plugin architecture (no raw DLL injection)
- Pattern scanning for version-independent address resolution
- Real-time multiplayer via GameNetworkingSockets
- Player position, rotation, and animation sync
- Combat and damage synchronization
- Chat system with UI overlay
- Server with connection management and world state tracking
- ImGui-based overlay menu (Delete key to toggle)

## Requirements

- **Fallout 4** Next-Gen (1.10.980+) or Anniversary Edition (1.11.x)
- **F4SE** 0.6.23+ (for the client plugin)
- **Visual Studio 2022** with C++ desktop development workload
- **CMake** 3.20+
- **vcpkg** for dependencies

## Building

### 1. Setup vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install --triplet x64-windows
```

### 2. Configure and Build

```powershell
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### 3. Install

**Client:**
- Copy `bin/F4MPClient.dll` to `<Fallout4>/Data/F4SE/Plugins/`

**Server:**
- Run `bin/F4MPServer.exe`

## Architecture

```
F4MP/
├── common/          # Shared library (logging, config, protocol, pattern scanning)
├── client/          # F4SE plugin (hooks, networking, UI)
├── server/          # Dedicated server executable
└── f4se/            # F4SE SDK headers
```

## Protocol

Communication uses a binary message protocol over GameNetworkingSockets:

- Connection management (handshake, accept/reject, disconnect)
- Player state (position, rotation, animation)
- Combat events (damage, death, projectiles)
- World state (entity spawn/despawn, object state)
- Chat messages

## Differences from Original F4MP

| Aspect | Original F4MP | F4MP Revived |
|--------|---------------|--------------|
| Loading | Raw DLL injection | F4SE plugin |
| Addresses | Hardcoded offsets | Pattern scanning |
| Networking | Headers only, no code | Full GameNetworkingSockets impl |
| Server | Empty stubs | Functional server with client management |
| C++ Standard | C++17 | C++20 |
| Dependencies | Pre-compiled libs | vcpkg managed |
| ImGui | 1.67 WIP (2019) | Latest stable |

## License

Original F4MP code was released to the public domain. This revived version is also public domain.

## Credits

- Original F4MP team for the initial concept
- Ian Patterson for F4SE
- Valve for GameNetworkingSockets
- ocornut for Dear ImGui
