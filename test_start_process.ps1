$tempBat = 'C:\Users\Usuario\Desktop\ScreenFy\temp_build.bat'
$tempLog = 'C:\Users\Usuario\Desktop\ScreenFy\build_logs\test.log'
'echo hello' | Out-File $tempBat -Encoding ascii
$cmdArgs = "/c `"`"$tempBat`" > `"$tempLog`" 2>&1`""
$p = Start-Process cmd.exe -ArgumentList $cmdArgs -WindowStyle Hidden -PassThru
$p.WaitForExit()
Write-Output 'ExitCode:'
Write-Output $p.ExitCode
if(Test-Path $tempLog){ Get-Content $tempLog }
