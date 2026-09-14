@echo off
setlocal enabledelayedexpansion
title ScreenFy 4K

:: Garantir que o CWD é a pasta raiz do projeto
cd /d "%~dp0"

echo ==============================================================================
echo                      A INICIAR O SCREENFY 4K...
echo ==============================================================================

if not exist "build\Release\ScreenShareApp.exe" (
    echo [ERRO] O ScreenFy ainda nao foi instalado ou compilado!
    echo Por favor, executa o INSTALL.bat primeiro.
    pause
    exit /b
)

if not exist "build\Release\opus.dll" (
    echo [AVISO] Falta a biblioteca de audio opus.dll. A copiar...
    copy /y opus.dll build\Release\opus.dll >nul
)

cd build\Release
start ScreenShareApp.exe
exit
