@echo off
setlocal EnableDelayedExpansion

echo ============================================
echo  Teleos Language Installer
echo  No admin required - installs to user PATH
echo ============================================
echo.

set INSTALL_DIR=%USERPROFILE%\teleos

echo [1/4] Creating install directory: %INSTALL_DIR%
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
if not exist "%INSTALL_DIR%\examples" mkdir "%INSTALL_DIR%\examples"

echo [2/4] Building teleos.exe with g++...

where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] g++ not found in PATH.
    echo Please install one of the following:
    echo   - MSYS2: https://www.msys2.org
    echo   - MinGW-w64: https://winlibs.com
    echo   - TDM-GCC: https://jmeubank.github.io/tdm-gcc
    echo After installing, re-run this script.
    pause
    exit /b 1
)

set SRC=src\main.cpp src\lexer.cpp src\parser.cpp src\interpreter.cpp
set FLAGS=-O3 -march=native -std=c++17 -flto -funroll-loops -ffast-math
set INCLUDES=-Isrc
set LIBS=-lwinhttp -lshell32 -lole32

g++ %FLAGS% %INCLUDES% %SRC% -o "%INSTALL_DIR%\teleos.exe" %LIBS%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed. Check that all source files are present.
    pause
    exit /b 1
)

echo [3/4] Copying examples...
copy /Y examples\*.tsl "%INSTALL_DIR%\examples\" >nul

echo [4/4] Adding %INSTALL_DIR% to your user PATH...

for /f "tokens=2*" %%A in ('reg query "HKCU\Environment" /v PATH 2^>nul') do set CURRENT_PATH=%%B

echo !CURRENT_PATH! | findstr /i "%INSTALL_DIR%" >nul
if %ERRORLEVEL% EQU 0 (
    echo     Already in PATH, skipping.
) else (
    if defined CURRENT_PATH (
        setx PATH "!CURRENT_PATH!;%INSTALL_DIR%"
    ) else (
        setx PATH "%INSTALL_DIR%"
    )
    echo     Done.
)

echo.
echo ============================================
echo  Teleos installed successfully!
echo ============================================
echo.
echo  Location : %INSTALL_DIR%
echo  Command  : teleos ^<script.tsl^>
echo  Example  : teleos examples\hello.tsl
echo.
echo  IMPORTANT: Open a NEW terminal window for
echo             the PATH change to take effect.
echo.
pause
endlocal
