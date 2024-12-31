/*
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
*/

#ifndef THEMESUPPORT_H
#define THEMESUPPORT_H

#include "core_global.h"

#include <QPalette>
#include <QList>
#include <QSize>

#include "include/core/SkColor.h"

class CORE_EXPORT ThemeSupport
{

public:
    static const QColor getQColor(int r, int g, int b, int a = 255);
    static const QColor getThemeBaseColor(int alpha = 255);
    static SkColor getThemeBaseSkColor(int alpha = 255);
    static const QColor getThemeBaseDarkColor(int alpha = 255);
    static const QColor getThemeBaseDarkerColor(int alpha = 255);
    static const QColor getThemeAlternateColor(int alpha = 255);
    static const QColor getThemeHighlightColor(int alpha = 255);
    static const QColor getThemeHighlightDarkerColor(int alpha = 255);
    static const QColor getThemeHighlightAlternativeColor(int alpha = 255);
    static const QColor getThemeHighlightSelectedColor(int alpha = 255);
    static SkColor getThemeHighlightSkColor(int alpha = 255);
    static const QColor getThemeButtonBaseColor(int alpha = 255);
    static const QColor getThemeButtonBorderColor(int alpha = 255);
    static const QColor getThemeComboBaseColor(int alpha = 255);
    static const QColor getThemeTimelineColor(int alpha = 255);
    static const QColor getThemeRangeColor(int alpha = 255);
    static const QColor getThemeRangeSelectedColor(int alpha = 255);
    static const QColor getThemeFrameMarkerColor(int alpha = 255);
    static const QColor getThemeObjectColor(int alpha = 255);
    static const QColor getThemeColorRed(int alpha = 255);
    static const QColor getThemeColorBlue(int alpha = 255);
    static const QColor getThemeColorYellow(int alpha = 255);
    static const QColor getThemeColorPink(int alpha = 255);
    static const QColor getThemeColorGreen(int alpha = 255);
    static const QColor getThemeColorGreenDark(int alpha = 255);
    static const QColor getThemeColorOrange(int alpha = 255);
    static const QColor getThemeColorTextDisabled(int alpha = 255);
    static const QColor getThemeColorText(int alpha = 255);
    static const QPalette getDefaultPalette(const QColor &highlight = QColor());
    static const QPalette getDarkPalette(int alpha = 255);
    static const QPalette getDarkerPalette(int alpha = 255);
    static const QPalette getNotSoDarkPalette(int alpha = 255);
    static const QString getThemeStyle(int iconSize = 20);
    static void setupTheme(const int iconSize = 20);
    static const QList<QSize> getAvailableIconSizes();
    static const QSize getIconSize(const int size);
    static bool hasIconSize(const int size);
    static const QSize findClosestIconSize(int iconSize);
};

#endif // THEMESUPPORT_H
