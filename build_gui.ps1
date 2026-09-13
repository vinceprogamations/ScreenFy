Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$LogDir = Join-Path $PSScriptRoot "build_logs"
if (-not (Test-Path $LogDir)) { New-Item -ItemType Directory -Path $LogDir | Out-Null }
$LogFile = Join-Path $LogDir "build_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"

$form = New-Object System.Windows.Forms.Form
$form.Text = "ScreenShare4K - Console de Compilacao Visual"
$form.Size = New-Object System.Drawing.Size(900, 600)
$form.StartPosition = "CenterScreen"
$form.BackColor = [System.Drawing.Color]::FromArgb(43, 45, 49)

$rtb = New-Object System.Windows.Forms.RichTextBox
$rtb.Dock = "Fill"
$rtb.BackColor = [System.Drawing.Color]::FromArgb(30, 31, 34)
$rtb.ForeColor = [System.Drawing.Color]::White
$rtb.Font = New-Object System.Drawing.Font("Consolas", 10)
$rtb.ReadOnly = $true
$form.Controls.Add($rtb)

$runspace = [runspacefactory]::CreateRunspace()
$runspace.ApartmentState = "STA"
$runspace.ThreadOptions = "ReuseThread"
$runspace.Open()
$runspace.SessionStateProxy.SetVariable("form", $form)
$runspace.SessionStateProxy.SetVariable("rtb", $rtb)
$runspace.SessionStateProxy.SetVariable("PSScriptRoot", $PSScriptRoot)
$runspace.SessionStateProxy.SetVariable("LogFile", $LogFile)

$psCmd = [PowerShell]::Create().AddScript({
    function Log-Message {
        param([string]$Message, [string]$Color = "White")
        if (-not $Message) { return }
        $timestamp = Get-Date -Format "HH:mm:ss"
        $fullMsg = "[$timestamp] $Message"
        Add-Content -Path $LogFile -Value $fullMsg
        $form.Invoke([action]{
            $rtb.SelectionStart = $rtb.TextLength
            $rtb.SelectionLength = 0
            $rtb.SelectionColor = [System.Drawing.Color]::FromName($Color)
            $rtb.AppendText("$fullMsg`r`n")
            $rtb.ScrollToCaret()
        })
    }
    
    function Run-Command-Stream {
        param([string]$BatContent, [string]$LogName)
        $tempBat = Join-Path $PSScriptRoot "temp_build.bat"
        $tempLog = Join-Path $PSScriptRoot "build_logs\$LogName"
        
        $wrapperBat = "@echo off`ncall :main > `"$tempLog`" 2>&1`nexit /b %ERRORLEVEL%`n:main`n$BatContent"
        $wrapperBat | Out-File $tempBat -Encoding ascii
        
        $p = Start-Process cmd.exe -ArgumentList "/c `"$tempBat`"" -WindowStyle Hidden -PassThru
        
        $lastPos = 0
        while (-not $p.HasExited) {
            Start-Sleep -Milliseconds 100
            if (Test-Path $tempLog) {
                $stream = [System.IO.File]::Open($tempLog, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
                $reader = New-Object System.IO.StreamReader($stream)
                $stream.Position = $lastPos
                while (-not $reader.EndOfStream) {
                    $line = $reader.ReadLine()
                    Log-Message $line "LightGray"
                }
                $lastPos = $stream.Position
                $reader.Close()
            }
        }
        # Read remaining lines
        if (Test-Path $tempLog) {
            $stream = [System.IO.File]::Open($tempLog, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
            $reader = New-Object System.IO.StreamReader($stream)
            $stream.Position = $lastPos
            while (-not $reader.EndOfStream) {
                $line = $reader.ReadLine()
                Log-Message $line "LightGray"
            }
            $reader.Close()
        }
        Remove-Item $tempBat -Force -ErrorAction Ignore
        return $p.ExitCode
    }

    Log-Message "Iniciando validador de compilacao (Log guardado em build_logs)..." "Cyan"
    
    Log-Message "Etapa 1: Verificando infraestrutura..." "Yellow"
    $cmakePath = Join-Path $PSScriptRoot "tools\cmake\bin\cmake.exe"
    if (-not (Test-Path $cmakePath)) {
        $sysCmake = Get-Command cmake.exe -ErrorAction SilentlyContinue
        if ($sysCmake) {
            $cmakePath = $sysCmake.Source
            Log-Message "-> CMake detectado no sistema: $cmakePath" "Lime"
        }
    }
    $vcpkgPath = Join-Path $PSScriptRoot "tools\vcpkg\scripts\buildsystems\vcpkg.cmake"
    
    if (-not (Test-Path $cmakePath)) {
        Log-Message "ERRO: CMake nao encontrado. Execute o install_tools.bat!" "Red"
        return
    }
    if (-not (Test-Path $vcpkgPath)) {
        Log-Message "ERRO: vcpkg nao encontrado. Execute o install_tools.bat!" "Red"
        return
    }
    Log-Message "-> Infraestrutura OK." "Lime"

    Log-Message "Etapa 2: Localizando Compilador C++ (MSVC)..." "Yellow"
    $pf = ${env:ProgramFiles}
    $pf86 = ${env:ProgramFiles(x86)}
    $foundVcvars = $null

    # 1. Tentar via vswhere oficial da Microsoft
    $vswherePaths = @(
        "$pf86\Microsoft Visual Studio\Installer\vswhere.exe",
        "$pf\Microsoft Visual Studio\Installer\vswhere.exe"
    )
    foreach ($vw in $vswherePaths) {
        if (Test-Path $vw) {
            $vsInst = & $vw -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
            if ($vsInst) {
                $candidate = Join-Path $vsInst "VC\Auxiliary\Build\vcvars64.bat"
                if (Test-Path $candidate) {
                    $foundVcvars = $candidate
                    break
                }
            }
        }
    }

    # 2. Fallback para caminhos tradicionais
    if (-not $foundVcvars) {
        $vcvarsPaths = @(
            "$pf\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
            "$pf\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
            "$pf\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
            "$pf\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
            "$pf86\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat",
            "$pf86\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
        )
        foreach ($path in $vcvarsPaths) {
            if (Test-Path $path) {
                $foundVcvars = $path
                break
            }
        }
    }
    
    if (-not $foundVcvars) {
        Log-Message "ERRO FATAL: O compilador C++ nao foi encontrado no seu PC!" "Red"
        Log-Message "Motivo: O 'install_tools.bat' nao instalou o Workload de C++ ou falhou em modo silencioso." "Red"
        Log-Message "Solucao: Feche esta janela, clique direito em install_tools.bat -> Executar como Administrador e aguarde!" "Red"
        return
    }
    Log-Message "-> MSVC encontrado em: $foundVcvars" "Lime"

    Log-Message "Etapa 3: Gerando build files (CMake)..." "Yellow"
    $cmakeDir = Split-Path $cmakePath
    $batCmake = "@echo off`nset PATH=`"$cmakeDir`";%PATH%`ncall `"$foundVcvars`"`ncmake -B `"$PSScriptRoot\build`" -DCMAKE_TOOLCHAIN_FILE=`"$vcpkgPath`""
    $batCmake | Out-File (Join-Path $PSScriptRoot "debug.bat") -Encoding ascii
    $exitCode = Run-Command-Stream $batCmake "step3_cmake.log"
    
    if ($exitCode -ne 0) {
        Log-Message "ERRO: Falha na geracao do projeto. Ocorreu um erro no CMake (Codigo $exitCode)." "Red"
        return
    }

    Log-Message "Etapa 4: Compilando o Projeto..." "Yellow"
    $batBuild = "@echo off`nset PATH=`"$cmakeDir`";%PATH%`ncall `"$foundVcvars`"`ncmake --build `"$PSScriptRoot\build`" --config Release"
    $exitCode = Run-Command-Stream $batBuild "step4_build.log"
    
    if ($exitCode -eq 0) {
        Log-Message "========================================" "Cyan"
        Log-Message "SUCESSO ABSOLUTO! Projeto compilado." "Lime"
        Log-Message "App Final: build\Release\ScreenShareApp.exe" "Lime"
        
        $exePath = Join-Path $PSScriptRoot "build\Release\ScreenShareApp.exe"
        $dllPath = Join-Path $PSScriptRoot "build\Release\opus.dll"
        if (Test-Path $exePath) {
            Copy-Item -Path $exePath -Destination (Join-Path $PSScriptRoot "ScreenShare.exe") -Force
            if (Test-Path $dllPath) {
                Copy-Item -Path $dllPath -Destination (Join-Path $PSScriptRoot "opus.dll") -Force
            }
            Log-Message "Pronto! Executavel gerado na raiz: ScreenShare.exe" "Lime"
        }
    } else {
        Log-Message "ERRO DE COMPILACAO. Codigo ($exitCode). Verifique logs." "Red"
    }

})
$psCmd.Runspace = $runspace

$form.Add_Shown({
    $psCmd.BeginInvoke()
})

[System.Windows.Forms.Application]::Run($form)
$runspace.Close()
