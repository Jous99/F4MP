# CLAUDE.md — F4MP (Fallout 4 Multiplayer)

> Contexto del proyecto para Claude Code. Léelo entero antes de tocar nada.
> Idioma de trabajo: **español** (comentarios de código y docs en español).
> El autor (Jose / Jous) está **aprendiendo a programar**: explica el porqué de los cambios, no solo el qué.

## Qué es F4MP
Mod multijugador para **Fallout 4** (Next-Gen 1.10.980+ / Anniversary), reescrito desde el F4MP
original abandonado en 2021. Objetivo doble: **co-op** y, a futuro, **mundo libre tipo NV:MP / GTA Online**.

## Estructura del repo (usa rutas RELATIVAS)
Trabajas dentro de este repo. El autor tiene **varios clones** en su PC y los sincroniza por **git**
(push desde uno, `git pull` en los demás), así que no hardcodees rutas absolutas.
- `client-ng/` → **cliente ACTIVO** (plugin F4SE + CommonLibF4, build con **xmake**). CommonLibF4 es submódulo en `client-ng/lib/commonlibf4`.
- `client/` → cliente VIEJO (pattern scanning), **en desuso, ignóralo**.
- `server/` → servidor dedicado (build con **cmake + Ninja**). Deps vendidas en `third_party/win-x64`.
- `tools/simulator/` → bots de prueba. `docs/ROADMAP.md`, `CHECKLIST.md` → plan y estado.
- `.github/workflows/` → CI (build-client-ng, build-server, make-deps, release). Archivos protegidos.
- **La WEB es un repo APARTE** (bilingüe ES/EN, se despliega en el homelab del autor); **no está en este repo**, no la toques desde aquí.

## Flujo de trabajo del autor (respétalo)
1. Edita el código en el PC → **git commit + push**.
2. En **cada máquina** (PC y portátil): `git pull` + recompila.
3. **Reinicia Fallout** tras compilar — F4SE carga el DLL al arrancar el juego; si recompilas con el juego abierto, sigue el DLL viejo.
4. Las **dos máquinas deben correr el mismo build** o los resultados son asimétricos y engañosos.

## Cómo compilar
- Cliente: `build_client.bat` (xmake, modo releasedbg). **OJO: su "[OK]/Desplegado: SI" ENGAÑA** — si hubo un `error C...` no recompiló. Mira que no haya errores.
- Servidor: `build_server.bat` (cmake). Linux/Docker: hay `Dockerfile` (expone `7779/udp`).
- **Al cambiar un struct de red, recompila cliente Y servidor** (los structs están duplicados, ver abajo).

## Arquitectura (clave para todo)
- **El servidor es solo un RELAY de red** (GameNetworkingSockets/UDP, puerto 7779). **No tiene motor de juego**: no simula nada, no lee Fallout, solo reenvía mensajes (structs con cabecera).
- **El cliente es el plugin** (`F4MPClient.dll`) dentro del Fallout de cada jugador. Es el que **habla con el juego** (lee/escribe vía CommonLibF4) y con la red.
- Cadena: `Fallout A ↔ plugin A ↔ [UDP] ↔ servidor ↔ [UDP] ↔ plugin B ↔ Fallout B`.
- Los NPCs/misiones son **HOST-AUTORITATIVOS**: como el servidor no simula, un **cliente (host)** es la verdad y difunde el estado; los demás lo reproducen. Puede haber un **host dedicado invisible** (toggle `g_hideSelf`: no envía su jugador → no aparece; solo difunde NPCs).
- **Datos de juego** (mundo, NPCs, misiones) viven en el Fallout de cada jugador → **todos deben tener la misma versión + los mismos mods** (para que los FormID coincidan).

## Protocolo de red
- `enum MessageType` + structs (`PlayerPositionMsg`, `NpcStateMsg`, `PlayerAppearanceMsg`, `ChatMessageMsg`, `PlayerLeftMsg`…) están **DUPLICADOS a mano** en `client-ng/src/network/NetworkClient.h` y `server/src/Network/GameServer.h`. **Mantenlos idénticos** (mismos campos, mismo orden, `#pragma pack(1)`).
- El cliente valida tamaño: si `payloadSize < sizeof(struct)` descarta el paquete (versión desincronizada).
- Pendiente: header de protocolo compartido en vez de duplicado.

## Estado actual
**Animación de jugadores remotos — JUGABLE.** En `client-ng/src/main.cpp`, `BodiesSync()`:
- Cuerpos = NPCs `player.placeatme 7 1` (base 0x7). IA calmada con `InitiateDoNothingPackage()` + `StopCombat()`. Borrado por API (`Disable` + `SetWantsDelete` + `MarkAsDeleted`).
- Movimiento: `SetPosition` interpolado (nueva = actual + delta*0.35/tick). Fluido. Teletransporte si dist>1500.
- **LA CLAVE de la animación:** `actor->SetGraphVariableBool("bIsSynced", true)` CADA tick → el grafo respeta el `Speed` que le fijamos (si no, el motor lo pisa a 0 y se congela).
- Cubierto: andar/correr/sprint (Speed + IsSprinting, suavizado + histéresis), agacharse (SetSneaking + IsSneaking + evento SneakStart; re-afirmar cada frame), arma en mano (`GetWeaponMagicDrawn` → `DrawWeaponMagicHands`), saltar (`IsJumping` → evento). Envío de posición a 60/s.
- Pendiente de pulir: el agacharse se mantiene desde una máquina pero no desde la otra (posible ajuste "mantener vs alternar sigilo"), NO bloqueante.

**Sync de NPCs (host autoritativo) — EN CURSO, pasos 1–3 hechos:**
- `NpcStateMsg` (FormID, pos, heading, …). El servidor lo reenvía (`HandleNpcState`).
- Toggle de host en el menú; el host difunde NPCs cercanos por FormID (~10/s, FormID < 0xFF000000, tope 30).
- El cliente conduce sus NPCs locales por FormID (`GetFormByID` → calmar IA → interpolar pos + heading). Sin spawnear copias (evita duplicados: las dos máquinas cargan el mismo mundo).
- Toggle `g_hideSelf` ("No aparecer / host dedicado").
- **Falta:** animación de NPCs (reusar bIsSynced), vida + muerte, devolver IA al salir de rango, área de interés por jugador, y que el servidor acepte NPC solo del host.

## Gotchas de la API CommonLibF4 (no los redescubras)
- `NotifyAnimationGraph` **NO existe** público → usa **`NotifyAnimationGraphImpl(BSFixedString)`**.
- `std::min/std::max` rompe por la macro de `<windows.h>` → usa **`std::min<float>(...)`** / `std::max<float>`.
- **NO** llames a `UpdateActor3DPosition()` cada frame: re-posa al actor y **reinicia la animación**. Para orientar usa `SetHeading`.
- Controlador físico del actor: `actor->currentProcess->middleHigh->charController` (NiPointer). `hkVector4f` en unidades havok = unidades de juego * `0.0142875`.
- Heading del actor: `actor->GetHeading()`.

## Lo que NO funciona (ya probado, no repetir)
- Fijar `Speed` SIN `bIsSynced` → el motor lo pone a 0 (animación congelada).
- `Actor::Move(dt, delta)` → mueve en espacio LOCAL (dirección de vista) → "siempre hacia delante", se desincroniza.
- Mover por `SetLinearVelocityImpl` del charController → no mueve bien el cuerpo (se ve a teletransportes/trompicones).

## Próximos pasos (ver `docs/ROADMAP.md`)
1. **NPCs**: animación (bIsSynced), luego vida/muerte.
2. Apariencia por jugador (ya existe `PlayerAppearanceMsg`; falta construir el cuerpo con ella en vez de base 0x7).
3. Equipo/arma (modelo) → combate/daño.
4. Sync de misiones (host difunde `SetStage`). Host dedicado permanente (máquina Windows con Fallout).
5. Infra: autoconnect (config al arrancar) + launcher. Servidor público: Docker en ZimaOS (relay) + forward UDP 7779.

## Convenciones
- Comentarios y docs en **español**. Mantén `docs/ROADMAP.md` y `CHECKLIST.md` al día cuando cierres un hito.
- No edites la web desde este repo (la web vive en su propio repo aparte).
- Sé incremental y explica el porqué (el autor aprende). Un cambio a la vez cuando depures en el juego.
