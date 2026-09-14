$tempBat = 'C:\Users\Usuario\Desktop\ScreenFy\temp_build.bat'
$tempLog = 'C:\Users\Usuario\Desktop\ScreenFy\build_logs\test2.log'
'echo hello' | Out-File $tempBat -Encoding ascii
$p = Start-Process $tempBat -WindowStyle Hidden -PassThru -RedirectStandardOutput $tempLog -RedirectStandardError $tempLog
$p.WaitForExit()
Write-Output 'ExitCode:'
Write-Output $p.ExitCode
if(Test-Path $tempLog){ Get-Content $tempLog }
