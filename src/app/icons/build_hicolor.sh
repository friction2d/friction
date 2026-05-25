#!/bin/sh
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

set -e

CWD=`pwd`
INKSCAPE=${INKSCAPE:-inkscape}
PNGQ=${PNGQ:-pngquant}
FORCE_GEN=${FORCE_GEN:-0}
INDEX=${INDEX:-0}
HICOLOR_DIR="${CWD}/hicolor"
HICOLOR_SVG="${HICOLOR_DIR}/scalable"
HICOLOR_SIZES="
16
17
18
20
21
22
24
25
26
28
30
32
34
36
38
40
42
44
46
48
64
96
128
192
256
"
HICOLOR_CATS="
actions
apps
categories
devices
friction
friction-tools
friction-nodes
legacy
mimetypes
places
status
"
INDEX_THEME="${HICOLOR_DIR}/index.theme"
INDEX_THEME_HEADER="
[Icon Theme]
Name=hicolor
Comment=hicolor
Example=folder
Inherits=hicolor

"
INDEX_THEME_DIRS="Directories="
INDEX_THEME_CATS=""

QRC="${CWD}/hicolor.qrc"
RCC="<RCC> \n
    <qresource prefix=\"/\"> \n
    <file alias='icons/hicolor/index.theme'>hicolor/index.theme</file> \n
"

for W in $HICOLOR_SIZES; do
    for C in $HICOLOR_CATS; do
        if [ "${C}" = "mimetypes" ]; then
            ACT="MimeTypes"
        else
            ACT="Actions"
        fi
        INDEX_THEME_DIRS="${INDEX_THEME_DIRS}${W}x${W}/${C},"
        INDEX_THEME_CATS="${INDEX_THEME_CATS}
[${W}x${W}/${C}]
Context=${ACT}
Size=${W}
Type=Fixed
"
        if [ -d "${HICOLOR_SVG}/${C}" ]; then
            ICONS=`ls ${HICOLOR_SVG}/${C}/ | sed 's#.svg##g'`
        else
            ICONS=""
        fi
        DIR="${HICOLOR_DIR}/${W}x${W}/${C}"
        if [ ! -d "${DIR}" ]; then
            mkdir -p "${DIR}"
        fi
        for ICON in $ICONS; do
            if [ -f "${HICOLOR_SVG}/${C}/${ICON}.svg" ]; then
#                RCC="${RCC}
#        <file alias='icons/hicolor/scalable/${C}/${ICON}.svg'>hicolor/scalable/${C}/${ICON}.svg</file>\n
#"
                if [ ! -f "${DIR}/${ICON}.png" ] || [ "${FORCE_GEN}" = 1 ]; then
                    echo "${DIR}/${ICON}.png"
                    $INKSCAPE \
                    --export-background-opacity=0 \
                    --export-width=${W} \
                    --export-height=${W} \
                    --export-type=png \
                    --export-filename="${DIR}/${ICON}.png" \
                    "${HICOLOR_SVG}/${C}/${ICON}.svg"
                    #$PNGQ "${DIR}/${ICON}.png"
                fi
            fi
        done
        PNGS=`ls ${DIR}`
        for PNG in $PNGS; do
            RCC="${RCC}
        <file alias='icons/hicolor/${W}x${W}/${C}/${PNG}'>hicolor/${W}x${W}/${C}/${PNG}</file>\n
"
        done
    done
done

RCC="${RCC}
    </qresource>\n
</RCC>\n
"

if [ "${INDEX}" = 1 ] || [ ! -f "${INDEX_THEME}" ]; then
    echo "${INDEX_THEME_HEADER}" > "${INDEX_THEME}"
    echo "${INDEX_THEME_DIRS}" >> "${INDEX_THEME}"
    echo "${INDEX_THEME_CATS}" >> "${INDEX_THEME}"
fi

echo ${RCC} > ${QRC}
echo "OK!"
