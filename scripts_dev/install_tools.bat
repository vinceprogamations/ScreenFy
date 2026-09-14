@echo off
setlocal enabledelayedexpansion
title ScreenFy - Instalador Inteligente de Ferramentas
color 0b

echo ==============================================================================
echo                 SCREENFY 4K - CONFIGURACAO DE AMBIENTE
echo ==============================================================================
echo Este script prepara o seu computador para compilar e rodar o ScreenFy.
echo Tudo e configurado automaticamente dentro da pasta do projeto.
echo ==============================================================================
echo.

set TOOLS_DIR=%~dp0tools
if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"

:: -------------------------------------------------------------------------
:: 1. VERIFICAR COMPILADOR C++ (MSVC / VISUAL STUDIO)
:: -------------------------------------------------------------------------
echo [1/4] Verificando Compilador C++ (Visual Studio 2019/2022)...

set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% set VSWHERE="%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

set VS_INSTALL_PATH=
if exist %VSWHERE% (
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set VS_INSTALL_PATH=%%i
    )
)

if not defined VS_INSTALL_PATH (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_INSTALL_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_INSTALL_PATH=C:\Program Files\Microsoft Visual Studio\2022\BuildTools"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_INSTALL_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        set "VS_INSTALL_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    )
)

if defined VS_INSTALL_PATH (
    echo [OK] Visual Studio com C++ detectado em:
    echo      !VS_INSTALL_PATH!
) else (
    echo [AVISO] Compilador C++ nao detectado automaticamente.
    set VS_INSTALLER="C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe"
    if exist !VS_INSTALLER! (
        echo [INFO] A iniciar instalacao automatica da workload C++ no Visual Studio...
        echo        (Aguarde o instalador terminar...)
        !VS_INSTALLER! modify --installPath "C:\Program Files\Microsoft Visual Studio\2022\Community" --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --passive
    ) else (
        echo [ERRO] Visual Studio nao encontrado.
        echo Baixe gratuitamente em: https://visualstudio.microsoft.com/vs/community/
        echo Certifique-se de marcar a opcao "Desenvolvimento para Desktop com C++".
    )
)

echo.

:: -------------------------------------------------------------------------
:: 2. VERIFICAR GIT
:: -------------------------------------------------------------------------
echo [2/4] Verificando Git...
where git >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo [OK] Git encontrado no PATH do sistema.
) else (
    if exist "C:\Program Files\Git\cmd\git.exe" (
        set "PATH=C:\Program Files\Git\cmd;%PATH%"
        echo [OK] Git localizado em C:\Program Files\Git\cmd
    ) else (
        echo [ERRO] Git nao encontrado! O Git e necessario para baixar dependencias.
        echo Instale o Git para Windows em: https://git-scm.com/download/win
        pause
        exit /b 1
    )
)

echo.

:: -------------------------------------------------------------------------
:: 3. VERIFICAR CMAKE
:: -------------------------------------------------------------------------
echo [3/4] Verificando CMake...
set CMAKE_BIN=
where cmake >nul 2>nul
if %ERRORLEVEL% equ 0 (
    echo [OK] CMake detectado no PATH do sistema.
) else (
    if exist "%TOOLS_DIR%\cmake\bin\cmake.exe" (
        echo [OK] CMake portatil pronto em: %TOOLS_DIR%\cmake
    ) else (
        echo [INFO] Baixando CMake portatil oficial (v3.27.4)...
        powershell -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/Kitware/CMake/releases/download/v3.27.4/cmake-3.27.4-windows-x86_64.zip' -OutFile '%TOOLS_DIR%\cmake.zip'"
        if not exist "%TOOLS_DIR%\cmake.zip" (
            echo [ERRO] Falha ao transferir CMake. Verifique a conexao com a internet.
            pause
            exit /b 1
        )
        echo [INFO] Extraindo CMake...
        powershell -ExecutionPolicy Bypass -Command "Expand-Archive -Path '%TOOLS_DIR%\cmake.zip' -DestinationPath '%TOOLS_DIR%\cmake_temp' -Force"
        move "%TOOLS_DIR%\cmake_temp\cmake-3.27.4-windows-x86_64" "%TOOLS_DIR%\cmake" >nul
        rmdir /s /q "%TOOLS_DIR%\cmake_temp" 2>nul
        del "%TOOLS_DIR%\cmake.zip" 2>nul
        echo [OK] CMake portatil instalado com sucesso em %TOOLS_DIR%\cmake
    )
)

echo.

:: -------------------------------------------------------------------------
:: 4. CONFIGURAR VCPKG E DEPENDENCIAS
:: -------------------------------------------------------------------------
echo [4/4] Verificando vcpkg (Gerenciador de Bibliotecas C++)...
if not exist "%TOOLS_DIR%\vcpkg\vcpkg.exe" (
    if not exist "%TOOLS_DIR%\vcpkg" (
        echo [INFO] Clonando repositorio oficial vcpkg...
        git clone --depth 1 https://github.com/microsoft/vcpkg.git "%TOOLS_DIR%\vcpkg"
    )
    echo [INFO] Compilando vcpkg bootstrap...
    call "%TOOLS_DIR%\vcpkg\bootstrap-vcpkg.bat" -disableMetrics
    if not exist "%TOOLS_DIR%\vcpkg\vcpkg.exe" (
        echo [ERRO] Falha ao compilar vcpkg.
        pause
        exit /b 1
    )
    echo [OK] vcpkg configurado com sucesso!
) else (
    echo [OK] vcpkg ja se encontra instalado e operacional.
)

echo.
echo ==============================================================================
echo                   TODAS AS FERRAMENTAS ESTAO PRONTAS!
echo ==============================================================================
echo Agora execute o arquivo:
echo     build.bat  (ou clique direito em build_gui.ps1 -^> Executar com PowerShell)
echo ==============================================================================
echo.
pause
