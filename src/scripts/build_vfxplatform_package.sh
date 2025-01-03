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

SDK=${SDK:-"/opt/friction"}
DISTFILES=${DISTFILES:-"/mnt"}
BUILD=${BUILD:-"${HOME}"}
VERSION=${VERSION:-""}
APPID="graphics.friction.Friction"
FRICTION_PKG=friction-${VERSION}

APPIMAGETOOL=bfe6e0c
APPIMAGERUNTIME=1bb1157

if [ "${VERSION}" = "" ]; then
    echo "Missing version"
    exit 1
fi
PKG_VERSION=`echo ${VERSION} | sed 's/-/./g'`

if [ ! -d "${BUILD}/${FRICTION_PKG}" ]; then
    echo "Missing Friction build"
    exit 1
fi

if [ ! -d "${DISTFILES}/builds/${VERSION}" ]; then
    mkdir -p ${DISTFILES}/builds/${VERSION}
fi

export PATH="${SDK}/bin:${PATH}"
export PKG_CONFIG_PATH="${SDK}/lib/pkgconfig"
export LD_LIBRARY_PATH="${SDK}/lib:${LD_LIBRARY_PATH}"

BIN_DIR=${BUILD}/${FRICTION_PKG}/opt/friction/bin
LIB_DIR=${BUILD}/${FRICTION_PKG}/opt/friction/lib
PLUG_DIR=${BUILD}/${FRICTION_PKG}/opt/friction/plugins

APP_DEPENDS=`(cd ${BIN_DIR} ; ldd friction | awk '{print $3}' | grep ${SDK})`

for so in ${APP_DEPENDS}; do
    cp ${so} ${LIB_DIR}/
done

for so in ${LIB_DIR}/*.so*; do
    DEPENDS=`ldd ${so} | awk '{print $3}'`
    for lib in ${DEPENDS}; do
        if [[ ${lib} == ${SDK}* ]]; then
            cp ${lib} ${LIB_DIR}/
        fi
    done
done

mkdir -p ${PLUG_DIR}/platforms
cp -a ${SDK}/plugins/platforms/libqxcb.so ${PLUG_DIR}/platforms/
cp -a ${SDK}/plugins/platforms/libqwayland-generic.so ${PLUG_DIR}/platforms/
cp -a ${SDK}/plugins/platforms/libqwayland-egl.so ${PLUG_DIR}/platforms/

mkdir -p ${PLUG_DIR}/platformthemes
cp -a ${SDK}/plugins/platformthemes/libqxdgdesktopportal.so ${PLUG_DIR}/platformthemes/

cp -a ${SDK}/plugins/audio ${PLUG_DIR}/
cp -a ${SDK}/plugins/xcbglintegrations ${PLUG_DIR}/
cp -a ${SDK}/plugins/wayland-graphics-integration-client ${PLUG_DIR}/
cp -a ${SDK}/plugins/wayland-shell-integration ${PLUG_DIR}/
cp -a ${SDK}/plugins/wayland-decoration-client ${PLUG_DIR}/

for so in ${PLUG_DIR}/*/*.so; do
    DEPENDS=`ldd ${so} | awk '{print $3}'`
    for lib in ${DEPENDS}; do
        if [[ ${lib} == ${SDK}* ]]; then
            cp ${lib} ${LIB_DIR}/
        fi
    done
done

for so in ${LIB_DIR}/*.so*; do
    DEPENDS=`ldd ${so} | awk '{print $3}'`
    for lib in ${DEPENDS}; do
        if [[ ${lib} == ${SDK}* ]]; then
            cp ${lib} ${LIB_DIR}/
        fi
    done
done

HICOLOR="
128x128
16x16
192x192
22x22
256x256
32x32
48x48
64x64
96x96
scalable
"

mkdir -p ${BUILD}/${FRICTION_PKG}/usr/bin
mkdir -p ${BUILD}/${FRICTION_PKG}/usr/share/mime/packages
mkdir -p ${BUILD}/${FRICTION_PKG}/usr/share/{applications,metainfo}

(cd ${BUILD}/${FRICTION_PKG}/usr/bin; ln -sf ../../opt/friction/bin/friction .)
(cd ${BUILD}/${FRICTION_PKG}/usr/share/mime/packages ; ln -sf ../../../../opt/friction/share/mime/packages/${APPID}.xml .)
(cd ${BUILD}/${FRICTION_PKG}/usr/share/applications; ln -sf ../../../opt/friction/share/applications/${APPID}.desktop .)
(cd ${BUILD}/${FRICTION_PKG}/usr/share/metainfo; ln -sf ../../../opt/friction/share/metainfo/${APPID}.appdata.xml .)

for icon in ${HICOLOR}; do
    ICON_SUFFIX=png
    if [ "${icon}" = "scalable" ]; then
        ICON_SUFFIX=svg
    fi
    mkdir -p ${BUILD}/${FRICTION_PKG}/usr/share/icons/hicolor/${icon}/{apps,mimetypes}
    ICON_APP_DIR=${BUILD}/${FRICTION_PKG}/usr/share/icons/hicolor/${icon}/apps
    ICON_MIME_DIR=${BUILD}/${FRICTION_PKG}/usr/share/icons/hicolor/${icon}/mimetypes
    (cd ${ICON_APP_DIR} ; ln -sf ../../../../../../opt/friction/share/icons/hicolor/${icon}/apps/${APPID}.${ICON_SUFFIX} .)
    (cd ${ICON_MIME_DIR} ; ln -sf ../../../../../../opt/friction/share/icons/hicolor/${icon}/mimetypes/application-x-${APPID}.${ICON_SUFFIX} .)
done

strip -s ${BUILD}/${FRICTION_PKG}/opt/friction/bin/friction
strip -s ${BUILD}/${FRICTION_PKG}/opt/friction/lib/*so*
strip -s ${BUILD}/${FRICTION_PKG}/opt/friction/plugins/*/*.so

echo "[Paths]" > ${BUILD}/${FRICTION_PKG}/opt/friction/bin/qt.conf
echo "Prefix = .." >> ${BUILD}/${FRICTION_PKG}/opt/friction/bin/qt.conf
echo "Plugins = plugins" >> ${BUILD}/${FRICTION_PKG}/opt/friction/bin/qt.conf

(cd ${BUILD}/${FRICTION_PKG}/opt/friction/lib ;
for so in *.so*; do
    patchelf --set-rpath '$ORIGIN' ${so}
done
)

SKIA_DIST_LIB=${DISTFILES}/skia/libskia.friction.so
PKG_SKIA_LIB64=${BUILD}/${FRICTION_PKG}/opt/friction/lib64/libskia.friction.so
PKG_SKIA_LIB=${BUILD}/${FRICTION_PKG}/opt/friction/lib/libskia.friction.so

if [ -f "${PKG_SKIA_LIB64}" ]; then
    mv ${PKG_SKIA_LIB64} ${PKG_SKIA_LIB}
    strip -s ${PKG_SKIA_LIB}
    rm -rf ${BUILD}/${FRICTION_PKG}/opt/friction/lib64
else
    if [ -f "${SKIA_DIST_LIB}" ] && [ ! -f "${PKG_SKIA_LIB}" ]; then
        cp -a ${SKIA_DIST_LIB} ${PKG_SKIA_LIB}
        strip -s ${PKG_SKIA_LIB}
    fi
fi

if [ ! -f "${PKG_SKIA_LIB}" ]; then
    echo "Missing libskia.friction.so"
    exit 1
fi

PLUGS="
platforms
audio
xcbglintegrations
wayland-graphics-integration-client
wayland-shell-integration
wayland-decoration-client
"
for pdir in ${PLUGS}; do
    for so in ${BUILD}/${FRICTION_PKG}/opt/friction/plugins/${pdir}/*.so; do
        patchelf --set-rpath '$ORIGIN/../../lib' ${so}
    done
done

# RPM
cd ${BUILD}
tar cvf ${FRICTION_PKG}.tar ${FRICTION_PKG}
if [ ! -d "${HOME}/rpmbuild/SOURCES" ]; then
    mkdir -p ${HOME}/rpmbuild/SOURCES
fi
mv ${FRICTION_PKG}.tar ${HOME}/rpmbuild/SOURCES/
cat ${BUILD}/friction/src/scripts/vfxplatform.spec | sed 's/__FRICTION_PKG_VERSION__/'${PKG_VERSION}'/g;s/__FRICTION_VERSION__/'${VERSION}'/g;s/__APPID__/'${APPID}'/g' > rpm.spec
rpmbuild -bb rpm.spec
cp -a ${HOME}/rpmbuild/RPMS/*/*.rpm ${DISTFILES}/builds/${VERSION}/

# Portable
FRICTION_PORTABLE=${FRICTION_PKG}-linux-x86_64
FRICTION_PORTABLE_DIR=${BUILD}/${FRICTION_PORTABLE}
cd ${BUILD}
rm -f ${FRICTION_PORTABLE_DIR} || true
mv ${BUILD}/${FRICTION_PKG} ${FRICTION_PORTABLE_DIR}
(cd ${FRICTION_PORTABLE_DIR} ;
rm -rf usr
mv opt/friction/* .
rm -rf opt share/doc
ln -sf bin/friction .
touch bin/portable.txt
)
cd ${BUILD}
tar cvf ${FRICTION_PORTABLE}.tar ${FRICTION_PORTABLE}
xz -9 ${FRICTION_PORTABLE}.tar
cp -a ${FRICTION_PORTABLE}.tar.xz ${DISTFILES}/builds/${VERSION}/

# AppImage
(cd ${FRICTION_PORTABLE_DIR} ;
rm -f bin/portable.txt
rm -f friction
mkdir usr
mv lib bin plugins share usr/
ln -sf usr/bin/friction AppRun
ln -sf usr/share/applications/${APPID}.desktop .
ln -sf usr/share/icons/hicolor/256x256/apps/${APPID}.png .
ln -sf usr/share/icons/hicolor/256x256/apps/${APPID}.png .DirIcon
)
tar xf ${DISTFILES}/linux/appimagetool-${APPIMAGETOOL}.tar.bz2
ARCH=x86_64 ./appimagetool/AppRun --verbose --runtime-file=${DISTFILES}/linux/runtime-x86_64-${APPIMAGERUNTIME}.bin ${FRICTION_PORTABLE}

cp -a *.AppImage ${DISTFILES}/builds/${VERSION}/

echo "FRICTION PACKAGE DONE"
