# Click / scroll a running window from outside the process, in WINDOW coordinates.
#
#   powershell -File tools/ui_probe/send_input.ps1 -X 120 -Y 360            # left click
#   powershell -File tools/ui_probe/send_input.ps1 -X 400 -Y 500 -Scroll -6 # 6 notches down
#
# Pairs with grab_window.ps1: screenshot, read coordinates off the picture, click, screenshot
# again. That closes the loop an agent otherwise cannot -- the UI probe reports geometry but
# cannot press anything, and KOGGER_UI_PROBE_PNG is unsafe against this app's GL scene.
#
# Coordinates are relative to the window's top-left, which is what grab_window.ps1's PNG
# shows, so positions read off that image can be used directly.
#
# The target window is raised and VERIFIED first (_window.ps1). Synthetic input goes wherever
# the cursor is, so an unverified click lands in whatever happens to be on top -- that is not
# a theoretical risk, it scrolled an unrelated application before this check existed.
param(
    [string] $Process = "KoggerApp",
    [int]    $X = -1,
    [int]    $Y = -1,
    [int]    $Scroll = 0,          # +up / -down, in notches
    [switch] $Right,
    [int]    $SettleMs = 350
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "_window.ps1")

if ($X -lt 0 -or $Y -lt 0) { throw "-X and -Y are required (window coordinates)" }

$p = Get-AppWindow $Process
Assert-Foreground $p.MainWindowHandle
$r = Get-Rect $p.MainWindowHandle
$sx = $r.L + $X
$sy = $r.T + $Y
if ($X -gt ($r.R - $r.L) -or $Y -gt ($r.B - $r.T)) {
    throw "window($X,$Y) is outside the window ($($r.R - $r.L) x $($r.B - $r.T))"
}

[KWin]::SetCursorPos($sx, $sy) | Out-Null
Start-Sleep -Milliseconds 120
Assert-Foreground $p.MainWindowHandle      # last gate before input is synthesised

if ($Scroll -ne 0) {
    # One notch is WHEEL_DELTA (120), sent one at a time: some handlers treat a single large
    # delta as one event and the view then moves less than asked.
    $step = 120 * [Math]::Sign($Scroll)
    for ($i = 0; $i -lt [Math]::Abs($Scroll); $i++) {
        [KWin]::mouse_event([KWin]::WHEEL, 0, 0, $step, [IntPtr]::Zero)
        Start-Sleep -Milliseconds 60
    }
    Write-Host "scrolled $Scroll notches at window($X,$Y) in '$($p.MainWindowTitle.Trim())'"
} else {
    if ($Right) { $down = [KWin]::RIGHTDOWN; $up = [KWin]::RIGHTUP }
    else        { $down = [KWin]::LEFTDOWN;  $up = [KWin]::LEFTUP }
    [KWin]::mouse_event($down, 0, 0, 0, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 40
    [KWin]::mouse_event($up, 0, 0, 0, [IntPtr]::Zero)
    Write-Host "clicked window($X,$Y) in '$($p.MainWindowTitle.Trim())'"
}
Start-Sleep -Milliseconds $SettleMs
