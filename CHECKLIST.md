# F4MP — Checklist de desarrollo

Estado del proyecto. Leyenda: ✅ hecho · 🟡 parcial · ⬜ pendiente

---

## Cliente NUEVO — `client-ng/` (F4SE + CommonLibF4)  ← el que se usa

Reescritura sobre la plantilla CommonLibF4 (xmake). Es el cliente principal.

- [x] Plugin de F4SE con CommonLibF4 (carga por F4SE, sin "incompatible")
- [x] Menú in-game con **F4SE Menu Framework** (sección F4MP: conectar/desconectar/estado)
- [x] **Leer la posición del jugador local** (`PlayerCharacter::GetSingleton()->GetPosition()`)
- [x] Red (`NetworkClient`, GameNetworkingSockets): conectar, handshake, enviar posición
- [x] **Enviar tu posición al servidor** (10/s desde un hilo de red)
- [x] **Recibir las posiciones de los demás** jugadores
- [x] **Marcadores en el mundo** de los jugadores remotos (mundo→pantalla, nombre + distancia + altura)
- [ ] **Spawnear cuerpos/NPC reales** para los jugadores remotos ← siguiente gran paso (ver abajo)
- [ ] Interpolación de jugadores remotos (movimiento suave)
- [ ] Sincronización de rotación / animaciones
- [ ] Sincronización de combate / daño

## Servidor — `server/` (F4MPServer.exe)

- [x] Carga de `config.json`, logger a consola + archivo, `server-name`
- [x] `GameServer` (GameNetworkingSockets): listen socket, poll groups, `RunCallbacks`
- [x] Aceptar conexiones vía callback, mapa de clientes, `player-limit` por config
- [x] Relay de chat y de posición entre clientes
- [x] Bucle a 30 ticks
- [x] **Consola de administración** estilo Rust: info de arranque, estado periódico, comandos (`help`, `status`, `say`, `kick`, `stop`)
- [ ] Handlers de rotación, animaciones y daño
- [ ] Lista de baneos funcional

## Herramientas

- [x] **Simulador** (`tools/simulator/`, `F4MPSim.exe`): conecta N bots de prueba; posición en círculo o fija (`F4MPSim host puerto count [x y z]`)

## Cliente ANTIGUO — `client/` (inyección / ASI)  ← en desuso

Primer cliente (pattern scanning + hook D3D propio). Funcionaba (carga, menú, red),
pero el pattern scanning no localizaba bien al jugador. **Sustituido por `client-ng`.**

## Infraestructura

- [x] Dependencias vendidas en el repo (`third_party/win-x64`) para el cliente/servidor antiguos, sin vcpkg
- [x] Workflows de GitHub Actions: `build-client`, `build-server`, `build-client-ng`, `make-deps`
- [x] Web Pip-Boy (`web/`, no se sube a git por decisión)
- [ ] Header de protocolo compartido (hoy duplicado entre cliente/servidor)
- [ ] Tests automáticos

---

## Lo que queda: spawnear cuerpos de jugadores remotos

Los **marcadores** ya muestran dónde está cada jugador. Falta que aparezca un
**cuerpo (NPC)** de verdad. Es la pieza más difícil:

- CommonLibF4 **no** envuelve la función de spawnear (`PlaceObjectAtMe`).
- Hay dos vías, ambas requieren **pruebas en el juego** (no se puede a ciegas):
  1. **Nativa:** localizar el ID de Address Library de `PlaceObjectAtMe` + su firma,
     llamarla en el hilo principal con una base de actor válida, y mover el actor
     con `Actor::SetPosition`. Alto riesgo de crash; iterativo.
  2. **Papyrus + `.esp`** (lo estándar en FO4): un script que spawnea y mueve un
     actor, alimentado por el plugin C++. Necesita Creation Kit / xEdit.

## Prioridad sugerida

1. **Cuerpos remotos** (una de las dos vías de arriba) — para veros de verdad.
2. **Interpolación** para que el movimiento sea suave.
3. **Rotación y animaciones**.
4. **Combate / daño**.
