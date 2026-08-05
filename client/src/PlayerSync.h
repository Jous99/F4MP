#ifndef F4MPCLIENT_PLAYERSYNC_H
#define F4MPCLIENT_PLAYERSYNC_H

// Sincronizacion del jugador local (Mitad 1):
// localiza al jugador en memoria, lee su posicion y la manda al servidor.
// Se llama cada frame desde el hook de render.

namespace PlayerSync {
    void Update();
}

#endif
