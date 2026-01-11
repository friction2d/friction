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
# See 'README.md' for more information.
#

if(WIN32)
    set(FFMPEG_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/sdk/include)
    set(FFMPEG_LIBRARIES
        avformat
        avcodec
        avutil
        swscale
        swresample)
    set(FFMPEG_LIBRARIES_DIRS ${CMAKE_SOURCE_DIR}/sdk/bin)
else()
    pkg_check_modules(AVFORMAT REQUIRED libavformat>=60)
    pkg_check_modules(AVCODEC REQUIRED libavcodec>=60)
    pkg_check_modules(AVUTIL REQUIRED libavutil>=58)
    pkg_check_modules(SWSCALE REQUIRED libswscale>=7)
    pkg_check_modules(SWRESAMPLE REQUIRED libswresample>=4)
    set(FFMPEG_INCLUDE_DIRS ${AVFORMAT_INCLUDE_DIRS})
    set(FFMPEG_LIBRARIES
        ${AVFORMAT_LIBRARIES}
        ${AVCODEC_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
        ${SWRESAMPLE_LIBRARIES})
    set(FFMPEG_LIBRARIES_DIRS ${AVFORMAT_LIBRARY_DIRS})
endif()
