@echo off
title Teleos Installer

net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Set UAC = CreateObject^("Shell.Application"^) > "%TEMP%\elevate.vbs"
    echo UAC.ShellExecute "cmd.exe", "/d /k cd /d ""%~dp0"" && ""%~f0""", "", "runas", 1 >> "%TEMP%\elevate.vbs"
    cscript //NoLogo "%TEMP%\elevate.vbs"
    del "%TEMP%\elevate.vbs"
    exit /b
)

cd /d "%~dp0"
title Teleos Installer [Admin]
echo.
echo ============================================
echo  Teleos Installer  [Admin OK]
echo ============================================
echo.

echo [1/5] Checking for g++...
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo  ERROR: g++ not found in PATH.
    echo  Install MSYS2, MinGW-w64, or TDM-GCC first.
    echo.
    pause & exit /b 1
)
echo       OK

echo [2/5] Building teleos.exe...
g++ -O3 -march=native -std=c++17 -flto -funroll-loops -ffast-math -Isrc src/main.cpp src/lexer.cpp src/parser.cpp src/interpreter.cpp -o teleos.exe -lwinhttp -lshell32 -lole32 -lws2_32 -ladvapi32
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo  ERROR: Build failed. Read the errors above.
    echo.
    pause & exit /b 1
)
echo       OK

echo [3/5] Copying to C:\teleos ...
if not exist "C:\teleos" mkdir "C:\teleos"
if not exist "C:\teleos\examples" mkdir "C:\teleos\examples"
copy /Y teleos.exe "C:\teleos\teleos.exe" >nul
copy /Y examples\*.tsl "C:\teleos\examples\" >nul
echo       OK

echo [4/5] Adding C:\teleos to system PATH...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p = [Environment]::GetEnvironmentVariable('Path','Machine'); if ($p -notlike '*C:\teleos*') { [Environment]::SetEnvironmentVariable('Path', $p + ';C:\teleos', 'Machine'); Write-Host '      Added.' } else { Write-Host '      Already present.' }"

echo [5/5] Refreshing PATH in this session...
set "PATH=%PATH%;C:\teleos"
echo       OK

echo.
echo  Testing...
echo ----------------------------------------
C:\teleos\teleos.exe examples\hello.tsl
echo ----------------------------------------
echo.
if %ERRORLEVEL% EQU 0 (
    echo  SUCCESS - open a new terminal and type: teleos yourscript.tsl
) else (
    echo  WARNING: Installed but test failed.
)
echo.
pause
