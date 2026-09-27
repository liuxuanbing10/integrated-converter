@echo off
rem Run the unit-test suite. Usage: run_tests.bat [Debug^|Release]
rem Assumes the matching cmake preset is already configured.
setlocal
set "CFG=%~1"
if "%CFG%"=="" set "CFG=Debug"
if /I "%CFG%"=="Release" (set "BIN=build\release\tests") else (set "BIN=build\debug\tests")

if not exist "%BIN%\test_runner.exe" (
    echo test_runner.exe not found in %BIN% — building it first...
    cmake --build %BIN%\.. --target test_runner || exit /b 1
)

echo Running %BIN%\test_runner.exe ...
"%BIN%\test_runner.exe"
set "RC=%ERRORLEVEL%"
echo EXIT_CODE=%RC%
if exist "%BIN%\test_results.log" (
    echo --- test_results.log ---
    type "%BIN%\test_results.log"
)
exit /b %RC%
