# Start (or stop) the virtual KOGGER device that KoggerApp talks to over loopback UDP.
#
#   powershell -File scripts/kpdev_serve.ps1            # start, 8 h cap, fresh trace
#   powershell -File scripts/kpdev_serve.ps1 -Stop      # stop whatever is serving
#   powershell -File scripts/kpdev_serve.ps1 -Hours 1 -Period 0    # judge only, no stimulus
#
# The app auto-connects to it because pinned_links.xml has a kAuto UDP link on
# 127.0.0.1:14650 -- see docs/KoggerApp-Docs/virtual-device-harness.md. No device, no USBL
# groups in the UI: their `visible` is bound to dev.isUSBL.
#
# WHY THIS SCRIPT EXISTS. A hand-typed `kpdevtool.py serve` ran unattended for three days and
# wrote a 1.4 GB trace, duplicated into a second 1.4 GB file because stdout had been
# redirected to a path that received the same JSONL. Both are flags, so both are defaults
# here: --duration caps the run and --quiet keeps the trace in exactly one file. The trace
# path is timestamped, so a new run can never append to or overwrite an old one.
#
# kpdev lives in another repo, and this one is public, so neither location is baked in here.
# Point at your checkout with -KpTools or $env:KPTOOLS_DIR, and at an interpreter with
# -Python or $env:KPDEV_PYTHON (default: whatever `python` resolves to on PATH).
param(
    [int]    $Port     = 14650,
    [string] $Scenario = "usbl",
    [double] $Hours    = 8,
    [double] $Period   = 1.0,        # solution emission interval, s (0 = judge only)
    [int]    $Seed     = 7,          # fixed: a run that cannot replay makes every flake a hunt
    [string] $KpTools  = "",
    [string] $Python   = "",
    [switch] $Stop
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

# Asked for, then the environment, then a clear refusal -- never a guessed path, which fails
# later and further from the cause.
if (-not $KpTools) { $KpTools = $env:KPTOOLS_DIR }
if (-not $Python)  { $Python  = $env:KPDEV_PYTHON }
if (-not $Python) {
    $found = Get-Command python -ErrorAction SilentlyContinue
    if ($found) { $Python = $found.Source }
}

function Get-KpdevProcesses {
    Get-CimInstance Win32_Process -Filter "Name = 'python.exe'" |
        Where-Object { $_.CommandLine -and $_.CommandLine -match "kpdevtool\.py\s+serve" }
}

$running = Get-KpdevProcesses

if ($Stop) {
    if (-not $running) { Write-Host "nothing serving"; exit 0 }
    foreach ($p in $running) {
        Write-Host "stopping PID $($p.ProcessId)"
        Stop-Process -Id $p.ProcessId -Force -Confirm:$false
    }
    exit 0
}

# One device per port, or the second one fails bind() and then silently never answers.
if ($running) {
    Write-Host "already serving:"
    foreach ($p in $running) { Write-Host "  PID $($p.ProcessId): $($p.CommandLine)" }
    Write-Host "stop it first: powershell -File scripts/kpdev_serve.ps1 -Stop"
    exit 1
}

if (-not $KpTools) {
    throw "no kptools location: pass -KpTools <dir> or set KPTOOLS_DIR"
}
if (-not $Python) {
    throw "no python: pass -Python <exe> or set KPDEV_PYTHON"
}
$tool     = Join-Path $KpTools "kpdevtool.py"
$contract = Join-Path $root "contract/kp-layouts.json"
foreach ($f in @($tool, $contract, $Python)) {
    if (-not (Test-Path $f)) { throw "not found: $f" }
}

$outDir = Join-Path $root "build/kpdev"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$trace = Join-Path $outDir "trace-$stamp.jsonl"
$log   = Join-Path $outDir "serve-$stamp.log"

$argList = @(
    $tool, "serve",
    "--scenario", $Scenario,
    "--port", $Port,
    "--contract", $contract,
    "--trace", $trace,
    "--duration", [int]($Hours * 3600),
    "--period", $Period,
    "--seed", $Seed,
    "--quiet"
)

$proc = Start-Process -FilePath $Python -ArgumentList $argList -WorkingDirectory $KpTools `
                      -WindowStyle Hidden -PassThru `
                      -RedirectStandardOutput $log -RedirectStandardError "$log.err"

Start-Sleep -Milliseconds 800
if ($proc.HasExited) {
    Write-Host "kpdev exited immediately (code $($proc.ExitCode)) -- see $log.err"
    Get-Content "$log.err" -Tail 20
    exit 1
}

Write-Host "kpdev serving scenario '$Scenario' on UDP $Port"
Write-Host "  PID:      $($proc.Id)   (stops itself after $Hours h)"
Write-Host "  trace:    $trace"
Write-Host "  log:      $log"
Write-Host ""
Write-Host "verdict:  python `"$tool`" verdict --trace `"$trace`" -q verdict"
Write-Host "stop:     powershell -File scripts/kpdev_serve.ps1 -Stop"
