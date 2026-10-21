@echo off
REM Update .ts translation sources from tr()/qsTr() found in src/ and qml/
cd /d "%~dp0\.."
python scripts\tutorial_tool.py gen || (pause & exit /b 1)
lupdate src qml -ts translations\translation_en.ts translations\translation_ru.ts translations\translation_pl.ts -no-obsolete
pause
