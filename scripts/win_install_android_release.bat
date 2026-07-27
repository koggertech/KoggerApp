@echo off
setlocal enabledelayedexpansion

REM Install the signed Android release APK onto the connected tablet via adb.
REM Press after building the Android Release in Qt Creator.

REM ============================ EDIT THESE ============================
REM 1) Path to the signed release APK produced by your Android Release build.
REM    Relative to the repo root (or an absolute path). The kit folder name
REM    (Qt_6_8_3_for_Android_Universal-Release) depends on YOUR Qt version /
REM    kit name - fix it here if your build dir differs.
set "APK=build\Qt_6_8_3_for_Android_Universal-Release\android-build-KoggerApp\build\outputs\apk\release\android-build-KoggerApp-release-signed.apk"

REM 2) Android SDK root (must contain platform-tools\adb.exe). Leave the line
REM    as-is to reuse the ANDROID_SDK_ROOT env var; falls back to C:\Android\Sdk.
REM    adb on PATH is used first and overrides this.
set "ANDROID_SDK=%ANDROID_SDK_ROOT%"

REM 3) Application package id (from AndroidManifest / build).
set "PKG=org.kogger.koggerapp"

REM 4) Optional: force a specific device serial (see `adb devices`). Leave
REM    empty to auto-pick the only device, or choose from a menu when several
REM    are connected.
set "DEVICE="
REM ===================================================================

REM Go to repo root (so a relative APK path resolves)
cd /d "%~dp0\.."

if "%ANDROID_SDK%"=="" set "ANDROID_SDK=C:\Android\Sdk"

REM Locate adb: PATH first, then %ANDROID_SDK%\platform-tools
set "adb="
where adb >nul 2>nul && set "adb=adb"
if not defined adb if exist "%ANDROID_SDK%\platform-tools\adb.exe" set "adb=%ANDROID_SDK%\platform-tools\adb.exe"
if not defined adb (
    echo [ERROR] adb not found on PATH or under "%ANDROID_SDK%\platform-tools".
    echo         Fix ANDROID_SDK at the top of this script.
    pause
    exit /b 1
)

REM Signed APK must exist
if not exist "%APK%" (
    echo [ERROR] Signed APK not found:
    echo        %APK%
    echo        Fix APK at the top of this script, or build a signed Android Release first.
    pause
    exit /b 1
)

REM Resolve the target device (DEVICE override, else auto / menu)
"%adb%" start-server >nul 2>nul
set "serial="
if defined DEVICE (
    set "serial=%DEVICE%"
) else (
    call :pick_device
)
if not defined serial (
    echo [ERROR] No device selected.
    pause
    exit /b 1
)
echo Target device: !serial!

REM Install (replace, keep data; --no-incremental forces a full push so the
REM whole APK transfers before adb returns - avoids the flaky incremental
REM install that makes a stale build look "successfully" deployed)
echo Installing "%APK%" ...
"%adb%" -s !serial! install -r --no-incremental "%APK%"
if errorlevel 1 (
    echo.
    echo [ERROR] Install failed.
    echo If it reports USER_RESTRICTED: on the tablet enable Developer options ^>
    echo   "Install via USB" and "USB debugging ^(Security settings^)", then retry.
    echo Fallback: "%adb%" -s !serial! uninstall %PKG%   then run this script again.
    pause
    exit /b 1
)

REM Relaunch so the new build is shown immediately
"%adb%" -s !serial! shell am force-stop %PKG%
"%adb%" -s !serial! shell monkey -p %PKG% -c android.intent.category.LAUNCHER 1 >nul 2>nul

echo.
echo Done. New build installed and launched on %serial%.
pause
endlocal
goto :eof

REM ---------------------------------------------------------------------------
REM Fill !serial! from connected devices: 0 -> error, 1 -> auto, many -> menu.
:pick_device
set "count=0"
for /f "usebackq skip=1 tokens=1,2" %%A in (`"%adb%" devices`) do (
    if "%%B"=="device" (
        set /a count+=1
        set "dev!count!=%%A"
    )
)
if !count!==0 (
    echo [ERROR] No device connected ^(or unauthorized/offline^). Check USB and allow the prompt.
    "%adb%" devices
    goto :eof
)
if !count!==1 (
    set "serial=!dev1!"
    goto :eof
)
echo Multiple devices connected:
for /l %%I in (1,1,!count!) do echo   %%I^) !dev%%I!
set "pick="
set /p "pick=Select device number: "
if not defined pick goto :eof
set "serial=!dev%pick%!"
goto :eof
