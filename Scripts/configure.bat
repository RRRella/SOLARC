@echo off
mkdir "../build"
cmake -S .. -B "../build" -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchains/windows-msvc.cmake
pause