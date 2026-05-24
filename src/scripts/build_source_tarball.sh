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
BUILD_DIR=${CWD}/build-source
BUILD_TMP=${CWD}/build-tmp
BUILD_ORIGIN=${BUILD_ORIGIN:-local}

if [ -d "${BUILD_DIR}" ]; then
    rm -rf ${BUILD_DIR}
fi
mkdir -p ${BUILD_DIR}

if [ -d "${BUILD_TMP}" ]; then
    rm -rf ${BUILD_TMP}
fi
mkdir -p ${BUILD_TMP}

cd ${BUILD_TMP}
cmake -G Ninja \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_INSTALL_PREFIX=/usr \
-DCMAKE_CXX_COMPILER=clang++ \
-DCMAKE_C_COMPILER=clang \
-DBUILD_ORIGIN=${BUILD_ORIGIN} \
..
VERSION=`cat version.txt`

cd ${BUILD_DIR}
git clone ${CWD} friction-${VERSION}
cd friction-${VERSION}
git checkout ${COMMIT}
git submodule update -i --recursive

${CWD}/src/scripts/build_changelog.sh

cd src/engine/skia
python3 tools/git-sync-deps

cd ${BUILD_DIR}/friction-${VERSION}

# no symlinks please
find . -type l -exec sh -c 'for i in "$@"; do cp --preserve --remove-destination "$(readlink -f "$i")" "$i"; done' sh {} +

# remove git anything
find . \( -name ".git" -o -name ".github" -o -name ".gitignore" -o -name ".gitmodules" -o -name ".gitattributes" \) -exec rm -rf -- {} +

cd ${BUILD_DIR}
tar cvvf friction-${VERSION}.tar friction-${VERSION}
xz -9 friction-${VERSION}.tar
