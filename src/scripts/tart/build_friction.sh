#!/bin/sh
set -e -x

BUILD_DIR="/tmp/friction_build"
rm -rf "${BUILD_DIR}" || true
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

ln -sf /Volumes/My\ Shared\ Files/friction/src .
ln -sf /Volumes/My\ Shared\ Files/friction/.git .
ln -sf /Volumes/My\ Shared\ Files/friction/docs .
ln -sf /Volumes/My\ Shared\ Files/friction/sdk .

cp /Volumes/My\ Shared\ Files/friction/CMakeLists.txt .

UPDATE_SUBMODULES=0 ./src/scripts/build_mac_ci.sh
