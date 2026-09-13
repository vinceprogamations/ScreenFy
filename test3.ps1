$tempBat = 'C:\Users\Usuario\Desktop\ScreenFy\temp_build.bat'
$tempLog = 'C:\Users\Usuario\Desktop\ScreenFy\build_logs\test_wrapper.log'
$wrapperBat = "@echo off`ncall :main > `"$tempLog`" 2>&1`nexit /b`n:main`necho hello wrapper"
$wrapperBat | Out-File $tempBat -Encoding ascii
$p = Start-Process cmd.exe -ArgumentList "/c `"$tempBat`"" -WindowStyle Hidden -PassThru
$p.WaitForExit()
Write-Output $p.ExitCode
if(Test-Path $tempLog){ Get-Content $tempLog }
