#!/bin/bash
#
# Friction - https://friction.graphics
#
# Copyright (c) Friction contributors
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
REL=${REL:-0}
BRANCH=${BRANCH:-""}
COMMIT=${COMMIT:-""}
TAG=${TAG:-""}
CUSTOM=${CUSTOM:-""}
MKJOBS=${MKJOBS:-32}
ONLY_SDK=${ONLY_SDK:-0}
VFX_BUILD=${VFX_BUILD:-1}
DOWNLOAD_SDK=${DOWNLOAD_SDK:-0}
SDK_VERSION="20240609"
TAR_VERSION=${TAR_VERSION:-""}

DOCKER="docker run"
DOCKER="${DOCKER} -e REL=${REL} -e MKJOBS=${MKJOBS} -e TAR_VERSION=${TAR_VERSION} -e SDK_VERSION=${SDK_VERSION} -e ONLY_SDK=${ONLY_SDK} -e DOWNLOAD_SDK=${DOWNLOAD_SDK} -e BRANCH=${BRANCH} -e COMMIT=${COMMIT} -e TAG=${TAG} -e CUSTOM=${CUSTOM}"
DOCKER="${DOCKER} -t --mount type=bind,source=${CWD}/distfiles,target=/mnt"

if [ ! -d "${CWD}/distfiles" ]; then
    mkdir -p ${CWD}/distfiles
    mkdir -p ${CWD}/distfiles/builds || true
    mkdir -p ${CWD}/distfiles/sdk || true
fi

if [ "${VFX_BUILD}" = 1 ]; then
    (cd src/scripts; docker build -t friction-vfxplatform -f Dockerfile.vfxplatform .)
fi

${DOCKER} friction-vfxplatform
