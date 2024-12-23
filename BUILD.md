# Build Friction

Generic instructions for building Friction on supported systems.

## Requirements on Linux

* pkg-config
* ninja
* python3
* cmake *(3.12+)*
* clang *(7+)*
* Qt *(6.8.x)*
    * Gui
    * Widgets
    * OpenGL
    * Multimedia
    * Qml
    * Xml
* qscintilla-qt6
* ffmpeg *(4.2.x)*
    * libavformat
    * libavcodec
    * libavutil
    * libswscale
    * libswresample
* libunwind
* expat
* harfbuzz
* freetype
* fontconfig
* libjpeg-turbo
* libpng
* libwebp
* zlib
* icu

## Requirements on Windows

* [Visual Studio 2017 Build Tools](https://aka.ms/vs/15/release/vs_buildtools.exe)
* `git` must be available in `PATH`
* `cmake` must be available in `PATH`
* `python3` must be available in `PATH`
* [LLVM](https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/LLVM-15.0.7-win64.exe) installed to default location
* Qt 6.8 libraries and headers in `friction\sdk`
  * See [configure_qt6.bat](src/scripts/configure_qt6.bat) and [build_qt6.bat](src/scripts/build_qt6.bat)
* QScintilla libraries and headers in `friction\sdk`
  * See [build_qscintilla.bat](src/scripts/build_qscintilla.bat)
* FFmpeg 4.2 libraries and headers in `friction\sdk`
  * See [build_mxe_ffmpeg.sh](src/scripts/build_mxe_ffmpeg.sh) and [mxe](https://github.com/friction2d/mxe)

Binary SDK available [here](https://github.com/friction2d/friction-sdk/releases).

## Requirements on macOS

* macOS Monterey 12.7 x86-64
* Xcode 14.2 *(Apple clang 14)*

Install required packages from macports:

```
sudo port install qt6-qtbase qt6-qttools qt6-qtmultimedia qt6-qtdeclarative qscintilla-qt6 ffmpeg pkgconfig ninja
```
### Notes

* FFmpeg in macports is not supported *(render issues)*, a custom Portfile or build will be made available.
* `python` and `python3` must be available in `PATH` and point to a Python 3 install.

## Get Source

```
git clone --recurse-submodules https://github.com/friction2d/friction
```

or

```
git clone --recurse-submodules https://codeberg.org/friction/friction
```

## Build on Linux

```
mkdir build
cd build
cmake -G Ninja \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_INSTALL_PREFIX=/usr \
-DCMAKE_CXX_COMPILER=clang++ \
-DCMAKE_C_COMPILER=clang \
..
```
Note that on some configurations you may need to specify `qscintilla` paths:
```
-DQSCINTILLA_INCLUDE_DIRS=<PATH_TO_QSCINTILLA_INCLUDE_DIR> \
-DQSCINTILLA_LIBRARIES_DIRS=<PATH_TO_LIBS> \
-DQSCINTILLA_LIBRARIES=<QSCINTILLA_LIBRARY_NAME> \
```

Now build:

```
cmake --build . --config Release
```

## Build on Windows

All requirements must be installed in the correct folders, this is an example and should be adjusted to fit your environment.

```
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\Common7\Tools\VsDevCmd.bat"

set SDK_DIR=%cd%\sdk
set PATH=C:\Python;%ProgramFiles%\CMake\bin;%ProgramFiles%\Git\bin;%SDK_DIR%\bin;%PATH%

mkdir build
cd build

cmake -G "Visual Studio 15 2017" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%SDK_DIR% ..

cmake --build . --config Release
```
