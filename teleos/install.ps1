param()

$ErrorActionPreference = "Stop"

$InstallDir = "$env:USERPROFILE\teleos"
$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Definition

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Teleos Language Installer (PowerShell)"   -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# --- Step 1: Check g++ ---
Write-Host "[1/5] Checking for g++..." -NoNewline
$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gpp) {
    Write-Host " NOT FOUND" -ForegroundColor Red
    Write-Host ""
    Write-Host "g++ is required to build Teleos. Please install one of:" -ForegroundColor Yellow
    Write-Host "  MSYS2     : https://www.msys2.org"
    Write-Host "  MinGW-w64 : https://winlibs.com"
    Write-Host "  TDM-GCC   : https://jmeubank.github.io/tdm-gcc"
    Write-Host ""
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host " Found at $($gpp.Source)" -ForegroundColor Green

# --- Step 2: Build ---
Write-Host "[2/5] Building teleos.exe..." -NoNewline

$src = @(
    "$ScriptDir\src\main.cpp",
    "$ScriptDir\src\lexer.cpp",
    "$ScriptDir\src\parser.cpp",
    "$ScriptDir\src\interpreter.cpp"
)

$buildArgs = @(
    "-O3", "-march=native", "-std=c++17", "-flto", "-funroll-loops", "-ffast-math",
    "-I$ScriptDir\src"
) + $src + @(
    "-o", "$ScriptDir\teleos.exe",
    "-lwinhttp", "-lshell32", "-lole32"
)

$proc = Start-Process -FilePath "g++" -ArgumentList $buildArgs -Wait -PassThru -NoNewWindow
if ($proc.ExitCode -ne 0) {
    Write-Host " FAILED" -ForegroundColor Red
    Write-Host "Build failed. Check that all source files are present in src\" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host " Done" -ForegroundColor Green

# --- Step 3: Create install dir and copy files ---
Write-Host "[3/5] Installing to $InstallDir..." -NoNewline
if (-not (Test-Path $InstallDir)) { New-Item -ItemType Directory -Path $InstallDir | Out-Null }
if (-not (Test-Path "$InstallDir\examples")) { New-Item -ItemType Directory -Path "$InstallDir\examples" | Out-Null }

Copy-Item "$ScriptDir\teleos.exe" "$InstallDir\teleos.exe" -Force
Copy-Item "$ScriptDir\examples\*.tsl" "$InstallDir\examples\" -Force
Write-Host " Done" -ForegroundColor Green

# --- Step 4: Add to user PATH permanently ---
Write-Host "[4/5] Adding to user PATH..." -NoNewline

$regPath  = "HKCU:\Environment"
$current  = (Get-ItemProperty -Path $regPath -Name PATH -ErrorAction SilentlyContinue).PATH

if ($current -and $current -like "*$InstallDir*") {
    Write-Host " Already present" -ForegroundColor Yellow
} else {
    $newPath = if ($current) { "$current;$InstallDir" } else { $InstallDir }
    Set-ItemProperty -Path $regPath -Name PATH -Value $newPath -Type ExpandString

    # Broadcast WM_SETTINGCHANGE so explorer + new terminals pick it up immediately
    Add-Type -Namespace Win32 -Name NativeMethods -MemberDefinition @"
        [DllImport("user32.dll", SetLastError=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessageTimeout(
            IntPtr hWnd, uint Msg, UIntPtr wParam,
            string lParam, uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
"@ -ErrorAction SilentlyContinue
    $result = [UIntPtr]::Zero
    try {
        [Win32.NativeMethods]::SendMessageTimeout(
            [IntPtr]0xFFFF, 0x001A, [UIntPtr]::Zero,
            "Environment", 2, 5000, [ref]$result) | Out-Null
    } catch {}

    Write-Host " Done" -ForegroundColor Green
}

# --- Step 5: Patch the CURRENT session PATH so it works right now ---
Write-Host "[5/5] Activating in current session..." -NoNewline
if ($env:PATH -notlike "*$InstallDir*") {
    $env:PATH = "$env:PATH;$InstallDir"
}
Write-Host " Done" -ForegroundColor Green

# --- Verify it actually works ---
Write-Host ""
Write-Host "Verifying..." -NoNewline
$check = Get-Command teleos -ErrorAction SilentlyContinue
if ($check) {
    Write-Host " OK — teleos found at $($check.Source)" -ForegroundColor Green
} else {
    Write-Host " WARNING: not found in this session (open a new terminal)" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Teleos installed!"                         -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Run RIGHT NOW (this window):"
Write-Host "    teleos examples\hello.tsl" -ForegroundColor White
Write-Host ""
Write-Host "  Or open any new terminal and type:"
Write-Host "    teleos yourscript.tsl" -ForegroundColor White
Write-Host ""
Read-Host "Press Enter to exit"
