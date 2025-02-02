#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

set(PROJECT_VERSION_MAJOR 2025)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

set(GIT_BRANCH "" CACHE STRING "Git branch")
set(GIT_COMMIT "" CACHE STRING "Git commit")
set(CUSTOM_BUILD "" CACHE STRING "Custom build")
if (NOT CUSTOM_BUILD STREQUAL "")
    add_definitions(-DCUSTOM_BUILD="${CUSTOM_BUILD}")
endif()
option(FRICTION_OFFICIAL_RELEASE "" OFF)
if (${FRICTION_OFFICIAL_RELEASE})
    add_definitions(-DPROJECT_OFFICIAL)
endif()
