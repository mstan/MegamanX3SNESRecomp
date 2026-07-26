# play.ps1 — one-click regen + build + launch for MegaManX3SNESRecomp.
#
# Two configurations:
#   -Config debug (default)  build-trace/  SNESRECOMP_ENABLE_TRACE=ON
#                            TCP debug server + always-on observability rings.
#                            This is the bring-up configuration: runs below
#                            realtime, audio is not representative.
#   -Config prod             build-play/   TRACE=OFF, representative audio.
#
# Uses the mingw64 toolchain explicitly — the `cmake` first on PATH is the
# MSYS/devkitPro one, which mangles Windows paths.
#
# Usage:
#   tools/play.ps1                       # regen + debug build + launch
#   tools/play.ps1 -SkipRegen            # reuse current src/gen
#   tools/play.ps1 -NoRun                # build only, print exe path
#   tools/play.ps1 -Config prod          # non-trace release build
param(
    [switch]$NoRun,
    [switch]$SkipRegen,
    [ValidateSet('debug', 'prod')] [string]$Config = 'debug'
)
$ErrorActionPreference = 'Stop'

$repo    = Split-Path $PSScriptRoot -Parent
$mingw   = 'C:\msys64\mingw64\bin'
$cmake   = "$mingw\cmake.exe"
$exeName = 'MegaManX3SNESRecomp.exe'
$rom     = Join-Path $repo 'mmx3.sfc'

$env:PATH = "$mingw;$env:PATH"
$env:SNESRECOMP_ANALYSIS_BACKEND = 'native'
Set-Location $repo

Get-Process ($exeName -replace '\.exe$', '') -ErrorAction SilentlyContinue |
    Stop-Process -Force

if (-not $SkipRegen) {
    Write-Host "=== regen (--cfg-roots) ===" -ForegroundColor Cyan
    & "$mingw\bash.exe" tools/regen.sh --no-tests
    if ($LASTEXITCODE -ne 0) { throw "regen failed" }
}

if ($Config -eq 'debug') {
    $bd    = Join-Path $repo 'build-trace'
    $trace = 'ON'
} else {
    $bd    = Join-Path $repo 'build-play'
    $trace = 'OFF'
}

if (-not (Test-Path (Join-Path $bd 'CMakeCache.txt'))) {
    Write-Host "=== configure $Config (TRACE=$trace) ===" -ForegroundColor Cyan
    & $cmake -G Ninja -B $bd -S $repo `
        -DCMAKE_BUILD_TYPE=RelWithDebInfo `
        -DSNESRECOMP_ENABLE_TRACE=$trace `
        -DCMAKE_C_COMPILER="$mingw/gcc.exe" `
        -DCMAKE_CXX_COMPILER="$mingw/g++.exe" `
        -DCMAKE_MAKE_PROGRAM="$mingw/ninja.exe"
    if ($LASTEXITCODE -ne 0) { throw "configure failed" }
}

Write-Host "=== build ===" -ForegroundColor Cyan
& $cmake --build $bd -j 4
if ($LASTEXITCODE -ne 0) { throw "build failed" }

$exe = Join-Path $bd $exeName
Write-Host "Built: $exe" -ForegroundColor Green
if ($NoRun) { return }
Write-Host "=== launch ===" -ForegroundColor Cyan
Start-Process $exe -ArgumentList $rom -WorkingDirectory $bd
