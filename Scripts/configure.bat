@echo off
mkdir "../build"
cmake -S .. -B "../build" -G "Visual Studio 18 2026" -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchains/windows-msvc.cmake
pause