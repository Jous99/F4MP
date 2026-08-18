<!-- Language: English (top) · Español (below) -->

# F4MP — Roadmap

*[English](#english) · [Español](#español)*

<a name="english"></a>

 Where the project is and where it's going. Legend: ✅ done · 🟡 in progress · ⬜ pending.
Day-to-day status lives in [`CHECKLIST.md`](CHECKLIST.md); this file is the higher-level plan.

## ✅ Done — the foundation

- Client loads as an F4SE plugin (CommonLibF4), in-game menu.
- Position sync: send your position, receive everyone else's.
- A **body per remote player** that moves, **interpolates smoothly** and **faces where the player looks**.
- The body is a **copy of your created character** (player base `0x7`).
- Cleanup: bodies are removed on disconnect and after a 5 s timeout.
- Server: poll groups, admin console, master-server heartbeat, **configurable tick rate** (default 60).
- Test simulator, web + master server (bilingual).

## 🟡 Phase 1 — Animation sync  ← we are here

Make the body walk/run/sneak/jump instead of gliding. The body is teleported each tick, so it won't animate on its own — we drive its animation graph from the sender's movement state.

- ⬜ Extend the protocol with movement state: `isMoving`, `isRunning` (already present), `isSneaking` (already present), `isJumping`/`inAir`, and ideally move direction (forward/back/strafe).
- ⬜ Sender: read the local player's movement flags each tick and send them.
- ⬜ Receiver: drive the body's animation graph with `NotifyAnimationGraph` events (`moveStart`/`moveStop`, `SprintStart`/`SprintStop`, `SneakStart`/`SneakStop`, jump events) based on the received state.
- ⬜ Blend interpolation with animation so feet don't slide (match anim speed to movement speed).
- ⬜ Test: two clients — one walks/runs/sneaks, the other sees the matching animation.

## ⬜ Phase 2 — Per-player appearance

Today every body uses *your* local character (base `0x7`), so on one machine everyone looks like you. For real multiplayer each player must send their own look.

- ⬜ On connect, each client serializes its appearance (head parts, face morphs, tint layers, hair/skin color, body).
- ⬜ Send it once (and on change) to the server; server stores + forwards to others.
- ⬜ Receiver builds/customizes the spawned NPC with that appearance instead of the local player base.
- ⬜ Handle male/female skeleton and race.

## ⬜ Phase 3 — Equipment & weapons

- ⬜ Sync equipped weapon and outfit so bodies show the right gear.
- ⬜ Sync draw/holster and weapon type (affects pose/animation).

## ⬜ Phase 4 — Combat & damage

- ⬜ Sync attacks / weapon fire / reload.
- ⬜ Damage model: who hit whom, health sync; server-side handlers.
- ⬜ Death / revive states.

## ⬜ Phase 5 — Polish & robustness

- ⬜ Cell / worldspace awareness (only show players in the same area; hide across load doors).
- ⬜ Name tags above bodies (name + distance), reusing the marker code.
- ⬜ In-game chat UI.
- ⬜ Smarter interpolation (fast-travel snap, extrapolation on packet loss, distance culling).
- ⬜ Reconnection handling and error messages.
- ⬜ Basic anti-cheat / sanity checks (server already owns the authoritative id).

## ⬜ Phase 6 — Server & infrastructure

- ⬜ Working ban/kick list persisted across restarts.
- ⬜ Shared protocol header (today the structs are duplicated between client, server and simulator).
- ⬜ Automated tests.
- ⬜ Master server: regions, version gating, richer server list.

## Suggested order

1. **Animations** (Phase 1) — biggest visual payoff next.
2. **Per-player appearance** (Phase 2) — needed for real multiplayer.
3. **Equipment** (Phase 3) — makes players recognizable.
4. **Combat** (Phase 4) — turns it into a game.
5. Polish + infra (Phases 5–6) in parallel as needed.

---

<a name="español"></a>

# F4MP — Roadmap (Español)

*[English](#english) · [Español](#español)*

Dónde está el proyecto y hacia dónde va. Leyenda: ✅ hecho · 🟡 en progreso · ⬜ pendiente.
El estado del día a día está en [`CHECKLIST.md`](CHECKLIST.md); este archivo es el plan a más alto nivel.

## ✅ Hecho — la base

- El cliente carga como plugin de F4SE (CommonLibF4), con menú in-game.
- Sync de posición: envías la tuya, recibes la de los demás.
- Un **cuerpo por jugador remoto** que se mueve, **se interpola suave** y **mira hacia donde miras**.
- El cuerpo es una **copia de tu personaje creado** (base del jugador `0x7`).
- Limpieza: los cuerpos se eliminan al desconectar y tras un timeout de 5 s.
- Servidor: poll groups, consola de administración, heartbeat al master server, **tick rate configurable** (60 por defecto).
- Simulador de prueba, web + master server (bilingüe).

## 🟡 Fase 1 — Sincronizar animaciones  ← estamos aquí

Que el cuerpo ande/corra/se agache/salte en vez de deslizarse. El cuerpo se teletransporta cada tick, así que no anima solo — hay que mover su grafo de animación según el estado de movimiento del jugador que lo controla.

- ⬜ Ampliar el protocolo con el estado de movimiento: `isMoving`, `isRunning` (ya está), `isSneaking` (ya está), `isJumping`/`enAire`, e idealmente la dirección (adelante/atrás/lateral).
- ⬜ Emisor: leer los flags de movimiento del jugador local cada tick y enviarlos.
- ⬜ Receptor: mover el grafo de animación del cuerpo con `NotifyAnimationGraph` (`moveStart`/`moveStop`, `SprintStart`/`SprintStop`, `SneakStart`/`SneakStop`, saltos) según el estado recibido.
- ⬜ Combinar interpolación y animación para que los pies no patinen (ajustar la velocidad de anim a la del movimiento).
- ⬜ Probar: dos clientes — uno anda/corre/se agacha, el otro ve la animación correcta.

## ⬜ Fase 2 — Apariencia por jugador

Ahora todos los cuerpos usan *tu* personaje local (base `0x7`), así que en una máquina todos tienen tu cara. Para multijugador de verdad, cada jugador debe enviar su propia apariencia.

- ⬜ Al conectar, cada cliente serializa su apariencia (head parts, morphs de cara, capas de tinte, color de pelo/piel, cuerpo).
- ⬜ Enviarla una vez (y al cambiar) al servidor; el servidor la guarda y la reparte.
- ⬜ El receptor construye/personaliza el NPC spawneado con esa apariencia en vez de la base del jugador local.
- ⬜ Gestionar esqueleto masculino/femenino y raza.

## ⬜ Fase 3 — Equipo y armas

- ⬜ Sincronizar arma equipada y ropa para que los cuerpos muestren el equipo correcto.
- ⬜ Sincronizar sacar/guardar arma y tipo de arma (afecta a pose/animación).

## ⬜ Fase 4 — Combate y daño

- ⬜ Sincronizar ataques / disparos / recarga.
- ⬜ Modelo de daño: quién golpea a quién, sync de vida; handlers en el servidor.
- ⬜ Estados de muerte / reaparición.

## ⬜ Fase 5 — Pulido y robustez

- ⬜ Conciencia de celda / worldspace (mostrar solo a jugadores de la misma zona; ocultar tras puertas de carga).
- ⬜ Etiquetas de nombre sobre los cuerpos (nombre + distancia), reusando el código de marcadores.
- ⬜ Interfaz de chat in-game.
- ⬜ Interpolación más lista (snap en fast travel, extrapolación ante pérdida de paquetes, culling por distancia).
- ⬜ Gestión de reconexión y mensajes de error.
- ⬜ Anti-cheat básico / comprobaciones de sanidad (el servidor ya controla el id autoritativo).

## ⬜ Fase 6 — Servidor e infraestructura

- ⬜ Lista de baneos/kicks persistente entre reinicios.
- ⬜ Header de protocolo compartido (hoy los structs están duplicados entre cliente, servidor y simulador).
- ⬜ Tests automáticos.
- ⬜ Master server: regiones, control de versión, lista de servidores más rica.

## Orden sugerido

1. **Animaciones** (Fase 1) — el mayor salto visual ahora.
2. **Apariencia por jugador** (Fase 2) — necesaria para multijugador real.
3. **Equipo** (Fase 3) — hace reconocibles a los jugadores.
4. **Combate** (Fase 4) — lo convierte en un juego.
5. Pulido + infra (Fases 5–6) en paralelo según haga falta.
