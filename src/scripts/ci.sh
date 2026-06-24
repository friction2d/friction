#!/bin/bash
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Friction Ubuntu CI

set -e -x

CWD=`pwd`
MKJOBS=${MKJOBS:-4}
CI=${CI:-0}
APT=${APT:-0}
PC=${PC:-""}
QT6=${QT6:-"OFF"}
CPU=${CPU:-"x86_64"} # or aarch64
DEB=${DEB:-0}
FFMPEG=${FFMPEG:-0}
FFMPEG_STATIC=${FFMPEG_STATIC:-0}
FFMPEG_URL=https://github.com/friction2d/ffmpeg
FFMPEG_COMMIT=44b01dbdbe3a742b5d006a7187b63a8a30293a45

if [ "${QT6}" = "ON" ]; then
    QTV=6
else
    QTV=5
fi

if [ "${APT}" = 1 ]; then
    sudo apt update -y
    sudo apt install -y curl git clang build-essential cmake python3 ninja-build libfontconfig1-dev libfreetype-dev libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev libunwind-dev libexpat1-dev libfreetype-dev libjpeg-turbo8-dev libpng-dev libwebp-dev zlib1g-dev libicu-dev libharfbuzz-dev libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev
    if [ "${QT6}" = "ON" ]; then
        sudo apt install -y libqscintilla2-qt6-dev qt6-base-dev-tools qt6-base-dev qt6-declarative-dev-tools qt6-declarative-dev qt6-multimedia-dev qt6-tools-dev-tools qt6-tools-dev
    else
        sudo apt install -y libqscintilla2-qt5-dev libqt5opengl5-dev qtbase5-dev-tools qtbase5-dev qtdeclarative5-dev-tools qtdeclarative5-dev qtmultimedia5-dev qttools5-dev-tools
    fi
fi

if [ "${CI}" = 1 ]; then
    git submodule update --init --recursive
fi

if [ "${PC}" != "" ]; then
    export PKG_CONFIG_PATH="${PC}:${PKG_CONFIG_PATH}"
fi

cd ${CWD}
rm -rf build-ci || true
mkdir build-ci
cd build-ci

if [ "${FFMPEG}" = 1 ]; then
    if [ "${APT}" = 1 ]; then
        sudo apt install -y nasm yasm libaom-dev libmp3lame-dev libtheora-dev libvorbis-dev libvpx-dev libx264-dev libx265-dev libxvidcore-dev
    fi
    if [ "${FFMPEG_STATIC}" = 1 ]; then
        FFMPEG_LIBS="--enable-static --disable-shared --enable-pic --extra-cflags=-fPIC --extra-cxxflags=-fPIC --pkg-config-flags=--static"
    else
        FFMPEGS="--disable-static --enable-shared"
    fi
    FFMPEG_INSTALL=${CWD}/build-ci/ffmpeg-install
    git clone ${FFMPEG_URL} ffmpeg-src
    cd ffmpeg-src
    ./configure ${FFMPEG_LIBS} --prefix=${FFMPEG_INSTALL} --disable-xlib --disable-libxcb --disable-libv4l2 --disable-alsa --disable-network --disable-programs --disable-debug --disable-doc --enable-avresample --enable-gpl --enable-version3 --disable-avisynth --disable-gnutls --disable-libass --disable-libbluray --disable-libbs2b --disable-libcaca --enable-libmp3lame --disable-libopencore-amrnb --disable-libopencore-amrwb --disable-libopus --disable-libspeex --enable-libtheora --disable-libvidstab --disable-libvo-amrwbenc --enable-libvorbis --enable-libvpx --enable-libx264 --enable-libaom --enable-libx265 --enable-libxvid
    make -j${MKJOBS}
    make install
    export PKG_CONFIG_PATH=${FFMPEG_INSTALL}/lib/pkgconfig:${PKG_CONFIG_PATH}
    export LD_LIBRARY_PATH=${FFMPEG_INSTALL}/lib:${LD_LIBRARY_PATH}
fi

cd ${CWD}/build-ci

cmake -G Ninja \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_INSTALL_PREFIX=/usr \
-DCMAKE_CXX_COMPILER=clang++ \
-DCMAKE_C_COMPILER=clang \
-DQSCINTILLA_INCLUDE_DIRS=/usr/include/${CPU}-linux-gnu/qt${QTV} \
-DQSCINTILLA_LIBRARIES=qscintilla2_qt${QTV} \
-DUSE_QT6=${QT6} \
-DSKIA_STATIC=ON \
..
cmake --build .

if [ "${DEB}" = 1 ]; then
    cpack -G DEB
fi
