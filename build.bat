@echo off
if not exist "build_logs" mkdir "build_logs"
powershell.exe -ExecutionPolicy Bypass -WindowStyle Hidden -File "%~dp0build_gui.ps1"
exit /b
