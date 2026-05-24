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
project(friction.graphics)

option(BUILD_SKIA "Build skia" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/src/cmake")
include(friction-version)
include(friction-meta)

write_file(version.txt ${PROJECT_VERSION_FULL})

if(UNIX AND NOT APPLE)
    set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
    set(CPACK_PACKAGE_NAME ${FRICTION_NAME})
    set(CPACK_PACKAGE_DESCRIPTION ${PROJECT_SUMMARY})
    set(CPACK_PACKAGE_VENDOR ${PROJECT_COPYRIGHT})
    set(CPACK_STRIP_FILES TRUE)

    set(CPACK_RPM_SPEC_MORE_DEFINE "%define _build_id_links none")
    set(CPACK_RPM_PACKAGE_LICENSE ${PROJECT_LICENSE})
    set(CPACK_RPM_PACKAGE_SUMMARY ${PROJECT_SUMMARY})
    set(CPACK_RPM_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
    set(CPACK_RPM_PACKAGE_URL ${PROJECT_HOMEPAGE_URL})
    set(CPACK_RPM_FILE_NAME "${FRICTION_NAME}.rpm")

    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5multimedia5-plugins")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${PROJECT_COPYRIGHT})
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PROJECT_HOMEPAGE_URL})
    set(CPACK_DEBIAN_FILE_NAME "${FRICTION_NAME}.deb")

    include(CPack)
endif()

include(FetchContent)
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        0.8.0
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

if(${BUILD_SKIA})
    add_subdirectory(src/skia)
endif()
if(UNIX AND NOT APPLE)
    option(BUILD_TESTING "Don't build gperftools tests" OFF)
    add_subdirectory(src/gperftools)
endif()
add_subdirectory(src/core)
add_subdirectory(src/ui)
add_subdirectory(src/app)

if(${BUILD_SKIA})
    add_dependencies(frictioncore skialib)
endif()
