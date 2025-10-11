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

SDK=1.0.0
SDK_REV=r2
SDK_URL=https://github.com/friction2d/friction-sdk/releases/download/v${SDK}
SDK_TAR=friction-sdk-${SDK}${SDK_REV}-macOS.tar.xz
SDK_SHA256=0130555da20b1ebabda97f182cad4e2a5efff9d74142378bb2811b83df1826c6

if [ ! -d "${CWD}/sdk" ]; then
    curl -OL ${SDK_URL}/${SDK_TAR}
    echo "${SDK_SHA256}  ${SDK_TAR}" | shasum -a 256 --check
    tar xf ${SDK_TAR}
fi

git submodule update --init --recursive

export CUSTOM=CI
export REL=$REL

cd ${CWD}
arch -x86_64 ./src/scripts/build_mac.sh

cd ${CWD}
./src/scripts/build_mac.sh

cd ${CWD}

VERSION=`cat build-release-arm64/version.txt`
if [ "${REL}" != "ON" ]; then
    VERSION="${VERSION}-${COMMIT}"
fi

VERSION=${VERSION} ./src/scripts/build_mac_universal.sh
