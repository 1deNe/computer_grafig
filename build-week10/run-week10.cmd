@echo off
cd /d "%~dp0"
if exist "build-local\bin\Debug\week10.exe" (
    "build-local\bin\Debug\week10.exe"
) else if exist "build-local\bin\week10.exe" (
    "build-local\bin\week10.exe"
) else (
    echo Build the week10 target with CMake first. See README.md.
    pause
)
