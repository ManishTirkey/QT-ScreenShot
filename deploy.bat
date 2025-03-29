@echo off
echo Building release version...
qmake -config release
mingw32-make clean
mingw32-make

echo Copying dependencies...
cd release
windeployqt ScreenshotTool.exe

echo Creating installer...
cd ..
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer.iss

echo Done!
pause 