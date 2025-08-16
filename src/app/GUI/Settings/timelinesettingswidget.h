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

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#ifndef TIMELINESETTINGSWIDGET_H
#define TIMELINESETTINGSWIDGET_H

#include "widgets/settingswidget.h"

#include <QCheckBox>

class ColorAnimatorButton;

class TimelineSettingsWidget : public SettingsWidget {
public:
    explicit TimelineSettingsWidget(QWidget *parent = nullptr);

    void applySettings();
    void updateSettings(bool restore = false);
private:
    //QCheckBox* mAlternateRowCheck = nullptr;
    //ColorAnimatorButton* mAlternateRowColor = nullptr;

    //QCheckBox* mHighlightRowCheck = nullptr;
    //ColorAnimatorButton* mHighlightRowColor = nullptr;

    ColorAnimatorButton* mObjectKeyframeColor = nullptr;
    ColorAnimatorButton* mPropertyGroupKeyframeColor = nullptr;
    ColorAnimatorButton* mPropertyKeyframeColor = nullptr;
    ColorAnimatorButton* mSelectedKeyframeColor = nullptr;
    ColorAnimatorButton* mThemeButtonBaseColor = nullptr;
    ColorAnimatorButton* mThemeButtonBorderColor = nullptr;
    ColorAnimatorButton* mThemeBaseDarkerColor = nullptr;
    ColorAnimatorButton* mThemeHighlightColor = nullptr;
    ColorAnimatorButton* mThemeBaseColor = nullptr;
    ColorAnimatorButton* mThemeAlternateColor = nullptr;
    ColorAnimatorButton* mThemeColorOrange = nullptr;
    ColorAnimatorButton* mThemeRangeSelectedColor = nullptr;
    ColorAnimatorButton* mThemeColorTextDisabled = nullptr;
    ColorAnimatorButton* mThemeColorOutputDestinationLineEdit = nullptr;

    //ColorAnimatorButton* mVisibilityRangeColor = nullptr;
    //ColorAnimatorButton* mSelectedVisibilityRangeColor = nullptr;
    //ColorAnimatorButton* mAnimationRangeColor = nullptr;
};

#endif // TIMELINESETTINGSWIDGET_H
