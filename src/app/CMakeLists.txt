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

cmake_minimum_required(VERSION 3.12)
project(friction LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(friction-version)
include(friction-meta)
include(friction-common)
#include(friction-quazip)
include(friction-ffmpeg)

add_definitions(-DPROJECT_VERSION="${PROJECT_VERSION}")

if(${LINUX_DEPLOY})
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

option(USE_SKIA_SYSTEM_LIBS "Use skia (third-party) system libraries on Linux" ON)
if(APPLE)
    set(USE_SKIA_SYSTEM_LIBS OFF)
endif()

if(${USE_SKIA_SYSTEM_LIBS} AND UNIX)
    pkg_check_modules(EXPAT REQUIRED expat)
    pkg_check_modules(FREETYPE REQUIRED freetype2)
    pkg_check_modules(JPEG REQUIRED libjpeg)
    pkg_check_modules(PNG REQUIRED libpng)
    pkg_check_modules(WEBP REQUIRED libwebp)
    pkg_check_modules(WEBPMUX REQUIRED libwebpmux)
    pkg_check_modules(WEBPDEMUX REQUIRED libwebpdemux)
    pkg_check_modules(ZLIB REQUIRED zlib)
    pkg_check_modules(ICU REQUIRED icu-i18n)
    pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
else()
    add_definitions(-DFRICTION_BUNDLE_SKIA_BUNDLE)
endif()

if(APPLE)
    set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/icons/${PROJECT_NAME}.icns)
    set(MACOSX_BUNDLE_BUNDLE_NAME "Friction")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    set(MACOSX_BUNDLE_COPYRIGHT ${PROJECT_COPYRIGHT})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${PROJECT_IDENTIFIER})
    set(MACOSX_BUNDLE_ICON_FILE ${PROJECT_NAME})
    set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
endif()

if(WIN32)
    set(QSCINTILLA_LIBRARIES_DIRS ${CMAKE_SOURCE_DIR}/sdk/bin)
    set(QSCINTILLA_LIBRARIES qscintilla2_qt6)
    set(QSCINTILLA_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/sdk/include)
    add_definitions(-DQSCINTILLA_DLL)
else()
    set(QSCINTILLA_LIBRARIES_DIRS "/usr/lib" CACHE STRING "qscintilla library path")
    set(QSCINTILLA_LIBRARIES "qscintilla2_qt6" CACHE STRING "qscintilla library")
    set(QSCINTILLA_INCLUDE_DIRS "/usr/include/x86_64-linux-gnu/qt6" CACHE STRING "qscintilla includes path")
endif()

message("-- qscintilla include: ${QSCINTILLA_INCLUDE_DIRS}")
message("-- qscintilla libraries: ${QSCINTILLA_LIBRARIES_DIRS} ${QSCINTILLA_LIBRARIES}")

if(WIN32)
    set(WINDOWS_ICON_FILE ${FRICTION_NAME}.ico)
    set(ICON_FILE ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${FRICTION_NAME}.ico)
    set(GUI_TYPE WIN32)
    set(PRODUCT_COMMENTS "${FRICTION_DISPLAY_NAME}")
    set(PRODUCT_COMPANY_NAME ${FRICTION_DISPLAY_NAME})
    set(PRODUCT_FILE_DESCRIPTION "${FRICTION_DISPLAY_NAME}")
    set(PRODUCT_VERSION "${PROJECT_VERSION}")
    set(PRODUCT_INTERNAL_NAME ${FRICTION_DISPLAY_NAME})
    set(PRODUCT_COMPANY_COPYRIGHT ${PROJECT_COPYRIGHT})
    set(PRODUCT_ORIGINAL_FILENAME "${FRICTION_NAME}.exe")
    set(PRODUCT_BUNDLE ${FRICTION_DISPLAY_NAME})
    set(WINDOWS_RESOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${FRICTION_NAME}.rc)
    configure_file(${FRICTION_NAME}.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${FRICTION_NAME}.rc
        @ONLY
    )
    configure_file(${FRICTION_NAME}.iss
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${FRICTION_NAME}.iss
        @ONLY
    )
    configure_file(icons/${FRICTION_NAME}.ico
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${FRICTION_NAME}.ico
        COPYONLY
    )
endif()

include_directories(
    #${QUAZIP_INCLUDE_DIRS}
    ${QSCINTILLA_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine/skia
)

if(${USE_SKIA_SYSTEM_LIBS} AND UNIX)
    include_directories(
        ${EXPAT_INCLUDE_DIRS}
        ${FREETYPE_INCLUDE_DIRS}
        ${JPEG_INCLUDE_DIRS}
        ${PNG_INCLUDE_DIRS}
        ${WEBP_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS}
        ${ICU_INCLUDE_DIRS}
        ${HARFBUZZ_INCLUDE_DIRS}
    )
endif()

if(UNIX AND NOT APPLE)
    include_directories(${GPERF_INCLUDE_DIRS})
    configure_file(
        app.desktop.in
        ${CMAKE_BINARY_DIR}/${PROJECT_FREEDESKTOP}.desktop
        @ONLY
    )
    configure_file(
        meta.xml.in
        ${CMAKE_BINARY_DIR}/meta.xml
        @ONLY
    )
    configure_file(
        appdata.xml.in
        ${CMAKE_BINARY_DIR}/appdata.xml
        @ONLY
    )
endif()

set(
    SOURCES
    main.cpp
    GUI/BoxesList/boxscroller.cpp
    GUI/Dialogs/dialogsinterfaceimpl.cpp
    GUI/Expressions/expressiondialog.cpp
    GUI/Expressions/expressioneditor.cpp
    GUI/Expressions/expressionhighlighter.cpp
    GUI/RenderWidgets/outputsettingsdialog.cpp
    GUI/RenderWidgets/rendersettingsdialog.cpp
    GUI/RenderWidgets/rendersettingsdisplaywidget.cpp
    GUI/Settings/generalsettingswidget.cpp
    GUI/Settings/settingsdialog.cpp
    GUI/Settings/timelinesettingswidget.cpp
    GUI/Settings/pluginssettingswidget.cpp
    GUI/canvasbasewrappernode.cpp
    GUI/canvaswindowevents.cpp
    GUI/canvaswrappernode.cpp
    GUI/ekeyfilter.cpp
    GUI/ewidgetsimpl.cpp
    GUI/layouthandler.cpp
    GUI/timelinebasewrappernode.cpp
    GUI/timelinedockwidget.cpp
    GUI/timelinehighlightwidget.cpp
    GUI/timelinewidget.cpp
    GUI/timelinewrappernode.cpp
    GUI/window.cpp
    effectsloader.cpp
    eimporters.cpp
    evfileio.cpp
    renderhandler.cpp
    GUI/BoxesList/boxsinglewidget.cpp
    GUI/BoxesList/boxscrollwidget.cpp
    GUI/BoxesList/boolpropertywidget.cpp
    memorychecker.cpp
    memoryhandler.cpp
    GUI/RenderWidgets/renderwidget.cpp
    GUI/RenderWidgets/renderinstancewidget.cpp
    GUI/RenderWidgets/closablecontainer.cpp
    GUI/BoxesList/boxtargetwidget.cpp
    GUI/BoxesList/boxscrollarea.cpp
    GUI/RenderWidgets/outputsettingsprofilesdialog.cpp
    GUI/RenderWidgets/outputsettingsdisplaywidget.cpp
    GUI/animationdockwidget.cpp
    GUI/canvaswindow.cpp
    GUI/graphboxeslist.cpp
    GUI/keysview.cpp
    GUI/mainwindow.cpp
    GUI/toolbox.cpp
    GUI/extraactions.cpp
    GUI/effectactions.cpp
)

set(
    HEADERS
    GUI/BoxesList/boxscroller.h
    GUI/Dialogs/dialogsinterfaceimpl.h
    GUI/Expressions/expressiondialog.h
    GUI/Expressions/expressioneditor.h
    GUI/Expressions/expressionhighlighter.h
    GUI/RenderWidgets/outputsettingsdialog.h
    GUI/RenderWidgets/rendersettingsdialog.h
    GUI/RenderWidgets/rendersettingsdisplaywidget.h
    GUI/Settings/generalsettingswidget.h
    GUI/Settings/settingsdialog.h
    GUI/Settings/timelinesettingswidget.h
    GUI/Settings/pluginssettingswidget.h
    GUI/canvasbasewrappernode.h
    GUI/canvaswrappernode.h
    GUI/ekeyfilter.h
    GUI/ewidgetsimpl.h
    GUI/layouthandler.h
    GUI/timelinebasewrappernode.h
    GUI/timelinedockwidget.h
    GUI/timelinehighlightwidget.h
    GUI/timelinewidget.h
    GUI/timelinewrappernode.h
    effectsloader.h
    eimporters.h
    renderhandler.h
    GUI/BoxesList/boxsinglewidget.h
    GUI/BoxesList/boxscrollwidget.h
    GUI/BoxesList/boolpropertywidget.h
    memorychecker.h
    memoryhandler.h
    GUI/RenderWidgets/renderwidget.h
    GUI/RenderWidgets/renderinstancewidget.h
    GUI/RenderWidgets/closablecontainer.h
    GUI/BoxesList/boxtargetwidget.h
    GUI/BoxesList/boxscrollarea.h
    GUI/RenderWidgets/outputsettingsprofilesdialog.h
    GUI/RenderWidgets/outputsettingsdisplaywidget.h
    GUI/animationdockwidget.h
    GUI/canvaswindow.h
    GUI/keysview.h
    GUI/mainwindow.h
    GUI/window.h
)

set(
    RESOURCES
    resources.qrc
    icons/hicolor.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/easing.qrc
)
if(${LINUX_DEPLOY})
    list(APPEND RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../../docs/docs_lin.qrc)
elseif(${MAC_DEPLOY})
    list(APPEND RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../../docs/docs_mac.qrc)
elseif(${WIN_DEPLOY})
    list(APPEND RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../../docs/docs_win.qrc)
else()
    list(APPEND RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../../docs/docs.qrc)
endif()

add_executable(
    ${PROJECT_NAME}
    ${ICON_FILE}
    ${HEADERS}
    ${SOURCES}
    ${RESOURCES}
    ${WINDOWS_RESOURCE_FILE}
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${FFMPEG_LIBRARIES_DIRS}
    ${QSCINTILLA_LIBRARIES_DIRS}
    #${QUAZIP_LIBRARIES_DIRS}
    ${SKIA_LIBRARIES_DIRS}
)

if(${USE_SKIA_SYSTEM_LIBS} AND UNIX)
    target_link_directories(
        ${PROJECT_NAME}
        PRIVATE
        ${EXPAT_LIBRARIES_DIRS}
        ${FREETYPE_LIBRARIES_DIRS}
        ${JPEG_LIBRARIES_DIRS}
        ${PNG_LIBRARIES_DIRS}
        ${WEBP_LIBRARIES_DIRS}
        ${ZLIB_LIBRARIES_DIRS}
        ${ICU_LIBRARIES_DIRS}
        ${HARFBUZZ_LIBRARIES_DIRS}
    )
endif()

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ${PROJECT_NAME}core
    ${PROJECT_NAME}ui
    ${QT_LIBRARIES}
    #${QUAZIP_LIBRARIES}
    ${QSCINTILLA_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    ${SKIA_LIBRARIES}
)

if(${USE_SKIA_SYSTEM_LIBS} AND UNIX)
    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ${EXPAT_LIBRARIES}
        ${FREETYPE_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${PNG_LIBRARIES}
        ${WEBP_LIBRARIES}
        ${WEBPMUX_LIBRARIES}
        ${WEBPDEMUX_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${ICU_LIBRARIES}
        ${HARFBUZZ_LIBRARIES}
    )
endif()

if(APPLE)
    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework CoreServices"
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ${GPERF_LIBRARIES}
    )
    include(GNUInstallDirs)
    install(
        TARGETS
        ${PROJECT_NAME}
        DESTINATION
        ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES
        ${CMAKE_BINARY_DIR}/${PROJECT_FREEDESKTOP}.desktop
        DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/applications
    )
    install(
        FILES
        ${CMAKE_BINARY_DIR}/appdata.xml
        DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
        RENAME
        ${PROJECT_FREEDESKTOP}.appdata.xml
    )
    install(
        FILES
        ${CMAKE_BINARY_DIR}/meta.xml
        DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/mime/packages
        RENAME
        ${PROJECT_FREEDESKTOP}.xml
    )
    set(HICOLOR_DIRS "16x16;22x22;32x32;48x48;64x64;96x96;128x128;192x192;256x256")
    foreach(HICOLOR_DIR ${HICOLOR_DIRS})
        install(
            FILES
            icons/hicolor/${HICOLOR_DIR}/apps/${PROJECT_FREEDESKTOP}.png
            DESTINATION
            ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${HICOLOR_DIR}/apps
            RENAME
            ${PROJECT_FREEDESKTOP}.png
        )
        install(
            FILES
            icons/hicolor/${HICOLOR_DIR}/mimetypes/application-x-${PROJECT_FREEDESKTOP}.png
            DESTINATION
            ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${HICOLOR_DIR}/mimetypes
            RENAME
            application-x-${PROJECT_FREEDESKTOP}.png
        )
    endforeach()
    install(
        FILES
        icons/hicolor/scalable/apps/${PROJECT_FREEDESKTOP}.svg
        DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
        RENAME
        ${PROJECT_FREEDESKTOP}.svg
    )
    install(
        FILES
        icons/hicolor/scalable/mimetypes/application-x-${PROJECT_FREEDESKTOP}.svg
        DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/mimetypes
        RENAME
        application-x-${PROJECT_FREEDESKTOP}.svg
    )
    install(
        FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE.md
        ${CMAKE_CURRENT_SOURCE_DIR}/../../README.md
        DESTINATION
        ${CMAKE_INSTALL_DOCDIR}-${PROJECT_VERSION}
    )
endif()
