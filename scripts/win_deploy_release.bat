@echo off

REM Windows deploy script using windeployqt
REM For correct operation, make sure that components are available in the environment variables


REM add to path (default loc):
REM C:\Qt\Tools\mingw810_64\bin
REM C:\Qt\5.15.2\mingw81_64\bin
REM C:\Program Files\7-Zip
REM ############################################

REM 1. Go to root
cd /d "%~dp0\.."

REM 2. Ask if user wants to archive
set /p archive=Do you want to archive the output directory? (y/n): 

REM 3. Set local paths
set binPath=build\MinGW_8_1_0_x64_desktop-Release\release\KoggerApp.exe
set qmlPath=qml
set outPath=out_x64
set zip_file=out_x64\KoggerApp_win_x64.zip

echo executable file path: "%binPath%"
echo QML project files path: "%qmlPath%"
echo output path: "%outPath%"

REM 4. remove old ouput directory if exist
if exist "%outPath%" (
    rmdir /s /q "%outPath%"
    echo the output directory will be overwritten
)

REM 5. Deploy and copy binary file
windeployqt %binPath% -qmldir %qmlPath% -dir %outPath% -no-translations -no-virtualkeyboard
xcopy "%binPath%" "%outPath%"

if /i "%archive%"=="y" (
    REM 6. Archiving to .zip 
    pushd "%outPath%"
    7z a "..\%zip_file%" *
    popd
    echo Archiving completed.
) else (
    echo Skipping archiving.
)

pause