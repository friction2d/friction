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

CWD=`pwd`
MKJOBS=${MKJOBS:-2}
COMMIT=`git rev-parse --short=8 HEAD`
BRANCH=`git rev-parse --abbrev-ref HEAD`
CUSTOM=${CUSTOM:-"CI"}

BUILD_ENGINE=${BUILD_ENGINE:-"OFF"}
REL=${REL:-0}
APPIMG=20240401
SDK=1.0.0
URL=https://github.com/friction2d/friction-sdk/releases/download/v${SDK}
APPIMAGE_TAR=friction-appimage-tools-${APPIMG}.tar.xz
SDK_TAR=friction-sdk-${SDK}r5-linux-x86_64.tar.xz
SKIA_TAR=skia-friction-f5941b02-linux-x86_64.tar.xz

mkdir -p distfiles/sdk || true

cd distfiles
if [ ! -d "linux" ]; then
    if [ ! -f "${APPIMAGE_TAR}" ]; then
        wget ${URL}/${APPIMAGE_TAR}
    fi
    tar xvf ${APPIMAGE_TAR}
fi

if [ ! -d "skia" ]; then
    if [ ! -f "${SKIA_TAR}" ]; then
        wget ${URL}/${SKIA_TAR}
    fi
    tar xvf ${SKIA_TAR}
fi

cd sdk
if [ ! -f "${SDK_TAR}" ]; then
    wget ${URL}/${SDK_TAR}
fi

cd ${CWD}

BUILD_ENGINE=${BUILD_ENGINE} LOCAL_BUILD=0 MKJOBS=${MKJOBS} REL=${REL} BRANCH=${BRANCH} COMMIT=${COMMIT} CUSTOM=${CUSTOM} ./src/scripts/run_docker.sh
