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

source /opt/rh/devtoolset-7/enable
gcc -v

SDK=${SDK:-"/opt/friction"}
SRC=${SDK}/src
DIST=${DIST:-"/mnt"}
MKJOBS=${MKJOBS:-32}
SRC_SUFFIX=tar.xz

QT_V=5.15.16_20241121_32be1543
QSCINTILLA_V=2.14.1

PELF_V=0.17.0
CMAKE_V=3.26.3

XCB_V=1.17.0
XCB_UTIL_V=0.4.1
XCB_CURSOR_V=0.1.4
XCB_ERRORS_V=1.0.1
XCB_IMAGE_V=0.4.1
XCB_KEYSYMS_V=0.4.1
XCB_RENDERUTIL_V=0.3.10
XCB_WM_V=0.4.2
XKBCOMMON_V=0.8.4

NINJA_BIN=${SDK}/bin/ninja
CMAKE_BIN=${SDK}/bin/cmake
PELF_BIN=${SDK}/bin/patchelf
QMAKE_BIN=${SDK}/bin/qmake

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

STATIC_CFLAGS="-fPIC"
DEFAULT_CFLAGS="-I${SDK}/include"
DEFAULT_LDFLAGS="-L${SDK}/lib"
COMMON_CONFIGURE="--prefix=${SDK}"
SHARED_CONFIGURE="${COMMON_CONFIGURE} --enable-shared --disable-static"
STATIC_CONFIGURE="${COMMON_CONFIGURE} --disable-shared --enable-static"
DEFAULT_CONFIGURE="${SHARED_CONFIGURE}"
XCB_CONFIGURE=${DEFAULT_CONFIGURE}

# patchelf
if [ ! -f "${PELF_BIN}" ]; then
    cd ${SRC}
    PELF_SRC=patchelf-${PELF_V}
    rm -rf ${PELF_SRC} || true
    tar xf ${DIST}/linux/${PELF_SRC}.tar.bz2
    cd ${PELF_SRC}
    ./configure ${COMMON_CONFIGURE}
    make -j${MKJOBS}
    make install
fi # patchelf

# xcb
if [ ! -f "${SDK}/lib/pkgconfig/xcb.pc" ]; then
    # https://github.com/pypa/pip/issues/10219#issuecomment-888127061
    export LANG="en_US.UTF-8"
    export LC_ALL="en_US.UTF-8"
    # proto
    cd ${SRC}
    XCB_PROTO_SRC=xcb-proto-${XCB_V}
    rm -rf ${XCB_PROTO_SRC} || true
    tar xf ${DIST}/x11/${XCB_PROTO_SRC}.${SRC_SUFFIX}
    cd ${XCB_PROTO_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
    mv ${SDK}/share/pkgconfig/* ${SDK}/lib/pkgconfig/
    # lib
    cd ${SRC}
    XCB_SRC=libxcb-${XCB_V}
    rm -rf ${XCB_SRC} || true
    tar xf ${DIST}/x11/${XCB_SRC}.${SRC_SUFFIX}
    cd ${XCB_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
    # util
    cd ${SRC}
    XCB_UTIL_SRC=xcb-util-${XCB_UTIL_V}
    rm -rf ${XCB_UTIL_SRC} || true
    tar xf ${DIST}/x11/${XCB_UTIL_SRC}.${SRC_SUFFIX}
    cd ${XCB_UTIL_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
    # errors
    cd ${SRC}
    XCB_ERRORS_SRC=xcb-util-errors-${XCB_ERRORS_V}
    rm -rf ${XCB_ERRORS_SRC} || true
    tar xf ${DIST}/x11/${XCB_ERRORS_SRC}.${SRC_SUFFIX}
    cd ${XCB_ERRORS_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
    # image
    cd ${SRC}
    XCB_IMAGE_SRC=xcb-util-image-${XCB_IMAGE_V}
    rm -rf ${XCB_IMAGE_SRC} || true
    tar xf ${DIST}/x11/${XCB_IMAGE_SRC}.${SRC_SUFFIX}
    cd ${XCB_IMAGE_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
    # keysyms
    cd ${SRC}
    XCB_KEYSYMS_SRC=xcb-util-keysyms-${XCB_KEYSYMS_V}
    rm -rf ${XCB_KEYSYMS_SRC} || true
    tar xf ${DIST}/x11/${XCB_KEYSYMS_SRC}.${SRC_SUFFIX}
    cd ${XCB_KEYSYMS_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
    # renderutil
    cd ${SRC}
    XCB_RENDERUTIL_SRC=xcb-util-renderutil-${XCB_RENDERUTIL_V}
    rm -rf ${XCB_RENDERUTIL_SRC} || true
    tar xf ${DIST}/x11/${XCB_RENDERUTIL_SRC}.${SRC_SUFFIX}
    cd ${XCB_RENDERUTIL_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
    # cursor
    cd ${SRC}
    XCB_CURSOR_SRC=xcb-util-cursor-${XCB_CURSOR_V}
    rm -rf ${XCB_CURSOR_SRC} || true
    tar xf ${DIST}/x11/${XCB_CURSOR_SRC}.${SRC_SUFFIX}
    cd ${XCB_CURSOR_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
    # wm
    cd ${SRC}
    XCB_WM_SRC=xcb-util-wm-${XCB_WM_V}
    rm -rf ${XCB_WM_SRC} || true
    tar xf ${DIST}/x11/${XCB_WM_SRC}.${SRC_SUFFIX}
    cd ${XCB_WM_SRC}
    ./configure ${XCB_CONFIGURE}
    make -j${MKJOBS}
    make install
fi # xcb

# libxkbcommon
if [ ! -f "${SDK}/lib/pkgconfig/xkbcommon.pc" ]; then
    cd ${SRC}
    XKB_SRC=libxkbcommon-${XKBCOMMON_V}
    rm -rf ${XKB_SRC} || true
    tar xf ${DIST}/x11/${XKB_SRC}.${SRC_SUFFIX}
    cd ${XKB_SRC}
    ./configure ${DEFAULT_CONFIGURE} \
    --disable-docs \
    --with-xkb-config-root=/usr/share/X11/xkb \
    --with-x-locale-root=/usr/share/X11/locale \
    --with-default-rules=evdev \
    --with-default-model=pc105 \
    --with-default-layout=us
    make -j${MKJOBS}
    make install
fi # libxkbcommon

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

# qt
#    -no-feature-bearermanagement \
#    -no-feature-dnslookup \
#    -no-feature-dtls \
#    -no-feature-ftp \
#    -no-feature-gssapi \
#    -no-feature-http \
#    -no-feature-localserver \
#    -no-feature-netlistmgr \
#    -no-feature-networkdiskcache \
#    -no-feature-networkinterface \
#    -no-feature-networkproxy \
#    -no-feature-qml-network \
#    -no-feature-qml-xml-http-request \
#    -no-feature-qml-animation \
#    -no-feature-socks5 \
#    -no-feature-sspi \
#    -no-feature-udpsocket \
#    -no-feature-printdialog \
#    -no-feature-printer \
#    -no-feature-printpreviewdialog \
#    -no-feature-printpreviewwidget \
#    -no-feature-pdf \
if [ ! -f "${QMAKE_BIN}" ]; then
    cd ${SRC}
    QT_SRC="qt-everywhere-src-${QT_V}"
    rm -rf ${QT_SRC} || true
    tar xf ${DIST}/qt/${QT_SRC}.${SRC_SUFFIX}
    cd ${QT_SRC}
    (cd qtbase ; xzcat ${DIST}/qt/qtbase-use-wayland-on-gnome.patch.xz | patch -p1)
    ./configure \
    -prefix ${SDK} \
    -c++std c++14 \
    -qtlibinfix Friction \
    -opengl desktop \
    -release \
    -shared \
    -opensource \
    -confirm-license \
    -optimize-size \
    -strip \
    -pulseaudio \
    -fontconfig \
    -system-freetype \
    -qt-pcre \
    -qt-zlib \
    -xkbcommon \
    -xcb \
    -xcb-xlib \
    -qpa xcb \
    -bundled-xcb-xinput \
    -qt-libpng \
    -no-mtdev \
    -no-syslog \
    -no-pch \
    -no-glib \
    -dbus \
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
    -no-linuxfb \
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
    -skip qttools \
    -skip qtvirtualkeyboard \
    -skip qtwebchannel \
    -skip qtwebengine \
    -skip qtwebglplugin \
    -skip qtwebsockets \
    -skip qtwebview \
    -skip qtx11extras \
    -skip qtxmlpatterns \
    -skip qttools
    make -j${MKJOBS}
    make install
fi # qt

# qscintilla
if [ ! -f "${SDK}/lib/libqscintilla2_friction_qt5.so" ]; then
    cd ${SRC}
    QSC_SRC="QScintilla_src-${QSCINTILLA_V}"
    rm -rf ${QSC_SRC}
    tar xf ${DIST}/qt/${QSC_SRC}.tar.gz
    cd ${QSC_SRC}/src
    sed -i 's/qscintilla2_qt/qscintilla2_friction_qt/g' qscintilla.pro
    sed -i 's#!ios:QT += printsupport##' qscintilla.pro
    sed -i 's#!ios:HEADERS += ./Qsci/qsciprinter.h##' qscintilla.pro
    sed -i 's#!ios:SOURCES += qsciprinter.cpp##' qscintilla.pro
    ${SDK}/bin/qmake CONFIG+=release
    make -j${MKJOBS}
    cp -a libqscintilla2_friction_qt5* ${SDK}/lib/
    cp -a Qsci ${SDK}/include/
fi # qscintilla

echo "SDK PART 2 DONE"
