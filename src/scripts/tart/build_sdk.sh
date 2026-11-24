#!/bin/sh
set -e -x

BUILD_DIR="/tmp/friction_sdk_build"
rm -rf "${BUILD_DIR}" || true
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

ln -sf /Volumes/My\ Shared\ Files/friction/distfiles .
mkdir -p src
ln -sf /Volumes/My\ Shared\ Files/friction/src/scripts src/scripts

./src/scripts/build_mac_sdk.sh
arch -x86_64 ./src/scripts/build_mac_sdk.sh
