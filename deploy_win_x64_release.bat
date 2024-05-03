@echo off
REM locat paths:
set binPath=build\MinGW_8_1_0_x64_desktop-Release\release\KoggerApp.exe
set qmlPath=QML
set outPath=build\MinGW_8_1_0_x64_desktop-Release\out_x64
echo executable file path: "%binPath%"
echo QML project files path: "%qmlPath%"
echo output path: "%outPath%"
if exist "%outPath%" (
    rmdir /s /q "%outPath%"
    echo the output directory will be overwritten
)
REM deploy and cope binary file
windeployqt %binPath% -qmldir %qmlPath% -dir %outPath% -no-translations -no-virtualkeyboard
xcopy "%binPath%" "%outPath%"
pause