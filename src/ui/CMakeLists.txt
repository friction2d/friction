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
project(frictionui LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(friction-version)
include(friction-meta)
include(friction-common)
include(friction-ffmpeg)

message("-- ffmpeg include: ${FFMPEG_INCLUDE_DIRS}")
message("-- ffmpeg libraries: ${FFMPEG_LIBRARIES_DIRS} ${FFMPEG_LIBRARIES}")
message("-- skia libraries: ${SKIA_LIBRARIES_DIRS} ${SKIA_LIBRARIES}")

add_definitions(-DUI_LIBRARY)

include_directories(
    ${FFMPEG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine/skia
)

set(
    SOURCES
    dialogs/adjustscenedialog.cpp
    dialogs/applyexpressiondialog.cpp
    dialogs/commandpalette.cpp
    dialogs/dialog.cpp
    dialogs/durationrectsettingsdialog.cpp
    dialogs/exportsvgdialog.cpp
    dialogs/markereditordialog.cpp
    dialogs/qrealpointvaluedialog.cpp
    dialogs/renderoutputwidget.cpp
    dialogs/scenesettingsdialog.cpp
    gradientwidgets/currentgradientwidget.cpp
    gradientwidgets/displayedgradientswidget.cpp
    gradientwidgets/gradientslistwidget.cpp
    gradientwidgets/gradientwidget.cpp
    misc/noshortcutaction.cpp
    misc/keyfocustarget.cpp
    optimalscrollarena/minimalscrollwidget.cpp
    optimalscrollarena/scrollarea.cpp
    optimalscrollarena/scrollvisiblepartbase.cpp
    optimalscrollarena/scrollwidget.cpp
    optimalscrollarena/scrollwidgetvisiblepart.cpp
    optimalscrollarena/singlewidget.cpp
    updater/updatechecker.cpp
    widgets/aboutwidget.cpp
    widgets/actionbutton.cpp
    widgets/alignwidget.cpp
    widgets/framescrollbar.cpp
    widgets/assetswidget.cpp
    widgets/bookmarkedcolors.cpp
    widgets/bookmarkedwidget.cpp
    widgets/buttonbase.cpp
    widgets/buttonslist.cpp
    widgets/canvassettingswidget.cpp
    widgets/canvastoolbar.cpp
    widgets/changewidthwidget.cpp
    widgets/colortoolbar.cpp
    widgets/colortoolbutton.cpp
    widgets/colorlabel.cpp
    widgets/colorsettingswidget.cpp
    widgets/colorvaluerect.cpp
    widgets/colorwidget.cpp
    widgets/colorwidgetshaders.cpp
    widgets/ecombobox.cpp
    widgets/editablecombobox.cpp
    widgets/fakemenubar.cpp
    widgets/fillstrokesettings.cpp
    widgets/flowlayout.cpp
    widgets/fontswidget.cpp
    widgets/framespinbox.cpp
    widgets/glwidget.cpp
    widgets/glwindow.cpp
    widgets/labeledslider.cpp
    widgets/markereditor.cpp
    widgets/performancesettingswidget.cpp
    widgets/presetsettingswidget.cpp
    widgets/qdoubleslider.cpp
    widgets/qrealanimatorvalueslider.cpp
    widgets/savedcolorbutton.cpp
    widgets/savedcolorswidget.cpp
    widgets/scenechooser.cpp
    widgets/settingswidget.cpp
    widgets/toolbar.cpp
    widgets/twocolumnlayout.cpp
    widgets/uilayout.cpp
    widgets/welcomedialog.cpp
    widgets/widgetstack.cpp
    widgets/widgetwrappernode.cpp
    widgets/wrappernode.cpp
)

set(
    HEADERS
    ui_global.h
    dialogs/adjustscenedialog.h
    dialogs/applyexpressiondialog.h
    dialogs/commandpalette.h
    dialogs/dialog.h
    dialogs/durationrectsettingsdialog.h
    dialogs/exportsvgdialog.h
    dialogs/markereditordialog.h
    dialogs/qrealpointvaluedialog.h
    dialogs/renderoutputwidget.h
    dialogs/scenesettingsdialog.h
    gradientwidgets/currentgradientwidget.h
    gradientwidgets/displayedgradientswidget.h
    gradientwidgets/gradientslistwidget.h
    gradientwidgets/gradientwidget.h
    misc/noshortcutaction.h
    misc/keyfocustarget.h
    optimalscrollarena/minimalscrollwidget.h
    optimalscrollarena/scrollarea.h
    optimalscrollarena/scrollvisiblepartbase.h
    optimalscrollarena/scrollwidget.h
    optimalscrollarena/scrollwidgetvisiblepart.h
    optimalscrollarena/singlewidget.h
    updater/updatechecker.h
    widgets/aboutwidget.h
    widgets/actionbutton.h
    widgets/alignwidget.h
    widgets/fillstrokesettings.h
    widgets/framescrollbar.h
    widgets/assetswidget.h
    widgets/bookmarkedcolors.h
    widgets/bookmarkedwidget.h
    widgets/buttonbase.h
    widgets/buttonslist.h
    widgets/canvassettingswidget.h
    widgets/canvastoolbar.h
    widgets/changewidthwidget.h
    widgets/colortoolbar.h
    widgets/colortoolbutton.h
    widgets/colorlabel.h
    widgets/colorsettingswidget.h
    widgets/colorvaluerect.h
    widgets/colorwidget.h
    widgets/colorwidgetshaders.h
    widgets/ecombobox.h
    widgets/editablecombobox.h
    widgets/fakemenubar.h
    widgets/flowlayout.h
    widgets/fontswidget.h
    widgets/framespinbox.h
    widgets/glwidget.h
    widgets/glwindow.h
    widgets/labeledslider.h
    widgets/markereditor.h
    widgets/performancesettingswidget.h
    widgets/presetsettingswidget.h
    widgets/qdoubleslider.h
    widgets/qrealanimatorvalueslider.h
    widgets/savedcolorbutton.h
    widgets/savedcolorswidget.h
    widgets/scenechooser.h
    widgets/settingswidget.h
    widgets/toolbar.h
    widgets/toolbutton.h
    widgets/twocolumnlayout.h
    widgets/uilayout.h
    widgets/vlabel.h
    widgets/welcomedialog.h
    widgets/widgetstack.h
    widgets/widgetwrappernode.h
    widgets/wrappernode.h
)

if(NOT WIN32)
    add_library(${PROJECT_NAME} STATIC ${HEADERS} ${SOURCES})
else()
    add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
endif()

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    VERSION
    ${PROJECT_VERSION}
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    SOVERSION
    ${PROJECT_VERSION_MAJOR}
)

target_link_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${FFMPEG_LIBRARIES_DIRS}
    ${SKIA_LIBRARIES_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    frictioncore
    ${QT_LIBRARIES}
    ${SKIA_LIBRARIES}
    ${FFMPEG_LIBRARIES}
)
