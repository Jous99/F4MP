<!-- Language: English (top) · Español (below) -->

# F4MP — Development checklist

*[English](#english) · [Español](#español)*

<a name="english"></a>

Project status. Legend: ✅ done · 🟡 partial · ⬜ pending

## New client — `client-ng/` (F4SE + CommonLibF4)  ← the one in use

Rebuilt on the CommonLibF4 template (xmake). This is the main client.

- [x] F4SE plugin with CommonLibF4 (loads via F4SE, no "incompatible")
- [x] In-game menu with **F4SE Menu Framework** (F4MP section: connect/disconnect/status)
- [x] **Read the local player position** (`PlayerCharacter::GetSingleton()->GetPosition()`)
- [x] Networking (`NetworkClient`, GameNetworkingSockets): connect, handshake, send position
- [x] **Send your position** to the server (~20×/s from a network thread)
- [x] **Receive other players'** positions
- [x] **In-world markers** for remote players (world→screen, name + distance + height)
- [x] **Spawn real bodies/NPCs** for remote players (placeholder Protectron)
- [x] **Interpolation** of remote players (smooth movement)
- [x] **Rotation** — body faces where the player is looking (sent over the network)
- [x] **Cleanup** — body is removed when a player disconnects
- [ ] Human / passive body (not a hostile robot)
- [ ] Animation synchronization
- [ ] Combat / damage synchronization

## Server — `server/` (F4MPServer.exe)

- [x] Loads `config.json`, logs to console + file, `server-name`
- [x] `GameServer` (GameNetworkingSockets): listen socket, poll groups, `RunCallbacks`
- [x] Accept connections via callback, client map, `player-limit` from config
- [x] Relay chat and position between clients
- [x] 30-tick loop
- [x] **Admin console** (Rust-style): startup info, periodic status, commands (`help`, `status`, `say`, `kick`, `stop`)
- [x] **Heartbeat** to the master server (`POST /heartbeat`)
- [ ] Rotation / animation / damage handlers
- [ ] Working ban list

## Tools

- [x] **Simulator** (`tools/simulator/`, `F4MPSim.exe`): connects N test bots; circular or fixed position (`F4MPSim host port count [x y z]`)

## Old client — `client/` (injection / ASI)  ← deprecated

First client (pattern scanning + custom D3D hook). It worked (load, menu, net), but the pattern scanning couldn't reliably locate the player. **Replaced by `client-ng`.**

## Infrastructure

- [x] Dependencies vendored in the repo (`third_party/win-x64`), no vcpkg
- [x] GitHub Actions workflows: `build-client`, `build-server`, `build-client-ng`, `make-deps`
- [x] Web + master server (single Node project; not committed to git by choice)
- [ ] Shared protocol header (today duplicated between client/server)
- [ ] Automated tests

## What's next

1. **Human / passive body** so the placeholder Protectron isn't a hostile robot.
2. **Animation sync** (walk/run/sneak poses).
3. **Combat / damage**.

---

<a name="español"></a>

# F4MP — Checklist de desarrollo (Español)

*[English](#english) · [Español](#español)*

Estado del proyecto. Leyenda: ✅ hecho · 🟡 parcial · ⬜ pendiente

## Cliente NUEVO — `client-ng/` (F4SE + CommonLibF4)  ← el que se usa

Reescritura sobre la plantilla CommonLibF4 (xmake). Es el cliente principal.

- [x] Plugin de F4SE con CommonLibF4 (carga por F4SE, sin "incompatible")
- [x] Menú in-game con **F4SE Menu Framework** (sección F4MP: conectar/desconectar/estado)
- [x] **Leer la posición del jugador local** (`PlayerCharacter::GetSingleton()->GetPosition()`)
- [x] Red (`NetworkClient`, GameNetworkingSockets): conectar, handshake, enviar posición
- [x] **Enviar tu posición** al servidor (~20 veces/s desde un hilo de red)
- [x] **Recibir las posiciones de los demás** jugadores
- [x] **Marcadores en el mundo** de los jugadores remotos (mundo→pantalla, nombre + distancia + altura)
- [x] **Spawnear cuerpos/NPC reales** para los jugadores remotos (Protectron de prueba)
- [x] **Interpolación** de jugadores remotos (movimiento suave)
- [x] **Rotación** — el cuerpo mira hacia donde mira el jugador (se envía por la red)
- [x] **Limpieza** — el cuerpo se elimina cuando un jugador se desconecta
- [ ] Cuerpo humano/pasivo (no un robot hostil)
- [ ] Sincronización de animaciones
- [ ] Sincronización de combate / daño

## Servidor — `server/` (F4MPServer.exe)

- [x] Carga de `config.json`, logger a consola + archivo, `server-name`
- [x] `GameServer` (GameNetworkingSockets): listen socket, poll groups, `RunCallbacks`
- [x] Aceptar conexiones vía callback, mapa de clientes, `player-limit` por config
- [x] Relay de chat y de posición entre clientes
- [x] Bucle a 30 ticks
- [x] **Consola de administración** estilo Rust: info de arranque, estado periódico, comandos (`help`, `status`, `say`, `kick`, `stop`)
- [x] **Heartbeat** al master server (`POST /heartbeat`)
- [ ] Handlers de rotación, animaciones y daño
- [ ] Lista de baneos funcional

## Herramientas

- [x] **Simulador** (`tools/simulator/`, `F4MPSim.exe`): conecta N bots de prueba; posición en círculo o fija (`F4MPSim host puerto count [x y z]`)

## Cliente ANTIGUO — `client/` (inyección / ASI)  ← en desuso

Primer cliente (pattern scanning + hook D3D propio). Funcionaba (carga, menú, red), pero el pattern scanning no localizaba bien al jugador. **Sustituido por `client-ng`.**

## Infraestructura

- [x] Dependencias vendidas en el repo (`third_party/win-x64`), sin vcpkg
- [x] Workflows de GitHub Actions: `build-client`, `build-server`, `build-client-ng`, `make-deps`
- [x] Web + master server (un solo proyecto Node; no se sube a git por decisión)
- [ ] Header de protocolo compartido (hoy duplicado entre cliente/servidor)
- [ ] Tests automáticos

## Lo que queda

1. **Cuerpo humano/pasivo** para que el Protectron de prueba no sea un robot hostil.
2. **Sincronización de animaciones** (poses de andar/correr/agachado).
3. **Combate / daño**.
