@echo off
setlocal

set OUT=teleos.exe
set SRC=src\main.cpp src\lexer.cpp src\parser.cpp src\interpreter.cpp
set FLAGS=-O3 -march=native -std=c++17 -flto -funroll-loops -ffast-math
set INCLUDES=-Isrc
set LIBS=-lwinhttp -lshell32 -lole32 -lws2_32 -ladvapi32

echo [Teleos Build] Compiling...

g++ %FLAGS% %INCLUDES% %SRC% -o %OUT% %LIBS%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [Teleos Build] FAILED. Make sure g++ is installed and in your PATH.
    exit /b 1
)

echo [Teleos Build] Success! Output: %OUT%
echo.
echo To install globally (no admin needed), run:
echo   install.bat
echo.
echo Or run directly from this folder:
echo   teleos.exe examples\fulldemo.tsl
endlocal
