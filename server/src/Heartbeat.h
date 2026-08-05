#ifndef F4MP_HEARTBEAT_H
#define F4MP_HEARTBEAT_H

#include <string>
#include <cstdint>

// Envia un POST /heartbeat al master server (via WinHTTP). Bloqueante:
// llamar desde un hilo aparte. Si url esta vacio, no hace nada.
namespace Heartbeat {
    void Send(const std::string& url, const std::string& name,
              uint16_t port, uint32_t players, uint32_t maxPlayers);
}

#endif
