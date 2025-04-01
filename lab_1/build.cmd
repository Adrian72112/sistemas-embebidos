@echo off
chcp 65001 >nul
setlocal

:: Leer el nombre del ejecutable desde CMakeLists.txt usando PowerShell
set EXECUTABLE=
for /f "delims=" %%A in ('powershell -Command ^
    "$match = Select-String -Path 'CMakeLists.txt' -Pattern 'add_executable\((\S+)' | ForEach-Object { $_.Matches.Groups[1].Value }; if ($match) { echo $match } else { echo ERROR }"') do (
    set EXECUTABLE=%%A
)

:: Verificar si se encontró un nombre válido
if "%EXECUTABLE%"=="ERROR" (
    echo ❌ Error: No se encontró un ejecutable en CMakeLists.txt
    exit /b 1
)

:: Mostrar el nombre del ejecutable detectado
echo 🏗️ Nombre del ejecutable detectado: %EXECUTABLE%

set BUILD_DIR=build

if "%1"=="" goto :usage

if "%1"=="compile" (
    echo 📦 Creando directorio %BUILD_DIR%...
    if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
    cd "%BUILD_DIR%"
    
    echo 🔧 Ejecutando CMake...
    cmake .. -G "MinGW Makefiles"
    
    echo ⚙️ Compilando con mingw32-make...
    mingw32-make
    
    cd ..
    echo ✅ Compilación completada: %EXECUTABLE%.exe
    exit /b
)

if "%1"=="clean" (
    echo 🧹 Limpiando el directorio %BUILD_DIR%...
    rmdir /s /q "%BUILD_DIR%"
    mkdir "%BUILD_DIR%"
    echo ✅ Directorio limpio.
    exit /b
)

if "%1"=="run" (
    echo 🚀 Buscando ejecutable: %BUILD_DIR%\%EXECUTABLE%.exe...
    if exist "%BUILD_DIR%\%EXECUTABLE%.exe" (
        echo ✅ Ejecutando %EXECUTABLE%...
        "%BUILD_DIR%\%EXECUTABLE%.exe"
    ) else (
        echo ❌ Error: No se encontró %EXECUTABLE%.exe. Compila primero con "build.cmd compile".
    )
    exit /b
)

:usage
echo ❌ Uso: build.cmd {compile|clean|run}
exit /b
