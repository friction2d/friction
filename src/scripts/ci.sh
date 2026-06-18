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

# Friction Qt5/Qt6 CI (Ubuntu 24.04)

set -e -x

CI=${CI:-0}
APT=${APT:-0}
PC=${PC:-""}
QT6=${QT6:-"OFF"}

if [ "${QT6}" = "ON" ]; then
    QTV=6
else
    QTV=5
fi

if [ "${APT}" = 1 ]; then
sudo apt update -y
sudo apt install -y \
curl \
git \
clang \
build-essential \
cmake \
python3 \
ninja-build \
libfontconfig1-dev \
libfreetype-dev \
libavcodec-dev \
libavformat-dev \
libavutil-dev \
libswresample-dev \
libswscale-dev \
libunwind-dev \
libexpat1-dev \
libfreetype-dev \
libjpeg-turbo8-dev \
libpng-dev \
libwebp-dev \
zlib1g-dev \
libicu-dev \
libharfbuzz-dev

if [ "${QT6}" = "ON" ]; then
sudo apt install -y \
libqscintilla2-qt6-dev \
qt6-base-dev-tools \
qt6-base-dev \
qt6-declarative-dev-tools \
qt6-declarative-dev \
qt6-multimedia-dev \
qt6-tools-dev-tools \
qt6-tools-dev
else
sudo apt install -y \
libqscintilla2-qt5-dev \
libqt5opengl5-dev \
qtbase5-dev-tools \
qtbase5-dev \
qtdeclarative5-dev-tools \
qtdeclarative5-dev \
qtmultimedia5-dev \
qttools5-dev-tools
fi

fi

if [ "${CI}" = 1 ]; then
    git submodule update --init --recursive
fi

if [ "${PC}" != "" ]; then
    export PKG_CONFIG_PATH="${PC}:${PKG_CONFIG_PATH}"
fi

CWD=`pwd`
MKJOBS=${MKJOBS:-4}

cd ${CWD}
rm -rf build-ci || true
mkdir build-ci
cd build-ci

cmake -G Ninja \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_INSTALL_PREFIX=/usr \
-DCMAKE_CXX_COMPILER=clang++ \
-DCMAKE_C_COMPILER=clang \
-DQSCINTILLA_INCLUDE_DIRS=/usr/include/x86_64-linux-gnu/qt${QTV} \
-DQSCINTILLA_LIBRARIES=qscintilla2_qt${QTV} \
-DUSE_QT6=${QT6} \
..
cmake --build .
#cpack -G DEB
