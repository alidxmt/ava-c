@echo off
setlocal

REM Clean optional
REM rmdir /s /q build

REM Create build dir if not exists
if not exist build mkdir build

cd build

REM Configure (only needed the first time or if CMakeLists.txt changes)
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build Debug config
cmake --build . --config Debug

cd ..
endlocal
