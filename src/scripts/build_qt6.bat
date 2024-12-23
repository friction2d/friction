@echo off

REM ### BUILD QT 5.15 FOR FRICTION
REM # Copyright (c) Ole-André Rodlie and contributors
REM # GPLv3+

set CWD=%cd%
set SRC_DIR=%CWD%\src
set QT_DIR=%SRC_DIR%\qt
set QT_V=6.8.2

set PATH=C:\Python;%ProgramFiles%\CMake\bin;%PATH%

cd "%SRC_DIR%\qt-everywhere-src-%QT_V%"
nmake
nmake
nmake install
