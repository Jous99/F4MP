<!-- Language: English (top) · Español (below) -->

# F4MP-NG — setup & build (CommonLibF4)

*[English](#english) · [Español](#español)*

<a name="english"></a>

Client rebuilt on **CommonLibF4** (the standard RE library for Fallout 4). With it we access the player without guessing addresses, and it loads correctly via F4SE.

## Prerequisites (install once)

1. **xmake** (build tool):
   ```
   winget install xmake
   ```
   or from https://xmake.io
2. **Address Library for F4SE** — download it from Nexus (the one for your version) and put its `.bin` in `Data\F4SE\Plugins\`. CommonLibF4 uses it to resolve game addresses.
3. Have **F4SE** installed.

## Setup (once)

From the **repo root** (`C:\Users\Jous\F4MP`):

```bat
git submodule add https://github.com/libxse/commonlibf4.git client-ng/lib/commonlibf4
git submodule update --init --recursive
```

Set where to deploy the plugin (so xmake auto-copies it to the game):

```bat
setx XSE_FO4_GAME_PATH "D:\SteamLibrary\steamapps\common\Fallout 4"
```

(Open a **new** terminal after `setx` so it picks up the variable.)

## Build

```bat
cd client-ng
xmake build
```

> The first build downloads and compiles CommonLibF4 and its dependencies — it takes a while. The plugin auto-deploys to `Data\F4SE\Plugins\F4MPClient.dll`.

## Test

1. **Remove the old ASI setup** so it doesn't clash: delete `F4MPClient.asi` and `dinput8.dll` from the game folder (this build loads via F4SE, not ASI).
2. Launch the game with **`f4se_loader.exe`**.
3. Load a save and move around.
4. Check the log at:
   ```
   Documents\My Games\Fallout4\F4SE\F4MP.log
   ```
   Connect to a server; you should see your position being read and sent, remote players arriving, and a body being spawned/moved for each.

---

<a name="español"></a>

# F4MP-NG — montaje y compilación (CommonLibF4)

*[English](#english) · [Español](#español)*

Cliente reescrito sobre **CommonLibF4** (la librería estándar de RE para Fallout 4). Con ella accedemos al jugador sin adivinar direcciones, y carga por F4SE correctamente.

## Requisitos (instalar una vez)

1. **xmake** (herramienta de build):
   ```
   winget install xmake
   ```
   o desde https://xmake.io
2. **Address Library for F4SE** — descárgalo del Nexus (el de tu versión) y coloca su `.bin` en `Data\F4SE\Plugins\`. CommonLibF4 lo usa para resolver las direcciones del juego.
3. Tener **F4SE** instalado.

## Montaje (una vez)

Desde la **raíz del repo** (`C:\Users\Jous\F4MP`):

```bat
git submodule add https://github.com/libxse/commonlibf4.git client-ng/lib/commonlibf4
git submodule update --init --recursive
```

Y define a dónde desplegar el plugin (para que xmake lo copie solo al juego):

```bat
setx XSE_FO4_GAME_PATH "D:\SteamLibrary\steamapps\common\Fallout 4"
```

(Abre una terminal NUEVA después del `setx` para que tome la variable.)

## Compilar

```bat
cd client-ng
xmake build
```

> La primera vez descarga y compila CommonLibF4 y sus dependencias: tarda un rato. El plugin se despliega solo en `Data\F4SE\Plugins\F4MPClient.dll`.

## Probar

1. **Quita la config vieja de ASI** para que no choque: borra `F4MPClient.asi` y `dinput8.dll` de la carpeta del juego (esto nuevo carga por F4SE, no por ASI).
2. Arranca el juego con **`f4se_loader.exe`**.
3. Carga una partida y muévete.
4. Mira el log en:
   ```
   Documents\My Games\Fallout4\F4SE\F4MP.log
   ```
   Conéctate a un servidor; deberías ver cómo se lee y envía tu posición, cómo llegan los jugadores remotos y cómo se spawnea/mueve un cuerpo por cada uno.
