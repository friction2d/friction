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

SDK=${SDK:-"/opt/friction"}
DISTFILES=${DISTFILES:-"/mnt"}
BUILD=${BUILD:-"${HOME}"}

REL=${REL:-1}
BRANCH=${BRANCH:-""}
COMMIT=${COMMIT:-""}
TAG=${TAG:-""}
CUSTOM=${CUSTOM:-""}
MKJOBS=${MKJOBS:-4}
ONLY_SDK=${ONLY_SDK:-0}
DOWNLOAD_SDK=${DOWNLOAD_SDK:-1}
TAR_VERSION=${TAR_VERSION:-""}
HEAD_REPO_URL=${HEAD_REPO_URL:-""}

SDK_VERSION=${SDK_VERSION:-"1.0.0"}
SDK_REV=${SDK_REV:-"r13"}
SDK_URL=${SDK_URL:-"https://github.com/friction2d/friction-sdk/releases/download/v${SDK_VERSION}"}
SDK_FILE=${SDK_FILE:-"friction-sdk-${SDK_VERSION}${SDK_REV}-linux-x86_64.tar"}
SDK_PATH="${DISTFILES}/sdk/${SDK_FILE}"

APPIMAGE_TOOLS=${APPIMAGE_TOOLS:-"friction-appimage-tools-20240401.tar.xz"}
APPIMAGETOOL_V=${APPIMAGETOOL_V:-"bfe6e0c"}
APPIMAGERUNTIME_V=${APPIMAGERUNTIME_V:-"1bb1157"}

if [ ! -d "${DISTFILES}/sdk" ]; then
    mkdir -p ${DISTFILES}/sdk
fi

if [ ! -d "${DISTFILES}/linux" ]; then
    (cd ${DISTFILES};
        wget ${SDK_URL}/${APPIMAGE_TOOLS}
        tar xvf ${APPIMAGE_TOOLS}
    )
fi

# Build/Install SDK
if [ ! -d "${SDK}" ]; then
    mkdir -p "${SDK}/lib"
    mkdir -p "${SDK}/bin"
    (cd "${SDK}"; ln -sf lib lib64)
fi

if [ -f "${SDK_PATH}.xz" ] || [ "${DOWNLOAD_SDK}" = 1 ]; then
    if [ ! -f "${SDK_PATH}.xz" ]; then
        (cd ${DISTFILES}/sdk ; wget ${SDK_URL}/${SDK_FILE}.xz )
    fi
    (cd ${SDK}/.. ; tar xf ${SDK_PATH}.xz )
else
    SDK=${SDK} DISTFILES=${DISTFILES} MKJOBS=${MKJOBS} ${BUILD}/build_vfxplatform_sdk01.sh
    SDK=${SDK} DISTFILES=${DISTFILES} MKJOBS=${MKJOBS} ${BUILD}/build_vfxplatform_sdk02.sh
    SDK=${SDK} DISTFILES=${DISTFILES} MKJOBS=${MKJOBS} ${BUILD}/build_vfxplatform_sdk03.sh
    (cd ${SDK}/.. ;
        rm -rf friction/src
        tar cvvf ${SDK_PATH} friction
        xz -9 ${SDK_PATH}
    )
fi

if [ "${ONLY_SDK}" = 1 ]; then
    exit 0
fi

# Build Friction
SDK=${SDK} \
BUILD=${BUILD} \
MKJOBS=${MKJOBS} \
REL=${REL} \
BRANCH=${BRANCH} \
COMMIT=${COMMIT} \
TAG=${TAG} \
CUSTOM=${CUSTOM} \
TAR_VERSION=${TAR_VERSION} \
HEAD_REPO_URL=${HEAD_REPO_URL} \
${BUILD}/build_vfxplatform_friction.sh

# Get Friction version
VERSION=`cat ${BUILD}/friction/build-vfxplatform/version.txt`
if [ "${REL}" != 1 ]; then
    GIT_COMMIT=`(cd ${BUILD}/friction ; git rev-parse --short=8 HEAD)`
    VERSION="${VERSION}-${GIT_COMMIT}"
fi
if [ "${TAR_VERSION}" != "" ]; then
    VERSION=${TAR_VERSION}
fi

# Package Friction
SDK=${SDK} \
DISTFILES=${DISTFILES} \
BUILD=${BUILD} \
VERSION=${VERSION} \
APPIMAGETOOL_V=${APPIMAGETOOL_V} \
APPIMAGERUNTIME_V=${APPIMAGERUNTIME_V} \
${BUILD}/build_vfxplatform_package.sh
