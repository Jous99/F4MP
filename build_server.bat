@echo off
setlocal
title F4MP - Compilar Servidor

REM Situarse en la carpeta del repo (donde esta este .bat)
cd /d "%~dp0"

echo ============================================
echo   F4MP - Compilando SERVIDOR (F4MPServer.exe)
echo ============================================
echo.

set "VENDORED=%~dp0third_party\deps\x64-windows"

if exist "%VENDORED%\share" (
  REM ===== Dependencias vendidas en el repo: compilar SIN vcpkg =====
  echo Usando dependencias del repo (third_party). No hace falta vcpkg.
  echo.
  echo Configurando el proyecto...
  cmake -S server -B server\build -A x64
  if errorlevel 1 ( echo [ERROR] Fallo la configuracion de CMake. & pause & exit /b 1 )

) else (
  REM ===== No hay dependencias vendidas: usar vcpkg =====
  if not defined VCPKG_ROOT if exist "%~dp0.deps\vcpkg\scripts\buildsystems\vcpkg.cmake" set "VCPKG_ROOT=%~dp0.deps\vcpkg"
  if not defined VCPKG_ROOT if exist "C:\vcpkg\scripts\buildsystems\vcpkg.cmake" set "VCPKG_ROOT=C:\vcpkg"

  if not defined VCPKG_ROOT (
    echo [ERROR] No encuentro las dependencias.
    echo         Ejecuta primero  setup_deps.bat  (solo una vez).
    pause
    exit /b 1
  )
  echo Usando vcpkg en: %VCPKG_ROOT%
  echo.
  "%VCPKG_ROOT%\vcpkg.exe" install gamenetworkingsockets:x64-windows spdlog:x64-windows
  if errorlevel 1 ( echo [ERROR] Fallo al instalar dependencias. & pause & exit /b 1 )
  echo.
  echo Configurando el proyecto...
  cmake -S server -B server\build -A x64 -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
  if errorlevel 1 ( echo [ERROR] Fallo la configuracion de CMake. & pause & exit /b 1 )
)
echo.

REM --- Compilar en modo Release ---
echo Compilando...
cmake --build server\build --config RelWithDebInfo
if errorlevel 1 ( echo [ERROR] Fallo la compilacion. & pause & exit /b 1 )
echo.

REM --- Copiar DLLs de runtime junto al binario ---
if exist "%VENDORED%\bin" copy /Y "%VENDORED%\bin\*.dll" "server\build\RelWithDebInfo\" >nul 2>&1
if defined VCPKG_ROOT copy /Y "%VCPKG_ROOT%\installed\x64-windows\bin\*.dll" "server\build\RelWithDebInfo\" >nul 2>&1

echo ============================================
echo   [OK] Servidor compilado.
echo   Salida: server\build\RelWithDebInfo\F4MPServer.exe
echo ============================================
pause
endlocal
