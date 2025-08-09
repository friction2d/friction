#!/bin/bash
set -e -x

CWD=`pwd`
DIR=${CWD}/backup
DATE=`date +%Y%m%d%H%M`
URL=https://github.com/friction2d
BACKUP_DIR=${DIR}/friction-git-backup-${DATE}
REPOS="
friction-icon-theme
friction
skia
friction2d.github.io
friction-shader-plugins
friction-sdk
mxe
friction-svgo
friction-unit-tests
gperftools
friction-examples
sfntly
gn
"

mkdir -p ${BACKUP_DIR}
cd ${BACKUP_DIR}
for repo in ${REPOS}; do
    git clone --mirror ${URL}/${repo}.git
done
