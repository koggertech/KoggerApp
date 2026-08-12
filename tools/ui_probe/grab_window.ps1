# Screenshot a running window from OUTSIDE the process, to a PNG.
#
#   powershell -File tools/ui_probe/grab_window.ps1 -Out build/uiprobe/shot.png
#
# WHY NOT THE PROBE'S PNG. UiProbe's KOGGER_UI_PROBE_PNG uses QQuickWindow::grabWindow(),
# which has to round-trip the scene graph and is not reliably safe against this app's custom
# GL scene. This reads the desktop instead, so
# the app is never asked to do anything, and what lands in the file is the composited result
# including the GL viewport.
#
# It answers a different question from the geometry dump and does not replace it: a picture
# cannot tell a zero-height item from a clipped one. Use assert_layout.py for that, and this
# to see what an operator sees.
#
# Reading the desktop means the window must actually BE on top -- see _window.ps1. If it
# cannot be raised this fails instead of photographing the wrong application.
param(
    [string] $Process = "KoggerApp",
    [string] $Out     = "build/uiprobe/shot.png",
    [switch] $FullScreen,          # whole desktop, for when the window is partly offscreen
    [int]    $SettleMs = 400
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "_window.ps1")

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not [System.IO.Path]::IsPathRooted($Out)) { $Out = Join-Path $root $Out }
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null

Add-Type -AssemblyName System.Drawing, System.Windows.Forms

$p = Get-AppWindow $Process
Assert-Foreground $p.MainWindowHandle
Start-Sleep -Milliseconds $SettleMs
# Re-check after settling: a popup or another app can steal focus while we wait.
Assert-Foreground $p.MainWindowHandle

if ($FullScreen) {
    $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $x = $b.X; $y = $b.Y; $w = $b.Width; $ht = $b.Height
} else {
    $r = Get-Rect $p.MainWindowHandle
    $x = $r.L; $y = $r.T; $w = $r.R - $r.L; $ht = $r.B - $r.T
}

$bmp = New-Object System.Drawing.Bitmap $w, $ht
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($x, $y, 0, 0, (New-Object System.Drawing.Size $w, $ht))
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()

Write-Host "$Out  ($w x $ht, window '$($p.MainWindowTitle.Trim())' at $x,$y)"
