<!-- Language: English (top) · Español (below) -->

# F4MP — Roadmap

*[English](#english) · [Español](#español)*

<a name="english"></a>

Where the project is and where it's going. Legend: ✅ done · 🟡 in progress · ⬜ pending.
Day-to-day status lives in [`CHECKLIST.md`](../CHECKLIST.md); this file is the higher-level plan.

## Vision — two modes on one foundation

- **Co-op** (small group, shared story): one host, sync most nearby NPCs + quests.
- **MMO / free-roam** (NV:MP style — dedicated server, free roam, role-play): a **dedicated host** (a machine
  running Fallout 4 + the mod as the permanent world), sync **players + key entities**, ambient NPCs stay local
  per client (like GTA Online's traffic), area-of-interest by distance.

Both share the same tech: **host-authoritative** (a client's game is the source of truth — the network server is
only a relay with no game engine), NPC sync by FormID, and area-of-interest. We build the co-op foundation first.

## ✅ Done — the foundation

- Client loads as an F4SE plugin (CommonLibF4), in-game menu.
- Position sync: send your position, receive everyone else's.
- A **body per remote player** that moves, **interpolates smoothly** and **faces where the player looks**.
- Cleanup: bodies removed on disconnect (server sends an explicit `Disconnect`) and after a 5 s timeout;
  removal by API (`Disable` + `SetWantsDelete` + `MarkAsDeleted`).
- Server: poll groups, admin console, master-server heartbeat, **configurable tick rate** (default 60).
- Test simulator, web + master server (bilingual).

## ✅ Phase 1 — Animation sync (playable)

The body walks/runs/sneaks/jumps instead of gliding. **Key trick:** `SetGraphVariableBool("bIsSynced", true)` each
tick makes the animation graph honor the `Speed` we set (otherwise the engine resets it to 0). Movement stays as
smooth `SetPosition`; animation is driven from the sender's movement state.

- ✅ Protocol carries movement state (speed, moveDir, isMoving/running/sprinting/sneaking/jumping, weaponDrawn).
- ✅ Walk / run / sprint (Speed + IsSprinting, smoothed + hysteresis so events don't flicker).
- ✅ Sneak (SetSneaking + IsSneaking + SneakStart event).
- ✅ Weapon drawn — combat stance (`GetWeaponMagicDrawn` → `DrawWeaponMagicHands`). *(Weapon MODEL = Phase 3.)*
- ✅ Jump (`IsJumping` → jump event; Z already follows the arc via SetPosition).
- 🟡 Polish: sneak-hold is asymmetric between the two test machines (likely a "hold vs toggle sneak" setting); revisit.

## 🟡 Phase 1.5 — NPC sync (host-authoritative)  ← we are here

The network server has **no game engine**, so NPCs must be authoritative on a **client (host)**. To avoid duplicates
(both machines load the same world), the client does **not** spawn copies: it finds its **own** NPC by **FormID**,
calms its AI, and drives it to the host's state. Reuses the whole player-body puppet system.

- ✅ Protocol + relay: `NpcState` message (FormID, position, heading, …); server just forwards it.
- ✅ Host toggle in the menu; host broadcasts nearby fixed-world NPCs (~10/s, FormID < 0xFF000000, capped).
- ✅ Client drives its local NPCs by FormID (calm AI + interpolated position + heading).
- ⬜ Animation for NPCs (reuse `bIsSynced` + Speed/events).
- ⬜ Health + death sync (host authoritative; damage from non-host sent to host).
- ⬜ Restore NPC AI when it leaves range / host stops. Dynamically-spawned NPCs. Area-of-interest per player.
- ⬜ Server-side: only accept NPC messages from the host (anti-spoof).

## ⬜ Phase 2 — Per-player appearance

Today every body uses *your* local character (base `0x7`). For real multiplayer each player must send their own look.
(Appearance capture already exists — `PlayerAppearanceMsg`; pending: build the body from it instead of base 0x7.)

## ⬜ Phase 3 — Equipment & weapons

- ⬜ Sync equipped weapon and outfit so bodies show the right gear.
- ⬜ Sync draw/holster and weapon type (affects pose/animation).

## ⬜ Phase 4 — Combat & damage

- ⬜ Sync attacks / weapon fire / reload.
- ⬜ Damage model: who hit whom, health sync; **host-authoritative** for NPCs.
- ⬜ Death / revive states.

## ⬜ Phase 5 — Polish & robustness

- ⬜ Cell / worldspace awareness (only show players/NPCs in the same area; hide across load doors).
- ⬜ Name tags above bodies (name + distance).
- ⬜ In-game chat UI.
- ⬜ Smarter interpolation (fast-travel snap, extrapolation on packet loss, distance culling).
- ⬜ Reconnection handling and error messages. Basic anti-cheat.

## ⬜ Phase 6 — Server, infra & distribution

- ✅ Cross-platform server (Windows + Linux; heartbeat) + Docker image.
- ✅ Manual release workflow (GitHub Actions, `workflow_dispatch`): builds client + server, publishes zips.
- ⬜ Quest sync (host broadcasts stage changes → others `SetStage`).
- ⬜ **Dedicated host** for the MMO mode (a permanent Fallout 4 instance as the world).
- ⬜ Autoconnect (plugin reads config on launch, skips the menu) + standalone launcher (update DLL, pick server, launch).
- ⬜ Working ban/kick list; shared protocol header; automated tests; richer master server (regions, version gating).

## Suggested order

1. **NPC sync** (Phase 1.5) — animation, then health/death.
2. **Per-player appearance** (Phase 2) — needed for real multiplayer.
3. **Equipment** (Phase 3) → **Combat** (Phase 4) — turns it into a game.
4. **Quest sync** + **dedicated host** — enables real co-op / MMO sessions.
5. Polish + infra (Phases 5–6) in parallel as needed.

---

<a name="español"></a>

# F4MP — Roadmap (Español)

*[English](#english) · [Español](#español)*

Dónde está el proyecto y hacia dónde va. Leyenda: ✅ hecho · 🟡 en progreso · ⬜ pendiente.
El estado del día a día está en [`CHECKLIST.md`](../CHECKLIST.md); este archivo es el plan a más alto nivel.

## Visión — dos modos sobre una misma base

- **Co-op** (grupo pequeño, historia compartida): un host, sincronizas casi todos los NPCs cercanos + misiones.
- **MMO / mundo libre** (estilo NV:MP — servidor dedicado, free-roam, rol): un **host dedicado** (una máquina con
  Fallout 4 + el mod como mundo permanente), sincronizas **jugadores + entidades clave**, y los NPCs de relleno son
  locales de cada cliente (como el tráfico de GTA Online), con área de interés por distancia.

Los dos comparten la misma tecnología: **host autoritativo** (el juego de un cliente es la verdad — el servidor de
red es solo un cartero sin motor de juego), sync de NPCs por FormID, y área de interés. Primero montamos la base de co-op.

## ✅ Hecho — la base

- El cliente carga como plugin de F4SE (CommonLibF4), con menú in-game.
- Sync de posición: envías la tuya, recibes la de los demás.
- Un **cuerpo por jugador remoto** que se mueve, **se interpola suave** y **mira hacia donde miras**.
- Limpieza: los cuerpos se eliminan al desconectar (el servidor manda un `Disconnect` explícito) y tras 5 s;
  borrado por API (`Disable` + `SetWantsDelete` + `MarkAsDeleted`).
- Servidor: poll groups, consola de administración, heartbeat al master server, **tick rate configurable** (60).
- Simulador de prueba, web + master server (bilingüe).

## ✅ Fase 1 — Sincronizar animaciones (jugable)

El cuerpo anda/corre/se agacha/salta en vez de deslizarse. **La clave:** `SetGraphVariableBool("bIsSynced", true)`
cada tick hace que el grafo respete el `Speed` que le fijamos (si no, el motor lo pone a 0). El movimiento sigue
siendo `SetPosition` suave; la animación se dirige según el estado de movimiento del emisor.

- ✅ El protocolo lleva el estado (speed, moveDir, isMoving/running/sprinting/sneaking/jumping, weaponDrawn).
- ✅ Andar / correr / sprint (Speed + IsSprinting, suavizado + histéresis para que los eventos no parpadeen).
- ✅ Agacharse (SetSneaking + IsSneaking + evento SneakStart).
- ✅ Arma en mano — postura de combate (`GetWeaponMagicDrawn` → `DrawWeaponMagicHands`). *(El MODELO del arma = Fase 3.)*
- ✅ Saltar (`IsJumping` → evento; la Z ya sigue el arco por SetPosition).
- 🟡 Pulir: mantener el agachado es asimétrico entre las dos máquinas de prueba (posible ajuste "mantener vs alternar sigilo").

## 🟡 Fase 1.5 — Sincronización de NPCs (host autoritativo)  ← estamos aquí

El servidor de red **no tiene motor**, así que los NPCs deben ser autoritativos en un **cliente (host)**. Para evitar
duplicados (las dos máquinas cargan el mismo mundo), el cliente **no** spawnea copias: busca **su propio** NPC por
**FormID**, le calma la IA y lo conduce al estado del host. Reutiliza todo el sistema de muñecos de jugador.

- ✅ Protocolo + reenvío: mensaje `NpcState` (FormID, posición, heading, …); el servidor solo lo reenvía.
- ✅ Toggle de host en el menú; el host difunde los NPCs fijos cercanos (~10/s, FormID < 0xFF000000, con tope).
- ✅ El cliente conduce sus NPCs locales por FormID (calmar IA + posición interpolada + heading).
- ⬜ Animación de los NPCs (reusar `bIsSynced` + Speed/eventos).
- ⬜ Sync de vida + muerte (host autoritativo; el daño del no-host se manda al host).
- ⬜ Devolver la IA al NPC al salir de rango / si el host para. NPCs dinámicos. Área de interés por jugador.
- ⬜ En el servidor: aceptar mensajes de NPC solo del host (anti-suplantación).

## ⬜ Fase 2 — Apariencia por jugador

Hoy todos los cuerpos usan *tu* personaje local (base `0x7`). Para multijugador de verdad, cada jugador debe enviar
su propia apariencia. (La captura ya existe — `PlayerAppearanceMsg`; falta construir el cuerpo con ella en vez de base 0x7.)

## ⬜ Fase 3 — Equipo y armas

- ⬜ Sincronizar arma equipada y ropa para que los cuerpos muestren el equipo correcto.
- ⬜ Sincronizar sacar/guardar arma y tipo de arma (afecta a pose/animación).

## ⬜ Fase 4 — Combate y daño

- ⬜ Sincronizar ataques / disparos / recarga.
- ⬜ Modelo de daño: quién golpea a quién, sync de vida; **host autoritativo** para NPCs.
- ⬜ Estados de muerte / reaparición.

## ⬜ Fase 5 — Pulido y robustez

- ⬜ Conciencia de celda / worldspace (mostrar solo a jugadores/NPCs de la misma zona; ocultar tras puertas de carga).
- ⬜ Etiquetas de nombre sobre los cuerpos (nombre + distancia).
- ⬜ Chat in-game. Interpolación más lista (snap en fast travel, extrapolación, culling). Reconexión. Anti-cheat básico.

## ⬜ Fase 6 — Servidor, infra y distribución

- ✅ Servidor multiplataforma (Windows + Linux; heartbeat) + imagen Docker.
- ✅ Workflow de release manual (GitHub Actions, `workflow_dispatch`): compila cliente + servidor, publica zips.
- ⬜ Sync de misiones (el host difunde los cambios de etapa → los demás `SetStage`).
- ⬜ **Host dedicado** para el modo MMO (una instancia permanente de Fallout 4 como mundo).
- ⬜ Autoconnect (el plugin lee config al arrancar y salta el menú) + launcher (actualiza DLL, elige servidor, lanza).
- ⬜ Lista de baneos/kicks; header de protocolo compartido; tests; master server más rico (regiones, versión).

## Orden sugerido

1. **Sync de NPCs** (Fase 1.5) — animación, luego vida/muerte.
2. **Apariencia por jugador** (Fase 2) — necesaria para multijugador real.
3. **Equipo** (Fase 3) → **Combate** (Fase 4) — lo convierte en un juego.
4. **Sync de misiones** + **host dedicado** — habilita co-op / sesiones MMO de verdad.
5. Pulido + infra (Fases 5–6) en paralelo según haga falta.
