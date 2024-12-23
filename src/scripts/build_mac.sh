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
SDK=${SDK:-"${CWD}/sdk"}
BUILD_ENGINE=${BUILD_ENGINE:-"ON"}
REL=${REL:-"OFF"}
BRANCH=${BRANCH:-`git rev-parse --abbrev-ref HEAD`}
COMMIT=${COMMIT:-`git rev-parse --short=8 HEAD`}
CUSTOM=${CUSTOM:-""}
BUILD_DIR=${BUILD_DIR:-"${CWD}/build-release"}

export PATH="${SDK}/bin:/usr/bin:/bin:/usr/sbin:/sbin"
export PKG_CONFIG_PATH="${SDK}/lib/pkgconfig"

if [ -d "${BUILD_DIR}" ]; then
    rm -rf ${BUILD_DIR}
fi
mkdir ${BUILD_DIR} && cd ${BUILD_DIR}

cmake -G Ninja \
-DCMAKE_OSX_DEPLOYMENT_TARGET=12.7 \
-DMAC_DEPLOY=ON \
-DGIT_COMMIT=${COMMIT} \
-DGIT_BRANCH=${BRANCH} \
-DFRICTION_OFFICIAL_RELEASE=${REL} \
-DCUSTOM_BUILD=${CUSTOM} \
-DBUILD_ENGINE=${BUILD_ENGINE} \
-DCMAKE_BUILD_TYPE=Release \
-DQSCINTILLA_INCLUDE_DIRS=${SDK}/include \
-DQSCINTILLA_LIBRARIES_DIRS=${SDK}/lib \
${CWD}

VERSION=`cat version.txt`
if [ "${REL}" != "ON" ]; then
    VERSION="${VERSION}-${COMMIT}"
fi

cmake --build .

#if [ "${BUILD_ENGINE}" = "ON" ]; then
#    (cd src/engine ;
#        tar cvvf friction-skia-build-${GIT_COMMIT}.tar skia
#        mv friction-skia-build-${GIT_COMMIT}.tar ${BUILD_DIR}/
#    )
#fi

mv src/app/friction.app src/app/Friction.app
macdeployqt src/app/Friction.app

rm -f src/app/Friction.app/Contents/Frameworks/{libQt6MultimediaWidgets.5.dylib,libQt6Svg.5.dylib}
rm -rf src/app/Friction.app/Contents/PlugIns/{bearer,iconengines,imageformats,mediaservice,printsupport,styles}

mkdir dmg && mv src/app/Friction.app dmg/
hdiutil create -volname "Friction" -srcfolder dmg -ov -format ULMO Friction-${VERSION}-x86_64.dmg
