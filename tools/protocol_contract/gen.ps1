# Regenerate the KP wire contracts and commit the diff.
#
#   gen.ps1              -> contract/kp-layouts.json           from src/id_binnary.h
#   gen.ps1 -Firmware    -> contract/kp-layouts-firmware.json  from the USBL firmware repo
#
# Run both after touching either side's headers. `kpdevtool.py contract-diff` compares
# them, so a header change the other side has not followed shows up as a finding instead
# of as a frame the device silently rejects.
param(
    [string]$Qt      = "C:/Qt/6.8.3/llvm-mingw_64",
    [string]$Clang   = "C:/Qt/Tools/llvm-mingw1706_64/bin/clang++.exe",
    [switch]$Firmware,
    [string]$FwRoot  = "D:/Kogger/EmbedCode/USBL-agent/io/Parser",
    [string]$OutJson
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Push-Location $root
try {
    if (-not $OutJson) {
        if ($Firmware) { $OutJson = "contract/kp-layouts-firmware.json" }
        else           { $OutJson = "contract/kp-layouts.json" }
    }
    New-Item -ItemType Directory -Force -Path build, (Split-Path $OutJson) | Out-Null

    if ($Firmware) {
        if (-not (Test-Path "$FwRoot/PayloadDefines.h")) {
            throw "no PayloadDefines.h under $FwRoot - pass -FwRoot <repo>/io/Parser"
        }
        $exe = "build/gen_contract_fw.exe"
        # fwshim/ supplies the two MCU headers PayloadDefines.h pulls in. Nothing else
        # from the firmware tree compiles on the host, and nothing else is needed to
        # read struct layouts.
        & $Clang -std=c++23 -fno-exceptions -w `
            -I $FwRoot -I tools/protocol_contract/fwshim `
            -o $exe tools/protocol_contract/gen_contract_fw.cpp
    }
    else {
        $exe = "build/gen_contract.exe"
        & $Clang -std=c++23 -fno-exceptions -w `
            -I src -I "$Qt/include" -I "$Qt/include/QtCore" -I "$Qt/include/QtGui" `
            -o $exe tools/protocol_contract/gen_contract.cpp `
            -L "$Qt/lib" -lQt6Core -lQt6Gui
    }
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
