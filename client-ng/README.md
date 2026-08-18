<!-- Language: English (top) · Español (below) -->

# F4MP — Client (CommonLibF4)

*[English](#english) · [Español](#español)*

<a name="english"></a>

The **F4MP** client, rebuilt as an **F4SE** plugin on top of **CommonLibF4**. This is the main client (it replaces the old `../client/`, which used injection + pattern scanning and couldn't locate the player reliably).

It loads via F4SE, reads your position from the game without guessing addresses, connects to the server, and shows the other players.

## Status

| Feature | State |
|---------|-------|
| Loads as an F4SE plugin (CommonLibF4) | ✅ |
| In-game menu (F4SE Menu Framework, key `]`) | ✅ |
| Read the player position and send it to the server | ✅ |
| Receive the other players' positions | ✅ |
| Floating world markers for remote players | ✅ |
| Test-spawn an actor ("Spawn dummy" button) | ✅ |
| A body (NPC) per remote player, moving | ✅ |
| Interpolation (smooth movement, no stutter) | ✅ |
| Cleanup of the body when the player disconnects | ✅ |
| Rotation (the body faces where the player looks) | ✅ |
| Human/passive body (it's a placeholder Protectron now) | 🟡 in progress |
| Animations / combat / inventory | ⬜ |

## How it works

- **CommonLibF4** gives version-independent access to the game: `PlayerCharacter::GetSingleton()->GetPosition()`, `Console::ExecuteCommand`, `HUDMenuUtils::WorldPtToScreenPt3`, `SetAngleOnReference`, etc.
- **NetworkClient** (GameNetworkingSockets) connects to the server, sends your position ~20×/s from a network thread and receives everyone else's.
- **F4SE Menu Framework** draws the menu and HUD (we don't hook DirectX ourselves).
- Bodies are spawned via `player.placeatme`, then moved each tick with `SetPosition` (interpolated) and oriented with `SetAngleOnReference` toward the received look heading.

## Build

Summary (full guide in [`SETUP.md`](SETUP.md)):

```bat
git submodule update --init --recursive
setx XSE_FO4_GAME_PATH "D:\SteamLibrary\steamapps\common\Fallout 4"
cd client-ng
xmake build
```

With `XSE_FO4_GAME_PATH` set, xmake auto-deploys the DLL to `Data\F4SE\Plugins\`.

## Requirements (to play)

- **Fallout 4** Next-Gen (1.10.980+) or Anniversary (1.11.x).
- **F4SE** for your version + **Address Library for F4SE**.
- **F4SE Menu Framework** (for the menu).

See the mods table with links in the [root README](../README.md#english).

## In-game usage

1. Launch with `f4se_loader.exe` and load a save.
2. Open the menu with **`]`** → **F4MP** section.
3. Enter your name, the server IP and port, and **Connect**.
4. Other players appear as bodies in the world (plus a marker with name + distance).

The plugin log is at `Documents\My Games\Fallout4\F4SE\F4MP.log`.

---

<a name="español"></a>

# F4MP — Cliente (CommonLibF4)

*[English](#english) · [Español](#español)*

Cliente de **F4MP** reescrito como plugin de **F4SE** sobre **CommonLibF4**. Es el cliente principal (sustituye al antiguo `../client/`, que usaba inyección + pattern scanning y no localizaba bien al jugador).

Carga por F4SE, lee tu posición del juego sin adivinar direcciones, se conecta al servidor y muestra a los demás jugadores.

## Estado

| Función | Estado |
|---------|--------|
| Carga como plugin de F4SE (CommonLibF4) | ✅ |
| Menú in-game (F4SE Menu Framework, tecla `]`) | ✅ |
| Leer la posición del jugador y enviarla al servidor | ✅ |
| Recibir la posición de los demás jugadores | ✅ |
| Marcadores flotantes de jugadores remotos (mundo→pantalla) | ✅ |
| Spawn de un actor de prueba (botón "Spawn dummy") | ✅ |
| Cuerpos (NPC) por jugador remoto, moviéndose | ✅ |
| Interpolación (movimiento suave, sin trompicones) | ✅ |
| Limpieza del cuerpo al desconectar el jugador | ✅ |
| Rotación (el cuerpo mira hacia donde mira el jugador) | ✅ |
| Cuerpo humano/pasivo (ahora es un Protectron de prueba) | 🟡 en progreso |
| Animaciones / combate / inventario | ⬜ |

## Cómo funciona

- **CommonLibF4** da acceso independiente de la versión al juego: `PlayerCharacter::GetSingleton()->GetPosition()`, `Console::ExecuteCommand`, `HUDMenuUtils::WorldPtToScreenPt3`, `SetAngleOnReference`, etc.
- **NetworkClient** (GameNetworkingSockets) conecta al servidor, envía tu posición ~20 veces/s desde un hilo de red y recibe la de los demás.
- **F4SE Menu Framework** dibuja el menú y el HUD (no montamos nuestro propio hook de DirectX).
- Los cuerpos se spawnean con `player.placeatme`, luego se mueven cada tick con `SetPosition` (interpolado) y se orientan con `SetAngleOnReference` hacia el ángulo de vista recibido.

## Compilar

Resumen (guía completa en [`SETUP.md`](SETUP.md)):

```bat
git submodule update --init --recursive
setx XSE_FO4_GAME_PATH "D:\SteamLibrary\steamapps\common\Fallout 4"
cd client-ng
xmake build
```

Con `XSE_FO4_GAME_PATH` definido, xmake despliega el DLL solo en `Data\F4SE\Plugins\`.

## Requisitos (jugar)

- **Fallout 4** Next-Gen (1.10.980+) o Anniversary (1.11.x).
- **F4SE** de tu versión + **Address Library for F4SE**.
- **F4SE Menu Framework** (para el menú).

Ver la tabla de mods con enlaces en el [README raíz](../README.md#español).

## Uso en el juego

1. Arranca con `f4se_loader.exe` y carga una partida.
2. Abre el menú con la tecla **`]`** → sección **F4MP**.
3. Pon tu nombre, la IP y el puerto del servidor, y **Conectar**.
4. Los demás jugadores aparecen como cuerpos en el mundo (además de un marcador con nombre + distancia).

El log del plugin está en `Documents\My Games\Fallout4\F4SE\F4MP.log`.
