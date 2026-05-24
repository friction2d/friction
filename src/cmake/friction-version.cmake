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

# Core version is read from semver.yaml at the project root.
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../semver.yaml" _semver_content)
string(REGEX MATCH "major:[ \t]*([0-9]+)" _ "${_semver_content}")
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "minor:[ \t]*([0-9]+)" _ "${_semver_content}")
set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "patch:[ \t]*([0-9]+)" _ "${_semver_content}")
set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_1})
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# Build metadata inputs — override via cmake -D flags or let them auto-detect.
set(GIT_COMMIT "" CACHE STRING "Git commit SHA (short, 8 chars)")
set(GIT_COMMIT_COUNT "" CACHE STRING "Git commit count (total commits reachable from HEAD)")
set(GIT_BRANCH "" CACHE STRING "Git branch name")
set(GHA_RUN_NUMBER "0" CACHE STRING "GitHub Actions run number")
set(BUILD_ORIGIN "local" CACHE STRING "Build origin: 'local' or 'ci'")

set(_dirty_flag "")

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../.git")
    if(NOT GIT_COMMIT)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../.."
            OUTPUT_VARIABLE _git_sha
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        set(GIT_COMMIT "${_git_sha}" CACHE STRING "Git commit SHA (short, 8 chars)" FORCE)
    endif()
    if(NOT GIT_COMMIT_COUNT)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../.."
            OUTPUT_VARIABLE _git_count
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        set(GIT_COMMIT_COUNT "${_git_count}" CACHE STRING "Git commit count" FORCE)
    endif()
    if(NOT GIT_BRANCH)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../.."
            OUTPUT_VARIABLE _git_branch
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        set(GIT_BRANCH "${_git_branch}" CACHE STRING "Git branch name" FORCE)
    endif()
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../.."
        OUTPUT_VARIABLE _git_dirty_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_git_dirty_output)
        set(_dirty_flag "[dirty]")
    endif()
endif()

if(NOT GIT_COMMIT_COUNT)
    set(GIT_COMMIT_COUNT "0")
endif()
if(NOT GIT_COMMIT)
    set(GIT_COMMIT "unknown")
endif()
if(NOT GIT_BRANCH)
    set(GIT_BRANCH "unknown")
endif()

# When git reports detached HEAD (e.g. CI checkout), use GitHub-provided ref name.
if(GIT_BRANCH STREQUAL "HEAD")
    if(DEFINED ENV{GITHUB_HEAD_REF} AND NOT "$ENV{GITHUB_HEAD_REF}" STREQUAL "")
        set(GIT_BRANCH "$ENV{GITHUB_HEAD_REF}")
    elseif(DEFINED ENV{GITHUB_REF_NAME} AND NOT "$ENV{GITHUB_REF_NAME}" STREQUAL "")
        set(GIT_BRANCH "$ENV{GITHUB_REF_NAME}")
    endif()
endif()

# Fall back to environment variables for values not passed via -D flags.
# This handles pre-built Docker images that invoke cmake without these flags.
if(GHA_RUN_NUMBER STREQUAL "0" AND DEFINED ENV{GHA_RUN_NUMBER} AND NOT "$ENV{GHA_RUN_NUMBER}" STREQUAL "")
    set(GHA_RUN_NUMBER "$ENV{GHA_RUN_NUMBER}")
endif()
if(BUILD_ORIGIN STREQUAL "local" AND DEFINED ENV{BUILD_ORIGIN} AND NOT "$ENV{BUILD_ORIGIN}" STREQUAL "")
    set(BUILD_ORIGIN "$ENV{BUILD_ORIGIN}")
endif()

# Replace '/' in branch names (e.g. feature/foo) with '-' for metadata compatibility.
string(REPLACE "/" "-" _branch_safe "${GIT_BRANCH}")

set(PROJECT_BUILD_METADATA "${GIT_COMMIT_COUNT}.${GHA_RUN_NUMBER}.${GIT_COMMIT}.${_branch_safe}${_dirty_flag}.${BUILD_ORIGIN}")
set(PROJECT_VERSION_FULL "${PROJECT_VERSION}+${PROJECT_BUILD_METADATA}")

message("-- friction version: ${PROJECT_VERSION_FULL}")
