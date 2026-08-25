# Downgrade de Fallout 4 vía consola de Steam (para F4MP)

Guía para bajar Fallout 4 a una versión concreta usando la consola de Steam
(`download_depot`), de forma que sea compatible con **F4SE** y, por tanto,
con **F4MP**.

## Por qué hace falta

F4MP se carga como plugin de **F4SE** (Fallout 4 Script Extender). F4SE
solo funciona con la versión exacta de `Fallout4.exe` para la que fue
compilado — si Steam actualiza el juego a una build más nueva que F4SE
todavía no soporta, el mod (y todos los mods F4SE) dejan de cargar.

Según el README de F4MP, las versiones soportadas son:
- **Next-Gen**: 1.10.980 en adelante
- **Anniversary Edition (AE)**: 1.11.x

Steam por defecto instala siempre la build más reciente. Si esa build es
más nueva que la última versión soportada por F4SE, hay que "clavar"
(pin) el juego a una versión anterior compatible mediante depots.

> ⚠️ Esto **no** sirve para jugar mods viejos pre-Next-Gen (esos necesitan
> downgradear a 1.10.163 o anterior). F4MP necesita explícitamente
> Next-Gen o AE, así que el objetivo aquí es fijar una build **dentro**
> de ese rango, no volver a la versión "legacy" clásica.

## Antes de empezar

1. **Comprueba qué versión soporta F4SE ahora mismo**:
   👉 <https://f4se.silverlock.org>
   La página indica la última versión de `Fallout4.exe` compatible.
2. **Busca el depot y el manifest ID de esa versión en SteamDB**:
   👉 <https://steamdb.info/app/377160/depots/>
   Los manifest IDs cambian con cada build, así que no uses IDs viejos
   de guías antiguas — verifica siempre en SteamDB cuál corresponde a
   la versión que necesitas.
3. **Haz backup** de tu partida guardada y, si puedes, de la carpeta
   completa de Fallout 4 antes de tocar nada.
4. Cierra Steam y el juego si están abiertos.

## Pasos

### 1. Anota los depots que necesitas

Fallout 4 (AppID `377160`) reparte sus archivos en varios depots
(juego base + DLCs). En SteamDB verás algo como:

| Depot | Contenido |
|---|---|
| 377161 | Fallout 4 - Content |
| 377162 | Fallout 4 - Content 2 |
| 377163 | Fallout 4 - Content 3 |
| 377164 | Fallout 4 - Content 4 |
| 435870 / 435871 | Automatron DLC |
| 435880 / 435882 | Wasteland Workshop DLC |
| 393885 / 393895 | otros DLC / High Res |
| 480630 / 480631 | otros contenidos |

Para cada depot que quieras descargar, en SteamDB entra al historial de
manifests de ese depot y copia el **manifest ID** correspondiente a la
build objetivo (por fecha, comparando con la versión que quieres).

### 2. Abre la consola de Steam

- Cierra sesión del juego en Steam (no hace falta cerrar Steam entero).
- Windows: `Win + R` → escribe `steam://open/console` → Enter.
  (O: Steam → menú `Steam` en la esquina superior... en algunas
  versiones hay que activar la consola con `-console` como parámetro
  de lanzamiento de Steam.)

### 3. Descarga cada depot

Para cada fila de tu tabla, en la consola de Steam escribe:

```
download_depot 377160 <depot_id> <manifest_id>
```

Ejemplo de formato (sustituye por los IDs reales que sacaste de SteamDB):

```
download_depot 377160 377161 <manifest_id_correcto>
download_depot 377160 377162 <manifest_id_correcto>
download_depot 377160 377163 <manifest_id_correcto>
download_depot 377160 377164 <manifest_id_correcto>
```

Repite para cada DLC/depot que uses. Steam descargará cada uno como una
carpeta separada en:

```
Steam\steamapps\content\app_377160\depot_<depot_id>\
```

Esto puede tardar bastante (varios GB por depot).

### 4. Junta los archivos

Copia el **contenido** de cada carpeta `depot_XXXXXX` dentro de
`app_377160` y pégalo todo mezclado en una única carpeta destino.
Como los depots no se pisan entre sí (cada uno trae archivos
distintos), el resultado es una copia completa del juego en esa
versión.

### 5. Reemplaza (o aísla) tu instalación

Tienes dos opciones:

**Opción A — Sobrescribir la instalación actual**
1. Ve a `Steam\steamapps\common\Fallout 4`.
2. Copia dentro los archivos que juntaste en el paso 4 (sobrescribiendo).
3. Ve a `Steam\steamapps\` y marca `appmanifest_377160.acf` como
   **solo lectura** (clic derecho → Propiedades → Solo lectura). Esto
   evita que Steam vuelva a actualizar el juego automáticamente.

**Opción B — Instalación separada (recomendado si usas un gestor de mods)**
1. Crea una carpeta "Stock Game" fuera de `steamapps` (por ejemplo, en
   tu instancia de MO2/Vortex).
2. Copia ahí los archivos del paso 4.
3. Apunta tu gestor de mods / F4SE a esa carpeta en vez de a la de
   Steam. Así Steam puede actualizarse libremente sin afectar a tu
   instalación fijada.

### 6. Verifica la versión

1. Lanza el juego (ver paso 7) y entra al menú principal.
2. En la esquina, o en el menú de opciones/créditos, confirma el
   número de versión — debe coincidir con la que descargaste.

### 7. Lanza siempre vía F4SE

No uses el botón "Jugar" de Steam normal. Lanza con:

```
f4se_loader.exe
```

o mediante la opción "Launch F4SE" de tu gestor de mods. Si le das al
Play normal de Steam, F4SE (y por tanto F4MP) no se carga.

## Después de downgradear: instala F4MP

Con la versión ya fijada y compatible:

1. **F4SE** — para tu versión exacta: <https://f4se.silverlock.org>
2. **Address Library for F4SE**: <https://www.nexusmods.com/fallout4/mods/47327>
   (el `.bin` correspondiente a tu versión, en `Data\F4SE\Plugins\`)
3. **F4SE Menu Framework**: <https://www.nexusmods.com/fallout4/mods/105090>
4. **F4MP**: `F4MPClient.dll` → `Data\F4SE\Plugins\`; DLLs de runtime
   junto a `Fallout4.exe`. Descárgalo de las Actions/Releases del repo:
   <https://git.joustech.space/F4MP/F4MP>

## Alternativa: downgrader automático

Si prefieres no hacerlo a mano, existe un downgrader de la comunidad
que automatiza los pasos 1-5 (login con Steam, descarga de depots,
copia de archivos): **Fallout 4 Downgrader** en Nexus Mods
(busca "Fallout 4 Downgrader" en nexusmods.com/fallout4). Verifica la
versión objetivo que ofrece antes de usarlo, porque debe coincidir con
lo que F4SE soporta en el momento en que lo uses.

## Precauciones finales

- No uses manifest IDs de guías antiguas sin comprobarlos en SteamDB —
  cambian con cada actualización y un ID incorrecto te deja con una
  build corrupta o mixta.
- Guarda tus partidas antes de tocar la instalación.
- Si marcaste el `.acf` como solo lectura y luego quieres volver a
  actualizar el juego normalmente, quita el atributo de solo lectura.
- Si tras el downgrade el juego crashea en el vídeo de inicio, revisa
  que: (a) lanzaste con `f4se_loader.exe` y no con Steam directo, (b)
  el Address Library instalado es el de tu versión exacta, y (c) no
  quedaron archivos sueltos de una versión distinta mezclados en la
  carpeta del juego.
