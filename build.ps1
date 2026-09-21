# Video Downloader - build script (identyczny jak w Resource Extractor: MinGW g++ + windres)
# Usage:  powershell -ExecutionPolicy Bypass -File build.ps1
param(
    [string]$Compiler = "C:\msys64\ucrt64\bin\g++.exe",
    [string]$OutDir = "bin"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "Compiler : $Compiler"
& $Compiler --version | Select-Object -First 1

New-Item -ItemType Directory -Force -Path (Join-Path $root $OutDir) | Out-Null

# 1) Resources (version info + manifest) -> app_res.o
# windres potrzebuje gcc w PATH do preprocessingu
$mingwBin = Split-Path -Parent $Compiler
$env:PATH = "$mingwBin;$env:PATH"
$rc = Join-Path $mingwBin "windres.exe"
$obj = Join-Path $root "app_res.o"
if (Test-Path $rc) {
    & $rc -i (Join-Path $root "app.rc") -O coff -o $obj
    if ($LASTEXITCODE -ne 0) { Write-Error "windres failed" }
    Write-Host "Resources: app.rc -> app_res.o"
} else {
    Write-Warning "windres.exe not found next to compiler - building without version info/manifest."
    $obj = $null
}

# 2) Compile + link (static libgcc/libstdc++/zlib so the exe runs standalone)
$exe = Join-Path $root "$OutDir\Pobieracz.exe"
$flags = @("-std=c++17","-O2","-municode","-mwindows",
           (Join-Path $root "pobieracz.cpp"))
if ($obj) { $flags += $obj }
$flags += @("-o",$exe,
            "-lcomdlg32","-lshell32","-lole32","-lcomctl32","-lgdi32","-lz",
            "-static-libgcc","-static-libstdc++","-static")

Write-Host "Building  : $exe"
& $Compiler @flags
if ($LASTEXITCODE -ne 0) { Write-Error "Compilation failed" }

if ($obj -and (Test-Path $obj)) { Remove-Item $obj }

Write-Host ""
Write-Host "Done: $exe" -ForegroundColor Green
