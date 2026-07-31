# Regenerate contract/kp-layouts.json from the current headers.
# Run after touching src/id_binnary.h or src/proto_binnary.h; commit the diff.
param(
    [string]$Qt      = "C:/Qt/6.8.3/llvm-mingw_64",
    [string]$Clang   = "C:/Qt/Tools/llvm-mingw1706_64/bin/clang++.exe",
    [string]$OutJson = "contract/kp-layouts.json"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Push-Location $root
try {
    New-Item -ItemType Directory -Force -Path build, (Split-Path $OutJson) | Out-Null
    $exe = "build/gen_contract.exe"

    & $Clang -std=c++23 -fno-exceptions -w `
        -I src -I "$Qt/include" -I "$Qt/include/QtCore" -I "$Qt/include/QtGui" `
        -o $exe tools/protocol_contract/gen_contract.cpp `
        -L "$Qt/lib" -lQt6Core -lQt6Gui
    if ($LASTEXITCODE -ne 0) { throw "compile failed ($LASTEXITCODE)" }

    $env:PATH = "$Qt/bin;$(Split-Path $Clang);$env:PATH"
    $json = & $exe
    if ($LASTEXITCODE -ne 0) { throw "generator failed ($LASTEXITCODE)" }

    # Validate before overwriting: a truncated contract is worse than a stale one.
    $json -join "`n" | ConvertFrom-Json | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $root $OutJson), ($json -join "`n") + "`n")

    $parsed = (Get-Content $OutJson -Raw | ConvertFrom-Json)
    $n = ($parsed.structs.PSObject.Properties | Measure-Object).Count
    $m = ($parsed.enums.PSObject.Properties   | Measure-Object).Count
    Write-Host "$OutJson - $n structs, $m enums"
}
finally { Pop-Location }
