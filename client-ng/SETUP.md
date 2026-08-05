# F4MP-NG — cliente sobre CommonLibF4

Reescritura del cliente sobre **CommonLibF4** (la librería estándar de RE para
Fallout 4). Con ella accedemos al jugador sin adivinar direcciones, y carga por
F4SE correctamente.

## Hito actual

**Hito 1:** compilar y **leer la posición del jugador** correctamente (lo que el
pattern scanning no lograba). Luego añadiremos red y menú.

## Requisitos (instalar una vez)

1. **xmake** (herramienta de build): en una terminal:
   ```
   winget install xmake
   ```
   o desde https://xmake.io
2. **Address Library for F4SE** — descárgalo del Nexus (el de tu versión, Next-Gen)
   y coloca su `.bin` en `Data\F4SE\Plugins\`. CommonLibF4 lo usa para resolver
   las direcciones del juego.
3. Tener **F4SE** instalado (ya lo tienes).

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

> La primera vez descarga y compila CommonLibF4 y sus dependencias: tarda un rato.
> El plugin se despliega solo en `Data\F4SE\Plugins\F4MPClient.dll`.

## Probar

1. **Quita la config vieja de ASI** para que no choque: borra `F4MPClient.asi` y
   `dinput8.dll` de la carpeta del juego (esto nuevo carga por F4SE, no por ASI).
2. Arranca el juego con **`f4se_loader.exe`**.
3. Carga una partida y muévete.
4. Mira el log en:
   ```
   Documents\My Games\Fallout4\F4SE\F4MP.log
   ```
   Debe mostrar líneas tipo:
   ```
   [F4MP] Jugador -> x=-74982.0  y=81859.0  z=7547.2
   ```
   con TUS coordenadas reales. Si salen bien → hito 1 conseguido.

## Siguientes hitos

- **Hito 2:** traer la red (NetworkClient) y enviar la posición al servidor.
- **Hito 3:** menú in-game (conectar/desconectar, chat).
