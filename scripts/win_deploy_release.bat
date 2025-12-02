@echo off

REM Windows deploy script using windeployqt
REM For correct operation, make sure that components are available in the environment variables


REM add to path (default loc):
REM C:\Qt\Tools\llvm-mingw1706_64\bin
REM C:\Qt\6.8.3\llvm-mingw_64\bin
REM C:\Program Files\7-Zip
REM ############################################

REM 1. Go to root
cd /d "%~dp0\.."

REM 2. Ask if user wants to archive
set /p archive=Do you want to archive the output directory? (y/n): 

REM 3. Set local paths
set llvm_mingw=C:\Qt\Tools\llvm-mingw1706_64
set binPath=build\Desktop_Qt_6_8_3_llvm_mingw_64_bit-Release\release\KoggerApp.exe
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

REM 6. copy LLVM-MINGW RUNTIME manually
echo LLVM_MINGW=%llvm_mingw%
for %%F in (libunwind.dll libc++.dll libwinpthread-1.dll) do (
    if exist "%llvm_mingw%\bin\%%F" (
        echo copying %%F
        copy "%llvm_mingw%\bin\%%F" "%outPath%"
    ) else (
        echo NOT FOUND: "%llvm_mingw%\bin\%%F"
    )
)

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