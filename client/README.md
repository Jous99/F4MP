# F4MPClient

The client half of **F4MP** — a DLL that loads into Fallout 4, draws an in-game menu (Dear ImGui over a D3D11 hook) and talks to an F4MP server over GameNetworkingSockets.

> ⚠️ Work in progress. The menu and networking work, but in-game player sync (seeing other players in the world) is not implemented yet. See [`../CHECKLIST.md`](../CHECKLIST.md).

## Requirements

- **Fallout 4** Next-Gen (1.10.980+) or Anniversary Edition (1.11.x). Addresses are resolved at runtime by pattern scanning, so the signatures target these versions.
- To build: **Visual Studio 2022** with "Desktop development with C++". Dependencies are already vendored in the repo (`../third_party/deps`), so **vcpkg is not required**.

## Building

Easiest — from the repo root, double-click **`build_client.bat`**. It sets up the Visual Studio environment, builds with Ninja and copies the runtime DLLs next to the binary.

Manual:

```bash
cmake -S client -B client/build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build client/build
```

Output: `client/build/F4MPClient.dll`

You can also grab a prebuilt DLL from the **Actions** tab (artifact `F4MPClient`) or from a tagged **Release**.

## Files needed at runtime

`F4MPClient.dll` must sit next to `steam_api64.dll` and the GameNetworkingSockets runtime DLLs (`GameNetworkingSockets.dll`, `libcrypto-3-x64.dll`, `libssl-3-x64.dll`, `libprotobuf.dll`, `abseil_dll.dll`, `fmt.dll`, `spdlog.dll`, …). The build scripts and the CI artifact already bundle all of these.

## Loading into the game

**Option A — ASI loader (no code changes).** The DLL bootstraps from `DllMain`, so an ASI loader works: drop Ultimate ASI Loader next to `Fallout4.exe` as a proxy (`winmm.dll` / `dinput8.dll` — not `dxgi`/`d3d11`, which the mod hooks), rename `F4MPClient.dll` to `F4MPClient.asi`, and launch the game.

**Option B — F4SE plugin (recommended).** The DLL now exports the F4SE plugin entry points, so it loads as a normal mod:

1. Install **F4SE** (the version matching your game — there is an F4SE build for the Next-Gen update).
2. Put `F4MPClient.dll` in `Data/F4SE/Plugins/`.
3. Put the runtime DLLs (`steam_api64.dll`, `GameNetworkingSockets.dll`, `libcrypto-3-x64.dll`, …) next to `Fallout4.exe` — Windows resolves a plugin's dependencies from the game root, not from the `Plugins` folder.
4. Launch the game **through F4SE** (`f4se_loader.exe`).

The plugin is declared version-independent (it uses pattern scanning), so F4SE loads it on any game version.

**Option C — manual injection.** Any DLL injector into `Fallout4.exe`.

## Controls

- **Delete** — toggle the F4MP menu (enter player name, server address/port, connect/disconnect).

## Architecture

D3D11 `Present` hook via Microsoft Detours for the ImGui overlay · runtime address resolution by pattern scanning · GameNetworkingSockets for networking. See the root [`README.md`](../README.md) for the full picture.
