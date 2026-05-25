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

ICON=`pwd`/friction-icon-apple.png
ICONSET=`pwd`/friction.iconset

if [ -d "$ICONSET" ]; then
  rm -rf $ICONSET
fi
mkdir -p $ICONSET

sips -z 16 16     $ICON --out $ICONSET/icon_16x16.png
sips -z 32 32     $ICON --out $ICONSET/icon_16x16@2x.png
sips -z 32 32     $ICON --out $ICONSET/icon_32x32.png
sips -z 64 64     $ICON --out $ICONSET/icon_32x32@2x.png
sips -z 128 128   $ICON --out $ICONSET/icon_128x128.png
sips -z 256 256   $ICON --out $ICONSET/icon_128x128@2x.png
sips -z 256 256   $ICON --out $ICONSET/icon_256x256.png
sips -z 512 512   $ICON --out $ICONSET/icon_256x256@2x.png
sips -z 512 512   $ICON --out $ICONSET/icon_512x512.png
cp $ICON $ICONSET/icon_512x512@2x.png
iconutil -c icns $ICONSET
