@echo off
call "D:\tools\Visual Studio\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
set PATH=D:\tools\6.12.0\msvc2022_64\bin;%PATH%
cd /d "%~dp0"
echo Running test_runner.exe...
.\build\release\tests\test_runner.exe
echo EXIT_CODE=%ERRORLEVEL%
echo.
echo Checking for test_results.log...
if exist test_results.log (
    echo FOUND test_results.log:
    type test_results.log
) else (
    echo NOT FOUND test_results.log
)
if exist build\release\tests\test_results.log (
    echo FOUND build\release\tests\test_results.log:
    type build\release\tests\test_results.log
) else (
    echo NOT FOUND build\release\tests\test_results.log
)
