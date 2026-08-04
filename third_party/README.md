# third_party

Dependencias de terceros **ya compiladas** para F4MP, guardadas en el repo para
poder compilar **sin vcpkg**.

## Estructura

```
third_party/
└── win-x64/            Binarios para Windows x64 (Release)
    ├── include/        Cabeceras (GameNetworkingSockets, spdlog, ...)
    ├── lib/            Librerias de enlace (.lib)
    ├── bin/            DLLs de runtime (.dll)
    └── share/          Configs de CMake (para find_package)
```

## Qué contiene

- **GameNetworkingSockets** — red del mod.
- **spdlog** — logging.
- Y sus dependencias: OpenSSL, protobuf, abseil, fmt.

## Cómo lo usan los proyectos

Los `CMakeLists.txt` del cliente, servidor y simulador detectan esta carpeta y
la añaden a `CMAKE_PREFIX_PATH`, así `find_package(GameNetworkingSockets)` y
`find_package(spdlog)` funcionan sin necesitar vcpkg.

## Cómo regenerarlas

Si algún día hay que actualizarlas (nueva versión de una librería):

1. Ejecuta `setup_deps.bat` (compila con vcpkg en `.deps/`).
2. Ejecuta `vendor_deps.bat` (copia el resultado aquí).

O usa el workflow **Make Deps** en GitHub Actions y descarga el artifact `f4mp-deps`.

> Nota: solo se guarda la versión **Release** (sin `debug/`, sin `.pdb`, sin `tools/`)
> para no inflar el repo más de lo necesario.
