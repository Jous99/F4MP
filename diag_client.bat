@echo off
setlocal
cd /d "%~dp0"
set "LOG=%~dp0build_log.txt"

echo F4MP - diagnostico build cliente > "%LOG%"
echo Fecha: %date% %time% >> "%LOG%"
echo. >> "%LOG%"
echo Compilando... espera a que ponga "Listo" (puede tardar).

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :novs

set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto :novc

echo VSPATH=%VSPATH% >> "%LOG%"
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set "PATH=%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

echo. >> "%LOG%"
echo [where cmake] >> "%LOG%"
where cmake >> "%LOG%" 2>&1

echo. >> "%LOG%"
echo ===================== CONFIGURE ===================== >> "%LOG%"
cmake -S client -B client\buildx -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo >> "%LOG%" 2>&1

echo. >> "%LOG%"
echo ===================== BUILD ===================== >> "%LOG%"
cmake --build client\buildx >> "%LOG%" 2>&1

echo. >> "%LOG%"
echo ===================== FIN ===================== >> "%LOG%"

if exist "client\buildx\F4MPClient.dll" copy /Y "client\buildx\F4MPClient.dll" "client\buildx\F4MPClient.asi" >nul 2>&1
goto :done

:novs
echo [ERROR] No encuentro Visual Studio >> "%LOG%"
goto :done
:novc
echo [ERROR] Visual Studio sin herramientas C++ >> "%LOG%"
goto :done

:done
echo.
echo ============================================
echo   Listo. Se creo build_log.txt en la carpeta.
echo   Avisa a Claude de que ya esta.
echo ============================================
pause
endlocal
