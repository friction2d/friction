#!/bin/bash
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

set -e -x

source /opt/rh/llvm-toolset-7.0/enable
clang -v

SDK=${SDK:-"/opt/friction"}
SRC=${SDK}/src
DIST=${DIST:-"/mnt"}
MKJOBS=${MKJOBS:-4}

NINJA_V=1.11.1
CMAKE_V=3.26.3
UNWIND_V=1.4.0
SKIA_V=386256b297e93bbe06ee82abdcc701357d9ccd99
SKIA_URL=https://github.com/friction2d/skia

NINJA_BIN=${SDK}/bin/ninja
CMAKE_BIN=${SDK}/bin/cmake

STATIC_CFLAGS="-fPIC"
DEFAULT_CFLAGS="-I${SDK}/include"
DEFAULT_LDFLAGS="-L${SDK}/lib"
COMMON_CONFIGURE="--prefix=${SDK}"
SHARED_CONFIGURE="${COMMON_CONFIGURE} --enable-shared --disable-static"
STATIC_CONFIGURE="${COMMON_CONFIGURE} --disable-shared --enable-static"
DEFAULT_CONFIGURE="${SHARED_CONFIGURE}"

export PATH="${SDK}/bin:${PATH}"
export PKG_CONFIG_PATH="${SDK}/lib/pkgconfig"
export LD_LIBRARY_PATH="${SDK}/lib:${LD_LIBRARY_PATH}"

if [ ! -d "${SDK}" ]; then
    mkdir -p "${SDK}/lib"
    mkdir -p "${SDK}/bin"
    (cd "${SDK}"; ln -sf lib lib64)
fi

if [ ! -d "${SRC}" ]; then
    mkdir -p "${SRC}"
fi

# ninja
if [ ! -f "${NINJA_BIN}" ]; then
    cd ${SRC}
    NINJA_SRC=ninja-${NINJA_V}
    rm -rf ${NINJA_SRC} || true
    tar xf ${DIST}/tools/${NINJA_SRC}.tar.gz
    cd ${NINJA_SRC}
    ./configure.py --bootstrap
    cp -a ninja ${NINJA_BIN}
fi # ninja

# cmake
if [ ! -f "${CMAKE_BIN}" ]; then
    cd ${SRC}
    CMAKE_SRC=cmake-${CMAKE_V}
    rm -rf ${CMAKE_SRC} || true
    tar xf ${DIST}/ffmpeg/${CMAKE_SRC}.tar.gz
    cd ${CMAKE_SRC}
    ./configure ${COMMON_CONFIGURE} --parallel=${MKJOBS} -- -DCMAKE_USE_OPENSSL=OFF
    make -j${MKJOBS}
    make install
fi # cmake

# skia
if [ ! -f "${SDK}/lib/libskia.a" ]; then
    cd ${SRC}
    rm -rf skia-${SKIA_V} || true
    git clone ${SKIA_URL} skia-${SKIA_V}
    cd skia-${SKIA_V}
    git checkout ${SKIA_V}
    git submodule update -i --recursive
    mkdir build
    cd build
    cmake -G Ninja \
    -DCMAKE_INSTALL_PREFIX=${SDK} \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang \
    -DSKIA_USE_SYSTEM_LIBS=OFF \
    -DSKIA_SYNC_EXTERNAL=ON \
    -DSKIA_STATIC=ON \
    -DSKIA_USE_EGL=ON \
    -DLINUX_DEPLOY=ON \
    ..
    cmake --build .
    cp -a libskia.a ${SDK}/lib/
fi # skia

# libunwind
if [ ! -f "${SDK}/lib/pkgconfig/libunwind.pc" ]; then
    cd ${SRC}
    UNWIND_SRC=libunwind-${UNWIND_V}
    rm -rf ${UNWIND_SRC} || true
    tar xf ${DIST}/linux/${UNWIND_SRC}.tar.gz
    cd ${UNWIND_SRC}
    CC=clang CXX=clang++ ./configure ${DEFAULT_CONFIGURE} --disable-minidebuginfo --disable-tests
    make -j${MKJOBS}
    make install
fi # libunwind

echo "SDK PART 1 DONE"
