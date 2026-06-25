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
CUSTOM=${CUSTOM:-"CI"}
BUILD_ARM=${BUILD_ARM:-1}
BUILD_INTEL=${BUILD_INTEL:-1}
BUILD_UNIVERSAL=${BUILD_UNIVERSAL:-1}

if [ "${BUILD_UNIVERSAL}" = 1 ]; then
    BUILD_ARM=1
    BUILD_INTEL=1
fi

SDK_PATH=${CWD}/sdk
SDK_VERSION=1.0.0
SDK_REV=r6
SDK_URL=https://github.com/friction2d/friction-sdk/releases/download/v${SDK_VERSION}
SDK_BASE=friction-sdk-${SDK_VERSION}${SDK_REV}-macOS
SDK_ARM_TAR=${SDK_BASE}-arm64.tar.xz
SDK_INTEL_TAR=${SDK_BASE}-x86_64.tar.xz
SDK_ARM_SHA256=46a6e44b5d55f681f9dbddaa321635bfbc18cd080a649e48029d6b28cb41ad85
SDK_INTEL_SHA256=c25c2a220a6f801edde67b2ce6aca3564ff7a0f7e0c00e35bac7878f1632d560

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
