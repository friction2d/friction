@echo off

REM ### BUILD FRICTION ON WINDOWS
REM # Copyright (c) Ole-André Rodlie and contributors
REM # GPL3

set OPT=%1
set REL=OFF
set BTYPE=Release
set BDIR=release
set CBUILD=

if "%OPT%" == "release" (
    set REL=ON
)
if "%OPT%" == "ci" (
    set CBUILD=CI
)
if "%OPT%" == "debug" (
    set BTYPE=Debug
    set BDIR=debug
)

set CWD=%cd%
set SDK_DIR=%CWD%\sdk
set SDK_VERSION=1.0.0
set SDK_REV=r4
set SDK_SUFFIX=windows-x64.7z

set PATH=%SDK_DIR%\bin;%PATH%

git submodule update --init --recursive

set BRANCH=
for /f %%I in ('git rev-parse --abbrev-ref HEAD 2^> NUL') do set BRANCH=%%I

set COMMIT=
for /f %%i in ('git rev-parse --short^=8 HEAD') do set COMMIT=%%i

if not exist "sdk\" ( 
    curl -OL "https://github.com/friction2d/friction-sdk/releases/download/v%SDK_VERSION%/friction-sdk-%SDK_VERSION%%SDK_REV%-%SDK_SUFFIX%"
    7z x friction-sdk-%SDK_VERSION%%SDK_REV%-%SDK_SUFFIX%
)

if exist "build\" (
    @RD /S /Q build
)
mkdir build

cd "%CWD%\build"
mkdir output

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%BTYPE% -DCMAKE_PREFIX_PATH=%SDK_DIR% -DCUSTOM_BUILD=%CBUILD% -DBUILD_ENGINE=OFF -DFRICTION_OFFICIAL_RELEASE=%REL% -DWIN_DEPLOY=ON -DGIT_COMMIT=%COMMIT% -DGIT_BRANCH=%BRANCH% ..
set /p VERSION=<version.txt
cmake --build . --config %BTYPE%

if "%REL%" == "OFF" (
    set VERSION="%VERSION%-%COMMIT%"
)

set BUILD_OUTPUT="%CWD%\build\output"
set OUTPUT_DIR="%BUILD_OUTPUT%\friction-%VERSION%"

mkdir "%OUTPUT_DIR%"
mkdir "%OUTPUT_DIR%\audio"
mkdir "%OUTPUT_DIR%\platforms"

copy "%CWD%\build\src\core\%BDIR%\frictioncore.dll" "%OUTPUT_DIR%\"
copy "%CWD%\build\src\ui\%BDIR%\frictionui.dll" "%OUTPUT_DIR%\"
copy "%CWD%\build\src\app\%BDIR%\friction.exe" "%OUTPUT_DIR%\"

copy "%SDK_DIR%\bin\skia.dll" "%OUTPUT_DIR%\"

copy "%SDK_DIR%\bin\Qt5Core.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\Qt5Gui.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\Qt5Multimedia.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\Qt5Network.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\Qt5OpenGL.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\Qt5Qml.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\Qt5Widgets.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\Qt5Xml.dll" "%OUTPUT_DIR%\"

copy "%SDK_DIR%\plugins\audio\qtaudio_wasapi.dll" "%OUTPUT_DIR%\audio\"
copy "%SDK_DIR%\plugins\platforms\qwindows.dll" "%OUTPUT_DIR%\platforms\"

copy "%SDK_DIR%\bin\qscintilla2_qt5.dll" "%OUTPUT_DIR%\"

copy "%SDK_DIR%\bin\avcodec-58.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\avdevice-58.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\avformat-58.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\avutil-56.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\swresample-3.dll" "%OUTPUT_DIR%\"
copy "%SDK_DIR%\bin\swscale-5.dll" "%OUTPUT_DIR%\"

echo "Delete this file if you want to disable portable mode" > "%OUTPUT_DIR%\portable.txt"

cd "%BUILD_OUTPUT%"

copy "%SDK_DIR%\bin\svgo-win.exe" "%BUILD_OUTPUT%\"
if not exist "svgo-license.txt" ( 
    curl -OL "https://raw.githubusercontent.com/friction2d/friction-svgo/main/svgo-license.txt"
)

7z a -mx9 friction-svgo-windows-x64.7z svgo-win.exe svgo-license.txt
7z a -mx9 friction-%VERSION%-windows-x64.7z friction-%VERSION%

copy "%CWD%\build\src\app\%BDIR%\friction.iss" "%OUTPUT_DIR%\"
copy "%CWD%\src\app\icons\friction.ico" "%OUTPUT_DIR%\"
copy "%CWD%\src\app\icons\friction.bmp" "%OUTPUT_DIR%\"
copy "%CWD%\LICENSE.md" "%OUTPUT_DIR%\"
copy "%BUILD_OUTPUT%\svgo-win.exe" "%OUTPUT_DIR%\"
copy "%BUILD_OUTPUT%\svgo-license.txt" "%OUTPUT_DIR%\"

cd "%OUTPUT_DIR%"

iscc friction.iss
copy "setup\friction.exe" "%BUILD_OUTPUT%\friction-%VERSION%-setup-win64.exe"
