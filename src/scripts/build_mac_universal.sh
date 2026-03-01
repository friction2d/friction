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
VERSION=${VERSION:-"dev"}

ARM_BUILD=${CWD}/build-release-arm64
INTEL_BUILD=${CWD}/build-release-x86_64
UNI_BUILD=${CWD}/build-release-universal

ARM_FRAMEWORK=${ARM_BUILD}/dmg/Friction.app/Contents/Frameworks
INTEL_FRAMEWORK=${INTEL_BUILD}/dmg/Friction.app/Contents/Frameworks
UNI_FRAMEWORK=${UNI_BUILD}/Friction.app/Contents/Frameworks

ARM_PLUGINS=${ARM_BUILD}/dmg/Friction.app/Contents/PlugIns
INTEL_PLUGINS=${INTEL_BUILD}/dmg/Friction.app/Contents/PlugIns
UNI_PLUGINS=${UNI_BUILD}/Friction.app/Contents/PlugIns

ARM_APP=${ARM_BUILD}/dmg/Friction.app/Contents/MacOS
INTEL_APP=${INTEL_BUILD}/dmg/Friction.app/Contents/MacOS
UNI_APP=${UNI_BUILD}/Friction.app/Contents/MacOS

PLIST=${UNI_BUILD}/Friction.app/Contents/Info.plist

if [ -d "${UNI_BUILD}" ]; then
    rm -rf "${UNI_BUILD}"
fi
mkdir -p "${UNI_BUILD}"

cp -a ${ARM_BUILD}/dmg/Friction.app ${UNI_BUILD}/Friction.app

cd ${UNI_FRAMEWORK}
for lib in * ; do lipo -create -output $lib ${ARM_FRAMEWORK}/$lib ${INTEL_FRAMEWORK}/$lib ; done

cd ${UNI_PLUGINS}/audio
lipo -create -output libqtaudio_coreaudio.dylib ${ARM_PLUGINS}/audio/libqtaudio_coreaudio.dylib ${INTEL_PLUGINS}/audio/libqtaudio_coreaudio.dylib

cd ${UNI_PLUGINS}/platforms
lipo -create -output libqcocoa.dylib ${ARM_PLUGINS}/platforms/libqcocoa.dylib ${INTEL_PLUGINS}/platforms/libqcocoa.dylib

cd ${UNI_APP}
lipo -create -output friction ${ARM_APP}/friction ${INTEL_APP}/friction

cd ${UNI_BUILD}

plutil -insert LSArchitecturePriority -array ${PLIST}
plutil -insert LSArchitecturePriority.0 -string arm64 ${PLIST}
plutil -insert LSArchitecturePriority.1 -string x86_64 ${PLIST}

mkdir dmg
mv Friction.app dmg/
(cd dmg ; ln -sf /Applications Applications)

DOC=Friction.app/Contents/Resources/docs/index.html
if [ -f "dmg/${DOC}" ]; then
    (cd dmg ; ln -sf ${DOC} Documentation.html)
fi

# https://github.com/actions/runner-images/issues/7522
max_tries=10
i=0
until hdiutil create -volname "Friction" -srcfolder dmg -ov -format ULMO Friction-${VERSION}.dmg
do
    if [ $i -eq $max_tries ]; then
        echo 'Error: hdiutil did not succeed even after 10 tries.'
        exit 1
    fi
    i=$((i+1))
done
