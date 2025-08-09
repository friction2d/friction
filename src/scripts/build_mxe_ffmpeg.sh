#!/bin/bash
set -e -x

CWD=`pwd`
VERSION=4.2.11
BUILD=${CWD}/ffmpeg-friction-${VERSION}-win64
MXE=${CWD}/usr/x86_64-w64-mingw32.static

if [ ! -f "${CWD}/settings.mk" ]; then
    echo "Run script in the MXE folder!"
    exit 1
fi

if [ -d "${BUILD}" ]; then
    rm -rf ${BUILD}
fi
mkdir -p ${BUILD}/{bin,include}

make

cp -a ${MXE}/include/{libavcodec,libavdevice,libavfilter,libavformat,libavresample,libavutil,libpostproc,libswresample,libswscale} ${BUILD}/include/
cp -a ${MXE}/lib/*.def ${BUILD}/bin/
cp -a ${MXE}/bin/*.{dll,lib} ${BUILD}/bin/

if [ -d "${CWD}/legal" ]; then
    cp -a ${CWD}/legal ${BUILD}/
fi

zip -9 -r ${BUILD}.zip ${BUILD}
