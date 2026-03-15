@echo off
setlocal EnableDelayedExpansion

echo ============================================
echo  Teleos Uninstaller
echo ============================================
echo.

set INSTALL_DIR=%USERPROFILE%\teleos

echo [1/2] Removing %INSTALL_DIR% from user PATH...

for /f "tokens=2*" %%A in ('reg query "HKCU\Environment" /v PATH 2^>nul') do set CURRENT_PATH=%%B

if not defined CURRENT_PATH (
    echo     PATH entry not found, skipping.
    goto REMOVE_DIR
)

set NEW_PATH=
for %%P in ("!CURRENT_PATH:;=" "!") do (
    set PART=%%~P
    if /i not "!PART!"=="%INSTALL_DIR%" (
        if defined NEW_PATH (
            set NEW_PATH=!NEW_PATH!;!PART!
        ) else (
            set NEW_PATH=!PART!
        )
    )
)

setx PATH "!NEW_PATH!" >nul
echo     Removed from PATH.

:REMOVE_DIR
echo [2/2] Deleting install directory...
if exist "%INSTALL_DIR%" (
    rmdir /S /Q "%INSTALL_DIR%"
    echo     Deleted %INSTALL_DIR%
) else (
    echo     Directory not found, skipping.
)

echo.
echo  Teleos has been uninstalled.
echo  Open a new terminal for changes to take effect.
echo.
pause
endlocal
