@echo off

REM add to path (default loc):
REM C:\Qt\5.15.2\mingw81_64\bin
REM C:\Program Files\7-Zip

REM ask if user wants to archive
set /p archive=Do you want to archive the output directory? (y/n): 

REM local paths:
set binPath=build\MinGW_8_1_0_x64_desktop-Release\release\KoggerApp.exe
set qmlPath=QML
set outPath=out_x64
set zip_file=out_x64\KoggerApp_win_x64.zip
echo executable file path: "%binPath%"
echo QML project files path: "%qmlPath%"
echo output path: "%outPath%"
if exist "%outPath%" (
    rmdir /s /q "%outPath%"
    echo the output directory will be overwritten
)

REM deploy and copy binary file:
windeployqt %binPath% -qmldir %qmlPath% -dir %outPath% -no-translations -no-virtualkeyboard
xcopy "%binPath%" "%outPath%"

if /i "%archive%"=="y" (
    REM .zip archiving
    pushd "%outPath%"
    7z a "..\%zip_file%" *
    popd
    echo Archiving completed.
) else (
    echo Skipping archiving.
)

pause