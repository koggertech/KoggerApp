# Start (or stop) the virtual KOGGER device that KoggerApp talks to over loopback UDP.
#
#   powershell -File scripts/kpdev_serve.ps1            # start, 8 h cap, fresh trace
#   powershell -File scripts/kpdev_serve.ps1 -Stop      # stop whatever is serving
#   powershell -File scripts/kpdev_serve.ps1 -Hours 1 -Period 0    # judge only, no stimulus
#   powershell -File scripts/kpdev_serve.ps1 -Keep 20   # keep more past runs (0 = keep all)
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
#
# CALLING THIS FROM A SCRIPT OR AN AGENT: do not wait on it. It starts kpdev and returns, but
# the served process inherits the stdio handles this script redirects, so a caller that waits
# for the pipeline to close waits for kpdev's whole run. Observed twice; the redirect stays
# because it is the only way a startup crash gets reported. Launch it detached instead:
#
#   Start-Process powershell -ArgumentList '-File','scripts/kpdev_serve.ps1','-KpTools','<dir>'
#
# then confirm with the process check the -Stop path uses. Interactive use is unaffected.
param(
    [int]    $Port       = 14650,
    [string] $Scenario   = "usbl",
    [double] $Hours      = 8,
    # UNSOLICITED emission, off by default. kpdev now ANSWERS interrogations (2R/c plus the
    # node's turn-around), which is what a head does and the only mode in which the app's own
    # answer window means anything. Free-running emission on top of that is actively
    # misleading: solutions keep arriving after the app stops asking, which looks like a host
    # bug and is not one. Pass -Period 1 only if a test needs a stream nobody requested.
    [double] $Period     = 0,
    # Named link condition. 'clean' is what the water does and nothing else. 'flaky' is the
    # bad-link test: 300-800 ms of latency drawn per interrogation, 30% of them never answered.
    # Any of the three overrides below wins over the profile.
    [ValidateSet("clean", "flaky")]
    [string] $Profile       = "clean",
    [Nullable[double]] $ReplyDelay    = $null,  # 0 = derive from range; above the app's dwell
                                                # makes every interrogation time out
    [Nullable[double]] $ReplyDelayMax = $null,  # above -ReplyDelay: drawn per interrogation
    [Nullable[double]] $DropProb      = $null,  # fraction left unanswered
    [int]    $Seed       = 7,        # fixed: a run that cannot replay makes every flake a hunt
    # Past runs to keep in build/kpdev. 0 keeps everything, for when you are deliberately
    # collecting a series and would rather manage it yourself.
    [int]    $Keep       = 5,
    [string] $KpTools    = "",
    [string] $Python     = "",
    [switch] $NoReply,               # pre-2026-08 behaviour: nothing answers, --period only
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

# ── retention ────────────────────────────────────────────────────────────────
# --duration caps how big ONE run gets. Nothing capped how many runs accumulate, which is a
# different problem with the same ending: this directory reached 1.02 GB across 121 files, and
# the only reason it was noticed is that somebody asked. Three files per run, and every run
# ever started was still here.
#
# KEYED ON THE STAMP IN THE NAME, not on mtime. The name is when a run STARTED; mtime is when
# it was last appended to, so an eight-hour run started this morning looks newer than a short
# one started since, and "keep the newest" would keep the wrong ones. The stamp sorts as a
# string because it is yyyyMMdd-HHmmss.
#
# IT ONLY TOUCHES FILES IT MADE -- trace-<stamp>.jsonl and serve-<stamp>.log[.err]. A
# hand-named capture is not this script's to delete: when this directory was last cleared by
# hand the one file worth keeping turned out to be exactly that, a written-up soak result that
# nothing else records. Anything not matching the pattern is left alone, whatever its size.
#
# Logs whose trace is already gone are swept with the rest, or they outlive every run they
# describe and the count drifts away from what is actually on disk.
if ($Keep -gt 0) {
    $stamps = Get-ChildItem $outDir -File -Filter "trace-*.jsonl" -ErrorAction SilentlyContinue |
        ForEach-Object { if ($_.BaseName -match '^trace-(\d{8}-\d{6})$') { $Matches[1] } } |
        Sort-Object -Descending
    $keepSet = @($stamps | Select-Object -First $Keep)
    $doomed = @(Get-ChildItem $outDir -File -ErrorAction SilentlyContinue | Where-Object {
        ($_.Name -match '^(?:trace|serve)-(\d{8}-\d{6})') -and ($keepSet -notcontains $Matches[1])
    })
    if ($doomed.Count) {
        $freed = [math]::Round((($doomed | Measure-Object Length -Sum).Sum / 1MB), 1)
        Write-Host "retention: dropping $($doomed.Count) file(s) from older runs, $freed MB (keeping the newest $Keep)"
        $doomed | Remove-Item -Force -Confirm:$false -ErrorAction SilentlyContinue
    }
}

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
    "--profile", $Profile,
    "--seed", $Seed,
    "--quiet"
)
# Only pass the overrides that were actually given: kpdev distinguishes "not given" from
# "given as 0" so that -DropProb 0 can switch a profile's loss off.
if ($null -ne $ReplyDelay)    { $argList += @("--reply-delay",     $ReplyDelay) }
if ($null -ne $ReplyDelayMax) { $argList += @("--reply-delay-max", $ReplyDelayMax) }
if ($null -ne $DropProb)      { $argList += @("--drop-prob",       $DropProb) }
if ($NoReply)                 { $argList += "--no-reply" }

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
if ($NoReply) {
    Write-Host "  replies: OFF -- nothing answers interrogations"
} else {
    Write-Host "  replies: ON, profile '$Profile'$(if ($Profile -eq 'flaky') { ' -- 300-800 ms latency, 30% unanswered' } else { ' -- 2R/c + turn-around, none lost' })"
    foreach ($o in @(@("reply-delay", $ReplyDelay), @("reply-delay-max", $ReplyDelayMax), @("drop-prob", $DropProb))) {
        if ($null -ne $o[1]) { Write-Host "           override: $($o[0]) = $($o[1])" }
    }
}
if ($Period -gt 0) { Write-Host "  plus UNSOLICITED emission every $Period s" }
Write-Host "  PID:      $($proc.Id)   (stops itself after $Hours h)"
Write-Host "  trace:    $trace"
Write-Host "  log:      $log"
Write-Host ""
Write-Host "verdict:  python `"$tool`" verdict --trace `"$trace`" -q verdict"
Write-Host "stop:     powershell -File scripts/kpdev_serve.ps1 -Stop"
