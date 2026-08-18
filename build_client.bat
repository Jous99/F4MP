@echo off
setlocal
title F4MP - Compilar Cliente (client-ng)
cd /d "%~dp0"

echo ============================================
echo   F4MP - Compilando CLIENTE  [client-ng / plugin F4SE]
echo ============================================
echo.

REM -- Localizar Visual Studio (xmake usa el compilador MSVC) --
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :no_vs

set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto :no_vc

echo Visual Studio: %VSPATH%
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul

REM -- Comprobar xmake --
where xmake >nul 2>&1
if errorlevel 1 goto :no_xmake

REM -- Ruta del juego: xmake despliega el plugin aqui automaticamente --
set "GAMEDIR=D:\SteamLibrary\steamapps\common\Fallout 4"
set "XSE_FO4_GAME_PATH=%GAMEDIR%"

cd client-ng

echo.
echo Configurando (modo releasedbg)...
xmake f -m releasedbg -y
if errorlevel 1 goto :fail_build

echo.
echo Compilando... (la primera vez baja y compila CommonLibF4 + GNS, tarda)
xmake -y
if errorlevel 1 goto :fail_build

set "DEPLOYED=NO"
if exist "%GAMEDIR%\Data\F4SE\Plugins\F4MPClient.dll" set "DEPLOYED=SI"

echo.
echo ============================================
echo   [OK] Cliente (client-ng) compilado.
echo   Desplegado en Data\F4SE\Plugins: %DEPLOYED%
echo   Si es NO, revisa XSE_FO4_GAME_PATH o copia el DLL a mano.
echo ============================================
goto :end

:no_vs
echo [ERROR] No encuentro Visual Studio. Instalalo con la carga "Desarrollo para el escritorio con C++".
goto :end
:no_vc
echo [ERROR] Visual Studio no tiene las herramientas de C++ x64.
goto :end
:no_xmake
echo [ERROR] No encuentro xmake. Instalalo con:  winget install xmake
goto :end
:fail_build
echo [ERROR] Fallo la compilacion. Revisa el texto de arriba.
goto :end

:end
echo.
pause
endlocal
