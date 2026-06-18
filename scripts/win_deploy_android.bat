@echo off
setlocal enabledelayedexpansion

REM Copy the signed release APK to out_android\KoggerApp-<versionName>-universal.apk
REM Refuses to copy if the APK is missing or its signature does not verify.

REM 1. Go to repo root
cd /d "%~dp0\.."

set "apk=build\Qt_6_8_3_for_Android_Universal-Release\android-build-KoggerApp\build\outputs\apk\release\android-build-KoggerApp-release-signed.apk"
set "outDir=out_android"

set "sdk=%ANDROID_SDK_ROOT%"
if not defined sdk set "sdk=C:\Android\Sdk"

REM 2. Signed APK must exist
if not exist "%apk%" (
    echo [ERROR] Signed APK not found:
    echo        %apk%
    echo        Build a signed release APK first.
    exit /b 1
)

REM 3. Read version from resources\version.txt (single source)
set "version="
set /p version=<resources\version.txt
if not defined version (
    echo [ERROR] Could not read version from resources\version.txt
    exit /b 1
)

REM 4. Verify the signature (apksigner from the newest build-tools)
set "apksigner="
for /f "delims=" %%D in ('dir /b /ad /o-n "%sdk%\build-tools" 2^>nul') do (
    if not defined apksigner if exist "%sdk%\build-tools\%%D\apksigner.bat" set "apksigner=%sdk%\build-tools\%%D\apksigner.bat"
)
if not defined apksigner (
    echo [WARN] apksigner not found under "%sdk%\build-tools"; skipping signature check.
) else (
    call "%apksigner%" verify "%apk%" >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] APK is not signed or signature is invalid. Nothing copied.
        exit /b 1
    )
    echo Signature OK.
)

REM 5. Copy to out_android with the versioned name
if not exist "%outDir%" mkdir "%outDir%"
set "dest=%outDir%\KoggerApp-%version%-universal.apk"
copy /y "%apk%" "%dest%" >nul
if errorlevel 1 (
    echo [ERROR] Copy failed.
    exit /b 1
)
echo Done: %dest%

endlocal
