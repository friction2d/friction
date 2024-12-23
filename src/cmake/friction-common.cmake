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

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LINUX_DEPLOY "Linux Deploy" OFF)
option(MAC_DEPLOY "Mac Deploy" OFF)
option(WIN_DEPLOY "Windows Deploy" OFF)
option(BUILD_ENGINE "Build Engine" ON)
set(ENGINE_LIB_PATH "/mnt/skia" CACHE STRING "Path to prebuilt skia library")

if(${LINUX_DEPLOY})
    add_definitions(-DLINUX_DEPLOY)
endif()
if(${WIN_DEPLOY})
    add_definitions(-DWIN_DEPLOY)
endif()
if(${MAC_DEPLOY})
    add_definitions(-DMAC_DEPLOY)
endif()

if(NOT APPLE)
    if(UNIX AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Only Clang is supported.")
    elseif(WIN32 AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(FATAL_ERROR "Only MSVC is supported.")
    endif()
endif()

if(UNIX)
    add_compile_options(-Wall -Wextra -Wno-unused-private-field)
    if(NOT ${LINUX_DEPLOY})
        add_compile_options(-Wno-deprecated-copy-with-user-provided-copy)
    endif()
    if(APPLE)
        add_compile_options(-frtti)
    endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES "^(release|Release|RELEASE)$")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
else()
    add_definitions(-DQT_MESSAGELOGCONTEXT)
endif()

if(MSVC)
    add_definitions("/MP")
endif()

if(UNIX AND NOT APPLE)
    option(STATIC_FFMPEG "Link against static ffmpeg" OFF)
    if(${STATIC_FFMPEG})
        set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
    endif()
endif()

if(NOT WIN32)
    if ((NOT ${CMAKE_VERSION} VERSION_LESS 3.11) AND (NOT OpenGL_GL_PREFERENCE))
        set(OpenGL_GL_PREFERENCE "GLVND")
    endif()
    find_package(OpenGL REQUIRED)
endif()

find_package(PkgConfig QUIET)
find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
find_package(
    Qt${QT_VERSION_MAJOR}
    5.15.3
    COMPONENTS
    Gui
    Widgets
    OpenGL
    Multimedia
    Qml
    Xml
    #Svg
    REQUIRED
)
set(QT_LIBRARIES
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Xml
    #Qt${QT_VERSION_MAJOR}::Svg
)

if(WIN32)
    set(SKIA_LIBRARIES
        skia
        user32
        opengl32)
    add_definitions(-DSKIA_DLL)
    set(SKIA_LIBRARIES_DIRS ${CMAKE_SOURCE_DIR}/sdk/bin)
else()
    if(APPLE)
        set(SKIA_LIBRARIES skia)
    else()
        set(SKIA_LIBRARIES
            skia
            fontconfig
            ${OPENGL_LIBRARY})
        pkg_check_modules(UNWIND REQUIRED libunwind)
        set(GPERF_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/../gperftools ${UNWIND_INCLUDE_DIRS})
        set(GPERF_LIBRARIES tcmalloc_static ${UNWIND_LIBRARIES})
    endif()
    if(${BUILD_ENGINE})
        set(SKIA_LIBRARIES_DIRS ${CMAKE_CURRENT_BINARY_DIR}/../engine/skia)
    else()
        set(SKIA_LIBRARIES_DIRS ${ENGINE_LIB_PATH})
    endif()
endif()
