@echo off
call :main > "c:\Users\Usuario\Desktop\ScreenFy\build_logs\step4_build.log" 2>&1
exit /b
:main
@echo off
set PATH="c:\Users\Usuario\Desktop\ScreenFy\tools\cmake\bin";%PATH%
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake --build "c:\Users\Usuario\Desktop\ScreenFy\build" --config Release
