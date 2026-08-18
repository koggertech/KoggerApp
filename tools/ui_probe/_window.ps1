# Shared window handling for grab_window.ps1 / send_input.ps1. Dot-source it.
#
# THE RULE THIS FILE EXISTS TO ENFORCE: never read or click a window without first proving it
# is in the foreground. Windows lets SetForegroundWindow fail silently when the caller is not
# itself foreground -- and it does fail, intermittently, when the caller is an agent shell.
# A screenshot then photographs whatever is on top, and a click lands in someone else's app.
# Both happened; the second one scrolled an unrelated window. So activation is verified and a
# failure is fatal, never a shrug.

$ErrorActionPreference = "Stop"

if (-not ("KWin" -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public class KWin {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint f, int dx, int dy, int d, IntPtr e);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint f, IntPtr e);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    public const uint LEFTDOWN = 0x0002, LEFTUP = 0x0004;
    public const uint RIGHTDOWN = 0x0008, RIGHTUP = 0x0010, WHEEL = 0x0800;
    public const int RESTORE = 9;
}
"@
}

function Get-AppWindow([string] $ProcessName) {
    $p = Get-Process $ProcessName -ErrorAction SilentlyContinue |
         Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
    if (-not $p) { throw "no '$ProcessName' window found (is it running?)" }
    return $p
}

# Bring $h to the front and PROVE it got there. The ALT tap is what releases Windows'
# foreground lock for this call; without it SetForegroundWindow returns true and does nothing.
function Assert-Foreground([IntPtr] $h, [int] $TimeoutMs = 3000) {
    if ([KWin]::IsIconic($h)) { [KWin]::ShowWindow($h, [KWin]::RESTORE) | Out-Null }
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)
    while ((Get-Date) -lt $deadline) {
        if ([KWin]::GetForegroundWindow() -eq $h) { return }
        [KWin]::keybd_event(0x12, 0, 0,          [IntPtr]::Zero)   # ALT down
        [KWin]::keybd_event(0x12, 0, 2,          [IntPtr]::Zero)   # ALT up
        [KWin]::SetForegroundWindow($h) | Out-Null
        Start-Sleep -Milliseconds 200
    }
    throw ("could not bring the window to the foreground within ${TimeoutMs} ms -- " +
           "refusing to act on it, because the click or capture would hit whatever is on top")
}

function Get-Rect([IntPtr] $h) {
    $r = New-Object KWin+RECT
    [KWin]::GetWindowRect($h, [ref] $r) | Out-Null
    if (($r.R - $r.L) -le 0 -or ($r.B - $r.T) -le 0) { throw "window rect is empty" }
    return $r
}
