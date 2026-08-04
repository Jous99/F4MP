# F4MPServer

The dedicated server for **F4MP**. A standalone Windows executable that accepts client connections over GameNetworkingSockets and relays messages (positions, chat) between players.

> ⚠️ Work in progress. Connection handling and chat/position relay work; higher-level game sync is still pending. See [`../CHECKLIST.md`](../CHECKLIST.md).

## Requirements

- **Windows** (Linux support is disabled for now).
- To build: **Visual Studio 2022** with "Desktop development with C++". Dependencies are vendored in the repo (`../third_party/deps`), so **vcpkg is not required**.

## Building

Easiest — from the repo root, double-click **`build_server.bat`**. It sets up the Visual Studio environment, builds with Ninja and copies the runtime DLLs next to the binary.

Manual:

```bash
cmake -S server -B server/build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build server/build
```

Output: `server/build/F4MPServer.exe`

You can also grab a prebuilt binary from the **Actions** tab (artifact `F4MPServer`) or from a tagged **Release**.

## Running

1. Put `F4MPServer.exe` next to the GameNetworkingSockets runtime DLLs (the build scripts and CI artifact bundle them).
2. Run it. On first launch it creates a `config.json` and exits — edit it, then run again.
3. Press **Ctrl+C** to stop.

## Configuration (`config.json`)

| Key | Meaning | Default |
|-----|---------|---------|
| `ip` | Bind/display address | `127.0.0.1` |
| `port` | UDP port | `7779` |
| `player-limit` | Max simultaneous players | `100` |
| `run-as-service` | Reserved (runs in console mode for now) | `false` |
| `log-location` | Log file path, or `NONE` | `./logs.log` |

## Architecture

GameNetworkingSockets listen socket + poll group · connection state handled via the global status callback + `RunCallbacks()` · a 30-tick main loop routes messages between clients. See the root [`README.md`](../README.md) for the full picture.
