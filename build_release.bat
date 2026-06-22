@echo off
setlocal
call "D:\tools\Visual Studio\Common7\Tools\VsDevCmd.bat" -arch=amd64 -vcvars_ver=14.52
if errorlevel 1 (
    echo ERROR: VsDevCmd failed
    exit /b 1
)
echo VCToolsVersion=%VCToolsVersion%
cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DCMAKE_PREFIX_PATH="D:/tools/6.12.0/msvc2022_64"
if errorlevel 1 (
    echo ERROR: CMake configure failed
    exit /b 1
)
cmake --build build/release -j1
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)
echo BUILD SUCCEEDED
