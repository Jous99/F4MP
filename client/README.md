<!-- Language: English (top) · Español (below) -->

# F4MPClient  (DEPRECATED)

*[English](#english) · [Español](#español)*

<a name="english"></a>

> 🛑 **Deprecated.** This is the original injected/ASI client (custom pattern scanning + D3D11 hook). Its pattern scanning couldn't reliably locate the player, so it was **replaced by [`../client-ng/`](../client-ng/)**, a proper F4SE plugin built on CommonLibF4. Use that one. This folder is kept for reference.

The client half of **F4MP** — a DLL that loads into Fallout 4, draws an in-game menu (Dear ImGui over a D3D11 hook) and talks to an F4MP server over GameNetworkingSockets.

## Requirements

- **Fallout 4** Next-Gen (1.10.980+) or Anniversary Edition (1.11.x). Addresses are resolved at runtime by pattern scanning.
- To build: **Visual Studio 2022** with "Desktop development with C++". Dependencies are vendored in the repo (`../third_party/win-x64`), so **vcpkg is not required**.

## Building

Easiest — from the repo root, double-click **`build_client.bat`**. Manual:

```bash
cmake -S client -B client/build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build client/build
```

Output: `client/build/F4MPClient.dll`

## Loading into the game

- **Option A — ASI loader.** The DLL bootstraps from `DllMain`. Use Ultimate ASI Loader as a `winmm.dll`/`dinput8.dll` proxy (not `dxgi`/`d3d11`, which the mod hooks), rename `F4MPClient.dll` to `.asi`, launch.
- **Option B — F4SE plugin.** It also exports the F4SE entry points; put it in `Data/F4SE/Plugins/` and the runtime DLLs next to `Fallout4.exe`. Do **not** copy our `steam_api64.dll` into the game root.
- **Option C — manual injection.**

## Controls

- **Delete** — toggle the F4MP menu.

## Architecture

D3D11 `Present` hook via Microsoft Detours for the ImGui overlay · runtime address resolution by pattern scanning · GameNetworkingSockets for networking. See the root [`README.md`](../README.md).

---

<a name="español"></a>

# F4MPClient  (EN DESUSO)

*[English](#english) · [Español](#español)*

> 🛑 **En desuso.** Este es el cliente original por inyección/ASI (pattern scanning propio + hook de D3D11). Su pattern scanning no localizaba bien al jugador, así que fue **sustituido por [`../client-ng/`](../client-ng/)**, un plugin de F4SE en condiciones sobre CommonLibF4. Usa ese. Esta carpeta se conserva como referencia.

La mitad cliente de **F4MP** — una DLL que carga en Fallout 4, dibuja un menú in-game (Dear ImGui sobre un hook de D3D11) y habla con un servidor de F4MP por GameNetworkingSockets.

## Requisitos

- **Fallout 4** Next-Gen (1.10.980+) o Anniversary Edition (1.11.x). Las direcciones se resuelven en tiempo de ejecución por pattern scanning.
- Para compilar: **Visual Studio 2022** con "Desarrollo para el escritorio con C++". Dependencias incluidas en el repo (`../third_party/win-x64`), así que **no hace falta vcpkg**.

## Compilar

Lo más fácil — desde la raíz del repo, doble clic en **`build_client.bat`**. Manual:

```bash
cmake -S client -B client/build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build client/build
```

Salida: `client/build/F4MPClient.dll`

## Cargar en el juego

- **Opción A — cargador ASI.** La DLL arranca desde `DllMain`. Usa Ultimate ASI Loader como proxy `winmm.dll`/`dinput8.dll` (no `dxgi`/`d3d11`, que el mod hookea), renombra `F4MPClient.dll` a `.asi` y arranca.
- **Opción B — plugin de F4SE.** También exporta los puntos de entrada de F4SE; ponlo en `Data/F4SE/Plugins/` y los DLLs de runtime junto a `Fallout4.exe`. **No** copies nuestro `steam_api64.dll` a la raíz del juego.
- **Opción C — inyección manual.**

## Controles

- **Supr** — abre/cierra el menú de F4MP.

## Arquitectura

Hook de `Present` de D3D11 con Microsoft Detours para el overlay de ImGui · resolución de direcciones en runtime por pattern scanning · GameNetworkingSockets para la red. Ver el [`README.md`](../README.md) raíz.
