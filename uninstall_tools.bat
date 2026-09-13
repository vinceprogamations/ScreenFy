@echo off
setlocal enabledelayedexpansion
title ScreenFy - Limpeza e Desinstalacao Segura
color 0e

echo ==============================================================================
echo                 SCREENFY 4K - GERENCIADOR DE LIMPEZA
echo ==============================================================================
echo Escolha o que deseja limpar ou desinstalar.
echo O codigo-fonte (src) e arquivos essenciais NUNCA serao apagados acidentalmente.
echo ==============================================================================
echo.
echo [1] Limpar apenas compilacoes, caches e executaveis locais
echo     (Remove: build\, build_logs\, logs\, ScreenShare.exe)
echo.
echo [2] Limpar ferramentas e bibliotecas externas baixadas
echo     (Remove: tools\vcpkg, tools\cmake)
echo.
echo [3] Limpar dados e configuracoes do utilizador
echo     (Remove: %%APPDATA%%\ScreenShare4K - dados de amigos e configuracoes)
echo.
echo [4] Limpeza TOTAL do ambiente de build (Opcoes 1 + 2 + 3)
echo     (Mantem seu codigo fonte intacto como no GitHub)
echo.
echo [5] Cancelar e Sair
echo ==============================================================================

set /p OPCAO="Digite a opcao desejada [1-5]: "

if "%OPCAO%"=="1" goto CLEAN_BUILDS
if "%OPCAO%"=="2" goto CLEAN_TOOLS
if "%OPCAO%"=="3" goto CLEAN_APPDATA
if "%OPCAO%"=="4" goto CLEAN_ALL
if "%OPCAO%"=="5" goto EXIT
goto INVALID

:CLEAN_BUILDS
echo.
echo [INFO] Apagando pastas de compilacao e caches...
if exist "build" rmdir /s /q "build"
if exist "build_logs" rmdir /s /q "build_logs"
if exist "logs" rmdir /s /q "logs"
if exist "ScreenShare.exe" del /f /q "ScreenShare.exe"
if exist "temp_build.bat" del /f /q "temp_build.bat"
if exist "debug.bat" del /f /q "debug.bat"
echo [SUCESSO] Compilacoes e arquivos temporarios limpos com sucesso!
goto FINISH

:CLEAN_TOOLS
echo.
echo [INFO] Apagando pasta tools (vcpkg e cmake)...
if exist "tools" rmdir /s /q "tools"
echo [SUCESSO] Ferramentas e bibliotecas de terceiros removidas!
goto FINISH

:CLEAN_APPDATA
echo.
echo [INFO] Apagando configuracoes e historico do ScreenFy em AppData...
if exist "%APPDATA%\ScreenShare4K" rmdir /s /q "%APPDATA%\ScreenShare4K"
echo [SUCESSO] Dados de perfil e preferencias restaurados para o padrao!
goto FINISH

:CLEAN_ALL
echo.
echo [INFO] Executando limpeza completa de ambiente...
if exist "build" rmdir /s /q "build"
if exist "build_logs" rmdir /s /q "build_logs"
if exist "logs" rmdir /s /q "logs"
if exist "tools" rmdir /s /q "tools"
if exist "ScreenShare.exe" del /f /q "ScreenShare.exe"
if exist "temp_build.bat" del /f /q "temp_build.bat"
if exist "debug.bat" del /f /q "debug.bat"
if exist "%APPDATA%\ScreenShare4K" rmdir /s /q "%APPDATA%\ScreenShare4K"
echo [SUCESSO] Ambiente completamente limpo e restaurado ao estado original!
goto FINISH

:INVALID
echo [ERRO] Opcao invalida. Operacao cancelada.
goto EXIT

:FINISH
echo.
echo ==============================================================================
echo Operacao concluida com sucesso.
echo ==============================================================================

:EXIT
pause
