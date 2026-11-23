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

REL=${REL:-"OFF"}
BRANCH=${BRANCH:-`git rev-parse --abbrev-ref HEAD`}
COMMIT=${COMMIT:-`git rev-parse --short=8 HEAD`}
CUSTOM=${CUSTOM:-""}
BUILD_ARM=${BUILD_ARM:-1}
BUILD_INTEL=${BUILD_INTEL:-1}
BUILD_UNIVERSAL=${BUILD_UNIVERSAL:-1}

if [ "${BUILD_UNIVERSAL}" = 1 ]; then
    BUILD_ARM=1
    BUILD_INTEL=1
fi

SDK_PATH=${CWD}/sdk
SDK_VERSION=20261121
SDK_REV=r1
SDK_URL=https://github.com/friction2d/friction-sdk/releases/download/v${SDK_VERSION}
SDK_BASE=friction-sdk-${SDK_VERSION}${SDK_REV}-macOS
SDK_ARM_TAR=${SDK_BASE}-arm64.tar.xz
SDK_INTEL_TAR=${SDK_BASE}-x86_64.tar.xz
SDK_ARM_SHA256=e7ad987a45ca8807eb4a97e46f044f6cf54ad52f4669c640de39ffc9353d8579
SDK_INTEL_SHA256=3e4a362b71c9553f967d145345f1d81fefe1a3dc24c91881296edff5919a1a02

if [ ! -d "${SDK_PATH}" ]; then
    mkdir -p "${SDK_PATH}"
    if [ "${BUILD_ARM}" = 1 ]; then
        curl -OL ${SDK_URL}/${SDK_ARM_TAR}
        echo "${SDK_ARM_SHA256}  ${SDK_ARM_TAR}" | shasum -a 256 --check
        (cd "${SDK_PATH}" ; tar xf ${CWD}/${SDK_ARM_TAR} )
    fi
    if [ "${BUILD_INTEL}" = 1 ]; then
        curl -OL ${SDK_URL}/${SDK_INTEL_TAR}
        echo "${SDK_INTEL_SHA256}  ${SDK_INTEL_TAR}" | shasum -a 256 --check
        (cd "${SDK_PATH}" ; tar xf ${CWD}/${SDK_INTEL_TAR} )
    fi
fi

git submodule update --init --recursive

export CUSTOM=${CUSTOM}
export REL=${REL}
export BRANCH=${BRANCH}
export COMMIT=${COMMIT}

if [ "${BUILD_INTEL}" = 1 ]; then
    cd "${CWD}"
    arch -x86_64 ./src/scripts/build_mac.sh
fi

if [ "${BUILD_ARM}" = 1 ]; then
    cd "${CWD}"
    ./src/scripts/build_mac.sh
fi

if [ "${BUILD_UNIVERSAL}" = 1 ]; then
    cd "${CWD}"
    VERSION=`cat build-release-arm64/version.txt`
    if [ "${REL}" != "ON" ]; then
        VERSION="${VERSION}-${COMMIT}"
    fi
    VERSION=${VERSION} ./src/scripts/build_mac_universal.sh
fi
