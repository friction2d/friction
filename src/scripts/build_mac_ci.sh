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

set -e -x

CWD=`pwd`

GHA_RUN_NUMBER=${GHA_RUN_NUMBER:-0}
BUILD_ORIGIN=${BUILD_ORIGIN:-ci}
BUILD_ARCH=${BUILD_ARCH:-"universal"}

SDK=1.0.0
SDK_REV=r5
SDK_URL=https://github.com/friction2d/friction-sdk/releases/download/v${SDK}
SDK_TAR=friction-sdk-${SDK}${SDK_REV}-macOS.tar.xz
SDK_SHA256=36a30cb68862d3cd0fe39f9c283f1a9fb9cf2ea01a9dfc65c85024b0c2171d2d

if [ ! -d "${CWD}/sdk" ]; then
    curl -OL ${SDK_URL}/${SDK_TAR}
    echo "${SDK_SHA256}  ${SDK_TAR}" | shasum -a 256 --check
    tar xf ${SDK_TAR}
fi

git submodule update --init --recursive

export GHA_RUN_NUMBER
export BUILD_ORIGIN

if [ "${BUILD_ARCH}" = "x86_64" ] || [ "${BUILD_ARCH}" = "universal" ]; then
    cd ${CWD}
    arch -x86_64 ./src/scripts/build_mac.sh
fi

if [ "${BUILD_ARCH}" = "arm64" ] || [ "${BUILD_ARCH}" = "universal" ]; then
    cd ${CWD}
    ./src/scripts/build_mac.sh
fi

if [ "${BUILD_ARCH}" = "universal" ]; then
    cd ${CWD}
    VERSION=`cat build-release-arm64/version.txt`
    VERSION=${VERSION} ./src/scripts/build_mac_universal.sh
fi
