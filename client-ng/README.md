# F4MP — Cliente (CommonLibF4)

Cliente de **F4MP** reescrito como plugin de **F4SE** sobre **CommonLibF4**. Es el
cliente principal (sustituye al antiguo `../client/`, que usaba inyección + pattern
scanning y no localizaba bien al jugador).

Carga por F4SE, lee tu posición del juego sin adivinar direcciones, se conecta al
servidor y muestra a los demás jugadores.

## Estado

| Función | Estado |
|---------|--------|
| Carga como plugin de F4SE (CommonLibF4) | ✅ |
| Menú in-game (F4SE Menu Framework, tecla `]`) | ✅ |
| Leer la posición del jugador y enviarla al servidor | ✅ |
| Recibir la posición de los demás jugadores | ✅ |
| Marcadores flotantes de jugadores remotos (mundo→pantalla) | ✅ |
| Spawn de un actor de prueba (botón "Spawn dummy") | ✅ |
| Cuerpos (NPC) por jugador remoto, moviéndose | 🟡 en progreso |
| Rotación / animaciones / combate | ⬜ |

## Cómo funciona

- **CommonLibF4** da acceso version-independent al juego: `PlayerCharacter::GetSingleton()->GetPosition()`, `Console::ExecuteCommand`, `HUDMenuUtils::WorldPtToScreenPt3`, etc.
- **NetworkClient** (GameNetworkingSockets) conecta al servidor, envía tu posición ~10/s desde un hilo de red y recibe la de los demás.
- **F4SE Menu Framework** dibuja el menú y el HUD (no montamos nuestro propio hook de DirectX).

## Estructura

```
client-ng/
├── xmake.lua                 Build (xmake) + regla del plugin CommonLibF4
├── src/
│   ├── pch.h                 Cabeceras precompiladas (RE/Fallout.h, F4SE)
│   ├── main.cpp              Plugin: carga, menú, HUD, spawn, hilo de red
│   ├── F4SEMenuFramework.h   Header del framework de menús (GetProcAddress)
│   └── network/
│       ├── NetworkClient.h
│       └── NetworkClient.cpp
├── lib/commonlibf4           Submódulo CommonLibF4
├── SETUP.md                  Guía de montaje y compilación paso a paso
└── README.md                 (este archivo)
```

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

Ver la tabla de mods con enlaces en el [README raíz](../README.md#requisitos).

## Uso en el juego

1. Arranca con `f4se_loader.exe` y carga una partida.
2. Abre el menú con la tecla **`]`** → sección **F4MP**.
3. Pon tu nombre, la IP y el puerto del servidor, y **Conectar**.
4. Los demás jugadores aparecen como marcadores en el mundo (nombre + distancia).

El log del plugin está en `Documents\My Games\Fallout4\F4SE\F4MP.log`.
