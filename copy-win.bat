@echo off
set "sourceFolder=.\src\C++"
set "destinationFolder=.\include"

if not exist "%destinationFolder%" mkdir "%destinationFolder%"

rem Copy all .h files from source and its subdirectories to the destination folder
xcopy /S /I "%sourceFolder%\*.h" "%destinationFolder%"

echo "Header files copied successfully!"