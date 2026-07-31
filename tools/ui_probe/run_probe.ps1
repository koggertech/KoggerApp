# Launch KoggerApp with the UI probe armed, then check the capture.
#
# Run this yourself from a normal terminal. The app segfaults at startup when launched
# detached/backgrounded (reproducible WITHOUT the probe, so it is not the probe's fault);
# a foreground launch is reliable.
#
#   powershell -File tools/ui_probe/run_probe.ps1
#   powershell -File tools/ui_probe/run_probe.ps1 -WaitFor AppSettingsPage
#
# Open the pane you want measured; the capture takes itself 2.5 s after that page
# appears, then this script runs the invariant check on it.

param(
    [string]$WaitFor  = "DeviceSettingsPage",
    [string]$Exe      = "build/usbl-backend/KoggerApp.exe",
    [string]$OutDir   = "build/uiprobe",
    [string]$Qt       = "C:/Qt/6.8.3/llvm-mingw_64",
    [int]   $GraceMs  = 6000,
    [int]   $DelayMs  = 2500,
    [int]   $TimeoutMs = 600000,
    [switch]$Png,
    [switch]$NoCheck
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Push-Location $root
try {
    if (-not (Test-Path $Exe)) { throw "no such exe: $Exe (build it first)" }

    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
    Get-ChildItem -Path $OutDir -Filter "ui-*.json" -ErrorAction SilentlyContinue |
        Remove-Item -Force

    $env:PATH                       = "$Qt/bin;$env:PATH"
    $env:KOGGER_UI_PROBE            = (Resolve-Path $OutDir).Path
    $env:KOGGER_UI_PROBE_WAIT_FOR   = $WaitFor
    $env:KOGGER_UI_PROBE_GRACE_MS   = "$GraceMs"
    $env:KOGGER_UI_PROBE_DELAY_MS   = "$DelayMs"
    $env:KOGGER_UI_PROBE_TIMEOUT_MS = "$TimeoutMs"
    # grabWindow() is unsafe against this app's custom GL scene, so the PNG stays opt-in.
    if ($Png) { $env:KOGGER_UI_PROBE_PNG = "1" } else { Remove-Item Env:KOGGER_UI_PROBE_PNG -ErrorAction SilentlyContinue }

    Write-Host "probe armed: waiting for '$WaitFor' -> $($env:KOGGER_UI_PROBE)"
    Write-Host "open that pane in the app; capture happens $DelayMs ms after it appears."
    Write-Host ""

    & ".\$($Exe -replace '/','\')"
    Write-Host ""
    Write-Host "app exited ($LASTEXITCODE)"

    $dump = Get-ChildItem -Path $OutDir -Filter "ui-*.json" -ErrorAction SilentlyContinue |
            Select-Object -First 1
    if (-not $dump) {
        Write-Host "no capture written - was the pane opened before exit?"
        return
    }
    Write-Host "capture: $($dump.FullName) ($([int]($dump.Length/1KB)) KB)"
    if (-not $NoCheck) {
        Write-Host ""
        python tools/ui_probe/assert_layout.py $dump.FullName
    }
}
finally { Pop-Location }
