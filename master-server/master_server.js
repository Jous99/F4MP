// F4MP Master Server
// -------------------
// Servicio minimo (sin dependencias, solo Node.js) que mantiene la lista de
// servidores de F4MP vivos y la sirve a la web Pip-Boy.
//
//   POST /heartbeat   -> un servidor de F4MP se registra / renueva
//   GET  /servers     -> la web pide la lista de servidores vivos (JSON)
//   GET  /            -> estado del propio master server
//
// Ejecutar:   node master_server.js
// Puerto:     variable de entorno PORT (por defecto 8080)

const http = require('http');

const PORT   = process.env.PORT || 8080;
const TTL_MS = 30000; // un servidor caduca si no manda heartbeat en 30 s

/** @type {Map<string, object>} clave "host:port" -> info del servidor */
const servers = new Map();

function setCors(res) {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
}

function sendJson(res, code, obj) {
    setCors(res);
    res.writeHead(code, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(obj));
}

/** Elimina los servidores que llevan demasiado tiempo sin dar señales. */
function prune() {
    const now = Date.now();
    for (const [key, s] of servers) {
        if (now - s.lastSeen > TTL_MS) servers.delete(key);
    }
}

const server = http.createServer((req, res) => {
    // Preflight de CORS
    if (req.method === 'OPTIONS') {
        setCors(res);
        res.writeHead(204);
        res.end();
        return;
    }

    const url = new URL(req.url, `http://${req.headers.host}`);

    // --- Lista de servidores (lo que consume la web) ---
    if (req.method === 'GET' && url.pathname === '/servers') {
        prune();
        const list = [...servers.values()].map(s => ({
            name:       s.name,
            region:     s.region,
            host:       s.host,
            port:       s.port,
            players:    s.players,
            maxPlayers: s.maxPlayers,
            online:     true,
        }));
        return sendJson(res, 200, list);
    }

    // --- Registro / renovacion de un servidor ---
    if (req.method === 'POST' && url.pathname === '/heartbeat') {
        let body = '';
        req.on('data', chunk => {
            body += chunk;
            if (body.length > 100000) req.destroy(); // proteccion basica
        });
        req.on('end', () => {
            let d;
            try { d = JSON.parse(body); }
            catch { return sendJson(res, 400, { error: 'JSON invalido' }); }

            if (!d.name || !d.port) {
                return sendJson(res, 400, { error: 'faltan campos: name y port' });
            }

            const host = d.host || req.socket.remoteAddress;
            const key  = `${host}:${d.port}`;
            servers.set(key, {
                name:       String(d.name).slice(0, 64),
                region:     String(d.region || '').slice(0, 32),
                host:       host,
                port:       Number(d.port),
                players:    Number(d.players)    || 0,
                maxPlayers: Number(d.maxPlayers) || 0,
                lastSeen:   Date.now(),
            });
            return sendJson(res, 200, { ok: true });
        });
        return;
    }

    // --- Estado del master server ---
    if (req.method === 'GET' && url.pathname === '/') {
        prune();
        return sendJson(res, 200, { service: 'F4MP master server', servers: servers.size });
    }

    sendJson(res, 404, { error: 'no encontrado' });
});

server.listen(PORT, () => {
    console.log(`[F4MP] Master server escuchando en el puerto ${PORT}`);
    console.log(`[F4MP] Servidores caducan tras ${TTL_MS / 1000}s sin heartbeat`);
});
