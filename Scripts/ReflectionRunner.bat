@echo off

cd /d "%~dp0"

cd ..

set PREMAKE="%CD%\Tools\premake5.exe"

if not exist %PREMAKE% (
    echo Premake5 not found at %PREMAKE%
    pause
    exit /b 1
)

%PREMAKE% Reflection

if %errorlevel% neq 0 (
    echo Premake generation failed!
    pause
    exit /b %errorlevel%
)

echo Premake solution generated successfully!
pause
