# F4MP — Checklist de desarrollo

Estado del proyecto basado en el análisis del código. Marca las casillas a medida que avances.

Leyenda: ✅ hecho · 🟡 parcial / con bugs · ⬜ pendiente

---

## Cliente (F4MPClient.dll)

- [x] Inyección de DLL y arranque en un hilo (`DllMain` → `Main`)
- [x] Pattern scanning para resolver direcciones del juego sin offsets fijos
- [x] Hook de la consola del juego (`Console::Print`)
- [x] Hook de DirectX 11 (`IDXGISwapChain::Present`)
- [x] Menú ImGui (abrir con **Supr**), campos IP/puerto, botones Connect/Disconnect y overlay de estado
- [x] `NetworkClient`: wrapper de GameNetworkingSockets (Init/Connect/Disconnect/SendPacket/callbacks)
- [x] Llamar a `PumpMessages()` cada frame (en el hook de render)
- [x] Enviar `ConnectionRequest` con el nombre del jugador al conectar
- [x] Llamar a `RunCallbacks()` periódicamente
- [x] Detectar cambios de estado por callback (`SetGlobalCallback_SteamNetConnectionStatusChanged`)
- [x] Campo de **nombre de jugador** en el menú
- [x] Cargar como plugin de F4SE (`Data\F4SE\Plugins`) — sin inyector
- [ ] Leer la posición/rotación del jugador local desde la memoria del juego
- [ ] Crear y mover los actores de los jugadores remotos en el mundo
- [ ] Interpolación de jugadores remotos (movimiento suave)
- [ ] Sincronización de animaciones
- [ ] Sincronización de combate/daño

## Servidor (F4MPServer.exe)

- [x] Carga de `config.json` (lo crea si no existe) + logger
- [x] `GameServer`: socket de escucha, mapa de clientes, broadcast
- [x] Relay de chat y de posición entre clientes
- [x] Bucle principal a 30 ticks
- [x] Señales (Ctrl+C), stub de servicio Windows y daemon en Linux
- [x] Aceptar conexiones vía callback + poll groups + `RunCallbacks()`
- [x] Usar `player-limit` del `config.json` en vez del `MAX_PLAYERS` fijo
- [ ] Handlers de rotación, animaciones y daño
- [ ] Lista de baneos funcional
- [ ] Sincronización de NPCs / entidades del mundo

## Protocolo / red

- [x] Tipos de mensaje y structs definidos (posición, chat, daño, etc.)
- [ ] Header de protocolo **compartido** entre cliente y servidor *(hoy está duplicado)*
- [ ] Versionado de protocolo comprobado en el handshake

## Infraestructura

- [x] Workflow de compilación (GitHub Actions) para cliente y servidor
- [x] Publicación de binarios en Releases al crear un tag `v*`
- [ ] Tests automáticos
- [ ] Limpiar código muerto (`Server.cpp` / `Server.hpp` sin usar)
- [ ] Actualizar/eliminar `.gitmodules` obsoleto
- [ ] Web / landing del proyecto

---

## Prioridad sugerida

1. **Arreglar la red (bloqueantes):** `RunCallbacks`, callback de estado, `PumpMessages`, envío de `ConnectionRequest`. Con esto tendrás una conexión que funciona de verdad.
2. **Integración con el juego:** leer posición local y dibujar a los demás jugadores. Es el corazón del mod.
3. **Pulido:** interpolación, animaciones, combate, límites por config.
4. **Deuda técnica:** header compartido, limpiar código muerto, tests.
