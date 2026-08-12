# Build and run the firmware-upgrade regression tests.
#
#   run.ps1 -Firmware "C:\path\RecorderFiles.ufw"          # everything
#   run.ps1 -Firmware ... -Scenario happy-path             # one driver scenario
#   run.ps1 -Firmware ... -SkipLink                        # driver scenarios only
#   run.ps1 -Build                                         # compile, do not run
#
# Two binaries, deliberately:
#   test_fw_upgrade    - DevDriver + IDBinUpdate against a scriptable Recorder. Qt Core only.
#   test_link_reconnect- Link + LinkManager across a USB-VCP port dropout. Adds Network and
#                        SerialPort, and needs LinkManager's getCurrentSerialList seam.
#
# Neither one opens a real port or needs a device. A non-zero exit means a check failed.
#
# The compiler and Qt default to the same llvm-mingw kit tools/protocol_contract/gen.ps1
# uses. Override if your kit lives elsewhere; a wrong path fails here rather than half way
# through a link step.
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

    $inc = @(
        "-I", "tools/fw_upgrade_test/shim",   # must precede src/ so <core.h> resolves to the stub
        "-I", "src",
        "-I", "tools/fw_upgrade_test",
        "-I", "$Qt/include",
        "-I", "$Qt/include/QtCore",
        "-I", "$Qt/include/QtGui",        # dataset_defs.h -> math_defs.h -> QVector3D
        "-I", "$Qt/include/QtNetwork",
        "-I", "$Qt/include/QtSerialPort"
    )

    # ---- moc -------------------------------------------------------------------
    # Same set CMake's AUTOMOC would produce for these translation units.
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
        # 2 = the scenario could not run on this Qt build; that is not a pass and not a
        # product failure, so it is reported separately instead of folded into the count.
        if ($LASTEXITCODE -eq 2) { Write-Host "link scenario INCONCLUSIVE" }
        elseif ($LASTEXITCODE -ne 0) { $failed++ }
    }

    Write-Host ""
    if ($failed -gt 0) { Write-Host "$failed binary(ies) reported failures"; exit 1 }
    Write-Host "all green"
}
finally { Pop-Location }
