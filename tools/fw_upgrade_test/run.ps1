# Build and run the firmware-upgrade regression tests.
#
#   run.ps1 -Firmware "C:\path\RecorderFiles.ufw"          # everything
#   run.ps1 -Firmware ... -Scenario happy-path             # one driver scenario
#   run.ps1 -Firmware ... -SkipLink                        # driver scenarios only
#   run.ps1 -Build                                         # compile, do not run
#
# No hardware and no real port. A non-zero exit means a check failed.
param(
    [string] $Qt       = "C:/Qt/6.8.3/llvm-mingw_64",
    [string] $Clang    = "C:/Qt/Tools/llvm-mingw1706_64/bin/clang++.exe",
    [string] $Firmware = "",
    [string[]] $Scenario = @(),
    [switch] $SkipLink,
    [switch] $Build
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Push-Location $root
try {
    $moc = Join-Path $Qt "bin/moc.exe"
    foreach ($f in @($Clang, $moc)) {
        if (-not (Test-Path $f)) { throw "not found: $f  (pass -Qt / -Clang)" }
    }

    $out = "build/fw_upgrade_test"
    New-Item -ItemType Directory -Force -Path $out | Out-Null

    # shim must precede src so <core.h> resolves to the stub
    $inc = @(
        "-I", "tools/fw_upgrade_test/shim",
        "-I", "src",
        "-I", "tools/fw_upgrade_test",
        "-I", "$Qt/include",
        "-I", "$Qt/include/QtCore",
        "-I", "$Qt/include/QtGui",
        "-I", "$Qt/include/QtNetwork",
        "-I", "$Qt/include/QtSerialPort"
    )

    # ---- moc -------------------------------------------------------------------
    $mocHeaders = @{
        "src/id_binnary.h"          = "$out/moc_id_binnary.cpp"
        "src/device/dev_driver.h"   = "$out/moc_dev_driver.cpp"
        "src/dataset_defs.h"        = "$out/moc_dataset_defs.cpp"
        "src/link/link.h"           = "$out/moc_link.cpp"
        "src/link/link_manager.h"   = "$out/moc_link_manager.cpp"
        "src/notifications.h"       = "$out/moc_notifications.cpp"
    }
    foreach ($h in $mocHeaders.Keys) {
        & $moc @inc -o $mocHeaders[$h] $h
        if ($LASTEXITCODE -ne 0) { throw "moc failed on $h ($LASTEXITCODE)" }
    }

    # ---- driver-level test -----------------------------------------------------
    $driverExe = "$out/test_fw_upgrade.exe"
    & $Clang -std=c++23 -g -w @inc `
        -o $driverExe `
        tools/fw_upgrade_test/test_fw_upgrade.cpp `
        src/id_binnary.cpp `
        src/device/dev_driver.cpp `
        "$out/moc_id_binnary.cpp" "$out/moc_dev_driver.cpp" "$out/moc_dataset_defs.cpp" `
        -L "$Qt/lib" -lQt6Core -lQt6Gui
    if ($LASTEXITCODE -ne 0) { throw "compile failed: test_fw_upgrade ($LASTEXITCODE)" }
    Write-Host "built $driverExe"

    # ---- link-level test -------------------------------------------------------
    # src/notifications.cpp is not compiled here on purpose: its quoted #include "core.h"
    # would resolve to the real src/core.h. The test defines the three methods instead.
    $linkExe = "$out/test_link_reconnect.exe"
    if (-not $SkipLink) {
        & $Clang -std=c++23 -g -w @inc `
            -o $linkExe `
            tools/fw_upgrade_test/test_link_reconnect.cpp `
            src/link/link.cpp `
            src/link/link_manager.cpp `
            "$out/moc_link.cpp" "$out/moc_link_manager.cpp" "$out/moc_notifications.cpp" `
            -L "$Qt/lib" -lQt6Core -lQt6Gui -lQt6Network -lQt6SerialPort
        if ($LASTEXITCODE -ne 0) { throw "compile failed: test_link_reconnect ($LASTEXITCODE)" }
        Write-Host "built $linkExe"
    }

    if ($Build) { Write-Host "-Build given, not running"; exit 0 }

    # ---- run -------------------------------------------------------------------
    $env:PATH = "$Qt/bin;$(Split-Path $Clang);$env:PATH"
    $failed = 0

    if (-not $Firmware) {
        throw "no firmware: pass -Firmware <file.ufw> (the driver scenarios flash a real image)"
    }
    if (-not (Test-Path $Firmware)) { throw "not found: $Firmware" }

    Write-Host ""
    & $driverExe $Firmware @Scenario
    if ($LASTEXITCODE -ne 0) { $failed++ }

    if (-not $SkipLink) {
        Write-Host ""
        & $linkExe
        if ($LASTEXITCODE -ne 0) { $failed++ }
    }

    Write-Host ""
    if ($failed -gt 0) { Write-Host "$failed binary(ies) reported failures"; exit 1 }
    Write-Host "all green"
}
finally { Pop-Location }
