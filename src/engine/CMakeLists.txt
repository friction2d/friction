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

cmake_minimum_required(VERSION 3.12)

include(ExternalProject)
include(ProcessorCount)

ProcessorCount(N)

option(USE_SKIA_SYSTEM_LIBS "Use skia (third-party) system libraries on Linux" ON)
option(LINUX_DEPLOY "Linux Deploy" OFF)
option(MAC_DEPLOY "Mac Deploy" OFF)
option(SYNC_SKIA_DEPS "Sync third-party depends" ON)

if(UNIX AND NOT APPLE)
    option(USE_EGL "Use EGL" ON)
endif()

set(SKIA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/skia")
set(SKIA_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/skia")

if(APPLE)
    set(USE_SKIA_SYSTEM_LIBS OFF)
endif()

if(${USE_SKIA_SYSTEM_LIBS} AND UNIX)
    set(SKIA_ENABLE_EXTERNAL "true")
else()
    set(SKIA_ENABLE_EXTERNAL "false")
endif()

if(WIN32)
    set(GN_PATH ${SKIA_SRC}/bin/gn.exe)
    set(SKIA_ARGS_EXTRA --ide=vs)
    set(SKIA_BUILD_CMD msbuild all.sln)
    if(${SYNC_SKIA_DEPS})
        set(SKIA_UPDATE_CMD python.exe "tools\\git-sync-deps")
    else()
        set(SKIA_UPDATE_CMD :)
    endif()
    set(SKIA_ARGS "target_os=\"windows\" host_os=\"win\" current_os=\"win\" target_cpu=\"x64\" is_component_build=true")
    set(SKIA_ARGS "${SKIA_ARGS} clang_win=\"C:\\Program Files\\LLVM\" cc=\"clang-cl\" cxx=\"clang-cl\"")
    set(SKIA_ARGS "${SKIA_ARGS} extra_cflags=[\"-Wno-error\",\"/MD\",\"/O2\"]")
else()
    set(GN_PATH ${SKIA_SRC}/bin/gn)
    set(SKIA_BUILD_CMD ninja -j${N})
    if(${USE_SKIA_SYSTEM_LIBS})
        set(SKIA_UPDATE_CMD true)
    else()
        if(${SYNC_SKIA_DEPS})
            set(SKIA_UPDATE_CMD python3 "tools/git-sync-deps")
        else()
            set(SKIA_UPDATE_CMD true)
        endif()
    endif()
    set(SKIA_ARGS "ar=\"${CMAKE_AR}\" cc=\"${CMAKE_C_COMPILER}\" cxx=\"${CMAKE_CXX_COMPILER}\"")
    if(UNIX AND NOT APPLE)
        if(${USE_EGL})
            set(SKIA_ARGS "${SKIA_ARGS} skia_use_egl=true")
        endif()
        set(SKIA_ARGS "${SKIA_ARGS} is_component_build=true")
        if (${LINUX_DEPLOY})
            set(SKIA_ARGS "${SKIA_ARGS} extra_cflags=[\"-Wno-error\"]")
        else()
            set(SKIA_ARGS "${SKIA_ARGS} extra_cflags=[\"-Wno-error\", \"-Wno-psabi\"]")
        endif()
    endif()
    if(APPLE)
        set(GN_PATH ${SKIA_SRC}/bin/macOS/gn-${CMAKE_HOST_SYSTEM_PROCESSOR})
        set(SKIA_UPDATE_CMD python3 "tools/git-sync-deps")
        set(SKIA_ARGS "${SKIA_ARGS} target_cpu=\"${CMAKE_HOST_SYSTEM_PROCESSOR}\"")
        if(${MAC_DEPLOY})
            set(SKIA_ARGS "${SKIA_ARGS} extra_cflags=[\"-Wno-error\", \"-Wno-psabi\", \"-mmacosx-version-min=12.7\"]")
            set(SKIA_ARGS "${SKIA_ARGS} extra_cflags_cc=[\"-frtti\", \"-mmacosx-version-min=12.7\"]")
        else()
            set(SKIA_ARGS "${SKIA_ARGS} extra_cflags=[\"-Wno-error\", \"-Wno-psabi\"]")
            set(SKIA_ARGS "${SKIA_ARGS} extra_cflags_cc=[\"-frtti\"]")
        endif()
    endif()
endif()

set(SKIA_ARGS "${SKIA_ARGS} is_official_build=true is_debug=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_enable_pdf=false skia_enable_skottie=false skia_enable_tools=false skia_use_dng_sdk=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_expat=${SKIA_ENABLE_EXTERNAL}")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libjpeg_turbo=${SKIA_ENABLE_EXTERNAL}")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libpng=${SKIA_ENABLE_EXTERNAL}")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libwebp=${SKIA_ENABLE_EXTERNAL}")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_icu=${SKIA_ENABLE_EXTERNAL}")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_harfbuzz=${SKIA_ENABLE_EXTERNAL}")
if(WIN32 OR NOT ${USE_SKIA_SYSTEM_LIBS})
    set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_zlib=${SKIA_ENABLE_EXTERNAL}")
endif()
if(UNIX AND NOT APPLE)
    set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_freetype2=${SKIA_ENABLE_EXTERNAL}")
endif()
if(APPLE)
    set(SKIA_ARGS "${SKIA_ARGS} skia_use_libgifcodec=false")
endif()

message("-- skia args: ${SKIA_ARGS}")

ExternalProject_Add(
    Engine
    SOURCE_DIR ${SKIA_SRC}
    BINARY_DIR ${SKIA_BUILD_DIR}
    DOWNLOAD_COMMAND :
    UPDATE_COMMAND ${SKIA_UPDATE_CMD}
    PATCH_COMMAND :
    CONFIGURE_COMMAND ${GN_PATH} gen --root=${SKIA_SRC} ${SKIA_BUILD_DIR} "--args=${SKIA_ARGS}" ${SKIA_ARGS_EXTRA}
    BUILD_COMMAND ${SKIA_BUILD_CMD}
    INSTALL_COMMAND :
    USES_TERMINAL_UPDATE true
    USES_TERMINAL_CONFIGURE true
    USES_TERMINAL_BUILD true
)

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(
        FILES
        ${SKIA_BUILD_DIR}/libskia.friction.so
        DESTINATION
        ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(WIN32)
    add_custom_command(TARGET Engine POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SKIA_BUILD_DIR}/skia.dll.lib ${SKIA_BUILD_DIR}/skia.lib)
endif()
