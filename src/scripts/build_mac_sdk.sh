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

# keep in sync with other SDK's

NINJA_V=1.11.1
CMAKE_V=3.26.3
NASM_V=2.14.02
YASM_V=1.3.0
PKGCONF_V=1.1.0

QT_V=6.8.1
QSCINTILLA_V=2.14.1

LAME_V=3.100
VPX_V=1.13.0
OGG_V=1.3.5
VORBIS_V=1.3.7
THEORA_V=1.1.1
XVID_V=1.3.4
LSMASH_V=2.14.5
X264_V=20180806-2245
X265_V=3.5
AOM_V=3.6.1
FFMPEG_V=4.2.10

CWD=`pwd`
SDK=${SDK:-"${CWD}/sdk"}
SRC=${SDK}/src
DIST=${DIST:-"${CWD}/distfiles"}
MKJOBS=${MKJOBS:-2}
SRC_SUFFIX=tar.xz

QMAKE_BIN=${SDK}/bin/qmake
PYTHON_BIN=${SDK}/bin/python
NINJA_BIN=${SDK}/bin/ninja
CMAKE_BIN=${SDK}/bin/cmake

export PATH="${SDK}/bin:/usr/bin:/bin:/usr/sbin:/sbin"
export PKG_CONFIG_PATH="${SDK}/lib/pkgconfig"

STATIC_CFLAGS="-fPIC"
DEFAULT_CFLAGS="-I${SDK}/include"
DEFAULT_LDFLAGS="-L${SDK}/lib"
COMMON_CONFIGURE="--prefix=${SDK}"
SHARED_CONFIGURE="${COMMON_CONFIGURE} --enable-shared --disable-static"
STATIC_CONFIGURE="${COMMON_CONFIGURE} --disable-shared --enable-static"
DEFAULT_CONFIGURE="${SHARED_CONFIGURE}"

if [ ! -d "${SDK}" ]; then
    mkdir -p "${SDK}/lib"
    mkdir -p "${SDK}/bin"
    mkdir -p "${SDK}/src"
    (cd "${SDK}"; ln -sf lib lib64)
    (cd "${SDK}/bin"; ln -sf /usr/bin/python3 python)
fi

alias python="/usr/bin/python3"

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

# pkgconfig
if [ ! -f "${SDK}/bin/pkg-config" ]; then
    cd ${SRC}
    PKGCONF_SRC=pkgconf-${PKGCONF_V}
    rm -rf ${PKGCONF_SRC} || true
    tar xf ${DIST}/tools/${PKGCONF_SRC}.tar.xz
    cd ${PKGCONF_SRC}
    ./configure ${COMMON_CONFIGURE}
    make -j${MKJOBS}
    make install
    (cd ${SDK}/bin ; ln -sf pkgconf pkg-config)
fi # pkgconfig

# nasm
if [ ! -f "${SDK}/bin/nasm" ]; then
    cd ${SRC}
    NASM_SRC=nasm-${NASM_V}
    rm -rf ${NASM_SRC} || true
    tar xf ${DIST}/ffmpeg/${NASM_SRC}.tar.xz
    cd ${NASM_SRC}
    ./configure ${COMMON_CONFIGURE}
    make -j${MKJOBS}
    make install
fi # nasm

# yasm
if [ ! -f "${SDK}/bin/yasm" ]; then
    cd ${SRC}
    YASM_SRC=yasm-${YASM_V}
    rm -rf ${YASM_SRC} || true
    tar xf ${DIST}/tools/${YASM_SRC}.tar.gz
    cd ${YASM_SRC}
    ./configure ${COMMON_CONFIGURE}
    make -j${MKJOBS}
    make install
fi # yasm

# qt6
if [ ! -f "${QMAKE_BIN}" ]; then
    cd ${SRC}
    QT_SRC="qt-everywhere-src-${QT_V}"
    QT_TAR_SRC="qt-everywhere-opensource-src-${QT_V}"
    if [ ! -d "${QT_SRC}" ]; then
        tar xf ${DIST}/qt/${QT_TAR_SRC}.${SRC_SUFFIX}
    fi
    cd ${QT_SRC}
    ./configure \
    -prefix ${SDK} \
    -c++std c++14 \
    -opengl desktop \
    -release \
    -shared \
    -opensource \
    -confirm-license \
    -optimize-size \
    -strip \
    -qt-pcre \
    -qt-zlib \
    -qt-libpng \
    -no-framework \
    -no-mtdev \
    -no-syslog \
    -no-pch \
    -no-glib \
    -no-dbus \
    -no-avx2 \
    -no-avx512 \
    -no-gif \
    -no-ico \
    -no-tiff \
    -no-webp \
    -no-jasper \
    -no-libjpeg \
    -no-ssl \
    -no-cups \
    -no-mng \
    -no-gstreamer \
    -no-alsa \
    -no-sql-db2 \
    -no-sql-ibase \
    -no-sql-mysql \
    -no-sql-oci \
    -no-sql-odbc \
    -no-sql-psql \
    -no-sql-sqlite2 \
    -no-sql-sqlite \
    -no-sql-tds \
    -no-gtk \
    -no-eglfs \
    -no-kms \
    -nomake examples \
    -nomake tests \
    -skip qt3d \
    -skip qtactiveqt \
    -skip qtcanvas3d \
    -skip qtcharts \
    -skip qtconnectivity \
    -skip qtdatavis3d \
    -skip qtdoc \
    -skip qtgamepad \
    -skip qtgraphicaleffects \
    -skip qtlocation \
    -skip qtlottie \
    -skip qtnetworkauth \
    -skip qtpurchasing \
    -skip qtquick3d \
    -skip qtquickcontrols \
    -skip qtquickcontrols2 \
    -skip qtremoteobjects \
    -skip qtscript \
    -skip qtscxml \
    -skip qtsensors \
    -skip qtserialbus \
    -skip qtserialport \
    -skip qtspeech \
    -skip qtvirtualkeyboard \
    -skip qtwebchannel \
    -skip qtwebengine \
    -skip qtwebglplugin \
    -skip qtwebsockets \
    -skip qtwebview \
    -skip qtxmlpatterns
    make -j${MKJOBS}
    make install
fi # qt6

# qscintilla
if [ ! -f "${SDK}/lib/libqscintilla2_qt6.dylib" ]; then
    cd ${SRC}
    QSC_SRC="QScintilla_src-${QSCINTILLA_V}"
    rm -rf ${QSC_SRC}
    tar xf ${DIST}/qt/${QSC_SRC}.tar.gz
    cd ${QSC_SRC}/src
    sed -i '' 's#!ios:QT += printsupport##' qscintilla.pro
    sed -i '' 's#!ios:HEADERS += ./Qsci/qsciprinter.h##' qscintilla.pro
    sed -i '' 's#!ios:SOURCES += qsciprinter.cpp##' qscintilla.pro
    ${SDK}/bin/qmake CONFIG+=release
    make -j${MKJOBS}
    cp -a libqscintilla2_qt6* ${SDK}/lib/
    cp -a Qsci ${SDK}/include/
fi # qscintilla

# lame
if [ ! -f "${SDK}/lib/libmp3lame.dylib" ]; then
    cd ${SRC}
    LAME_SRC=lame-${LAME_V}
    rm -rf ${LAME_SRC} || true
    tar xf ${DIST}/ffmpeg/${LAME_SRC}.tar.gz
    cd ${LAME_SRC}
    patch -p0 < ${DIST}/ffmpeg/patch-lame-avoid_undefined_symbols_error.diff
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    ./configure ${DEFAULT_CONFIGURE} --disable-frontend --disable-gtktest --with-fileio=lame --enable-nasm
    make -j${MKJOBS}
    make install
fi # lame

# libvpx
if [ ! -f "${SDK}/lib/libvpx.dylib" ]; then
    cd ${SRC}
    VPX_SRC=libvpx-${VPX_V}
    rm -rf ${VPX_SRC} || true
    tar xf ${DIST}/ffmpeg/libvpx-${VPX_V}.tar.gz
    cd ${VPX_SRC}
    patch -p0 < ${DIST}/ffmpeg/patch-vpx-Makefile.diff
    patch -p0 < ${DIST}/ffmpeg/patch-vpx-configure.sh.diff
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    ./configure ${DEFAULT_CONFIGURE} \
    --enable-vp8 \
    --enable-vp9 \
    --enable-internal-stats \
    --enable-pic \
    --enable-postproc \
    --enable-multithread \
    --enable-runtime-cpu-detect \
    --enable-experimental \
    --enable-shared \
    --disable-install-docs \
    --disable-debug-libs \
    --disable-examples \
    --disable-unit-tests
    make -j${MKJOBS}
    make install
fi # libvpx

# libogg
if [ ! -f "${SDK}/lib/libogg.dylib" ]; then
    cd ${SRC}
    OGG_SRC=libogg-${OGG_V}
    rm -rf ${OGG_SRC} || true
    tar xf ${DIST}/ffmpeg/${OGG_SRC}.tar.gz
    cd ${OGG_SRC}
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    ./configure ${DEFAULT_CONFIGURE}
    make -j${MKJOBS}
    make install
fi # libogg

# libvorbis
if [ ! -f "${SDK}/lib/libvorbis.dylib" ]; then
    cd ${SRC}
    VORBIS_SRC=libvorbis-${VORBIS_V}
    rm -rf ${VORBIS_SRC} || true
    tar xf ${DIST}/ffmpeg/${VORBIS_SRC}.tar.gz
    cd ${VORBIS_SRC}
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    ./configure ${DEFAULT_CONFIGURE}
    make -j${MKJOBS}
    make install
fi # libvorbis

# libtheora
if [ ! -f "${SDK}/lib/libtheora.dylib" ]; then
    cd ${SRC}
    THEORA_SRC=libtheora-${THEORA_V}
    rm -rf ${THEORA_SRC} || true
    tar xf ${DIST}/ffmpeg/${THEORA_SRC}.tar.gz
    cd ${THEORA_SRC}
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    ./configure ${DEFAULT_CONFIGURE} --disable-examples --disable-sdltest
    make -j${MKJOBS}
    make install
fi # libtheora

# xvidcore
if [ ! -f "${SDK}/lib/libxvidcore.4.dylib" ]; then
    cd ${SRC}
    rm -rf xvidcore || true
    tar xf ${DIST}/ffmpeg/xvidcore-${XVID_V}.tar.gz
    cd xvidcore/build/generic
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    ./configure ${COMMON_CONFIGURE}
    make -j${MKJOBS}
    make install
fi # xvidcore

# liblsmash
#if [ ! -f "${SDK}/lib/liblsmash.dylib" ]; then
#    cd ${SRC}
#    LSMASH_SRC=l-smash-${LSMASH_V}
#    rm -rf ${LSMASH_SRC} || true
#    tar xf ${DIST}/ffmpeg/liblsmash-v${LSMASH_V}.tar.gz
#    cd ${LSMASH_SRC}
#    CFLAGS="${DEFAULT_CFLAGS}" \
#    CXXFLAGS="${DEFAULT_CFLAGS}" \
#    LDFLAGS="${DEFAULT_LDFLAGS}" \
#    ./configure ${DEFAULT_CONFIGURE}
#    make -j${MKJOBS}
#    make install
#fi # liblsmash

# x264
if [ ! -f "${SDK}/lib/libx264.dylib" ]; then
    cd ${SRC}
    X264_SRC=x264-snapshot-${X264_V}
    rm -rf ${X264_SRC} || true
    tar xf ${DIST}/ffmpeg/${X264_SRC}.tar.bz2
    cd ${X264_SRC}
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    ./configure ${COMMON_CONFIGURE} --enable-shared --disable-lavf --disable-swscale --disable-opencl --disable-cli
    make -j${MKJOBS}
    make install
fi # x264

# x265
if [ ! -f "${SDK}/lib/libx265.dylib" ]; then
    cd ${SRC}
    X265_SRC=x265_${X265_V}
    rm -rf ${X265_SRC} || true
    tar xf ${DIST}/ffmpeg/${X265_SRC}.tar.gz
    cd ${X265_SRC}/source
    mkdir build && cd build
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    cmake \
    -DCMAKE_INSTALL_PREFIX=${SDK} \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_SHARED=ON \
    -DENABLE_CLI=OFF ..
    make -j${MKJOBS}
    make install
fi # x265

# aom
if [ ! -f "${SDK}/lib/libaom.dylib" ]; then
    cd ${SRC}
    AOM_SRC=libaom-${AOM_V}
    rm -rf ${AOM_SRC} || true
    tar xf ${DIST}/ffmpeg/${AOM_SRC}.tar.gz
    cd ${AOM_SRC}
    mkdir build2 && cd build2
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    cmake \
    -DCMAKE_INSTALL_PREFIX=${SDK} \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_NASM=ON \
    -DENABLE_DOCS=OFF \
    -DENABLE_TESTS=OFF \
    -DENABLE_TESTDATA=OFF \
    -DENABLE_TOOLS=OFF \
    -DENABLE_EXAMPLES=OFF \
    -DCONFIG_AV1_HIGHBITDEPTH=0 \
    -DCONFIG_WEBM_IO=0 \
    -DBUILD_SHARED_LIBS=ON ..
    make -j${MKJOBS}
    make install
fi # aom

if [ ! -f "${SDK}/lib/pkgconfig/libavcodec.pc" ]; then
    cd ${SRC}
    FFMPEG_SRC=ffmpeg-${FFMPEG_V}
    rm -rf ${FFMPEG_SRC} || true
    tar xf ${DIST}/ffmpeg/${FFMPEG_SRC}.tar.xz
    cd ${FFMPEG_SRC}
    CFLAGS="${DEFAULT_CFLAGS}" \
    CXXFLAGS="${DEFAULT_CFLAGS}" \
    LDFLAGS="${DEFAULT_LDFLAGS}" \
    ./configure ${SHARED_CONFIGURE} \
    --disable-securetransport \
    --disable-videotoolbox \
    --disable-libxcb \
    --disable-hwaccels \
    --disable-devices \
    --disable-openssl \
    --disable-sdl2 \
    --disable-xlib \
    --disable-libxcb \
    --disable-libv4l2 \
    --disable-alsa \
    --disable-network \
    --disable-programs \
    --disable-debug \
    --disable-doc \
    --enable-avresample \
    --enable-gpl \
    --enable-version3 \
    --disable-avisynth \
    --disable-gnutls \
    --disable-libass \
    --disable-libbluray \
    --disable-libbs2b \
    --disable-libcaca \
    --enable-libmp3lame \
    --disable-libopencore-amrnb \
    --disable-libopencore-amrwb \
    --disable-libopus \
    --disable-libspeex \
    --enable-libtheora \
    --disable-libvidstab \
    --disable-libvo-amrwbenc \
    --enable-libvorbis \
    --enable-libvpx \
    --enable-libx264 \
    --enable-libaom \
    --enable-libx265 \
    --enable-libxvid
     make -j${MKJOBS}
     make install
fi # ffmpeg

(cd ${SDK}/lib ;
install_name_tool -change libvpx.8.dylib @rpath/libvpx.8.dylib libavformat.58.dylib
install_name_tool -change libvpx.8.dylib @rpath/libvpx.8.dylib libavcodec.58.dylib
)

echo "Friction macOS SDK done!"
