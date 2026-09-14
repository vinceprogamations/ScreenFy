@echo off
setlocal enabledelayedexpansion
title Instalar ScreenFy 4K
color 0b

:: Garantir que o CWD é a pasta raiz do projeto
cd /d "%~dp0"

echo ==============================================================================
echo                      BEM-VINDO AO SCREENFY 4K
echo ==============================================================================
echo.
echo Este instalador automatico vai preparar tudo para ti!
echo (Vai instalar dependencias, compilar o codigo e criar um atalho)
echo.
pause

:: 1. Executar o script de instalacao de dependencias
echo.
echo [1/3] A preparar ferramentas (C++, CMake, vcpkg)...
call scripts_dev\install_tools.bat

:: 2. Executar o script de compilacao
echo.
echo [2/3] A compilar o ScreenFy 4K...
call scripts_dev\build.bat

:: 3. Copiar as DLLs necessarias para a pasta de Release
echo.
echo [3/3] A configurar ficheiros do jogo (DLLs) e Atalhos...
if not exist "build\Release" mkdir "build\Release"
copy /y opus.dll build\Release\opus.dll >nul

:: Criar Atalho na Area de Trabalho
set SCRIPT="%TEMP%\CreateShortcut.vbs"
echo Set oWS = WScript.CreateObject("WScript.Shell") > %SCRIPT%
echo sLinkFile = "%USERPROFILE%\Desktop\ScreenFy 4K.lnk" >> %SCRIPT%
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> %SCRIPT%
echo oLink.TargetPath = "%~dp0build\Release\ScreenShareApp.exe" >> %SCRIPT%
echo oLink.WorkingDirectory = "%~dp0build\Release" >> %SCRIPT%
echo oLink.IconLocation = "%~dp0build\Release\ScreenShareApp.exe, 0" >> %SCRIPT%
echo oLink.Description = "Partilha o teu ecra a 4K 120 FPS" >> %SCRIPT%
echo oLink.Save >> %SCRIPT%

cscript /nologo %SCRIPT%
del %SCRIPT%
echo [OK] Atalho "ScreenFy 4K" criado com sucesso!

echo.
echo ==============================================================================
echo INSTALACAO CONCLUIDA COM SUCESSO!
echo Podes iniciar a aplicacao usando o atalho na tua area de trabalho.
echo ==============================================================================
echo.
pause
