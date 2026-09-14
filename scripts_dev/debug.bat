@echo off
set PATH="C:\Users\Usuario\Desktop\ScreenFy\tools\cmake\bin";%PATH%
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -B "C:\Users\Usuario\Desktop\ScreenFy\build" -DCMAKE_TOOLCHAIN_FILE="C:\Users\Usuario\Desktop\ScreenFy\tools\vcpkg\scripts\buildsystems\vcpkg.cmake"
