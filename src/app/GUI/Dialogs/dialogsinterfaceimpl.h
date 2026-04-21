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

#ifndef DIALOGSINTERFACEIMPL_H
#define DIALOGSINTERFACEIMPL_H

#include "GUI/dialogsinterface.h"

class DialogsInterfaceImpl : public DialogsInterface {
    static DialogsInterfaceImpl sInstance;
public:
    stdsptr<ShaderEffectCreator> execShaderChooser(
            const QString& name, const ShaderOptions& options) const;
    void showExpressionDialog(QrealAnimator* const target) const;
    void showExpressionDialog(QStringAnimator* const target) const;
    void showApplyExpressionDialog(QrealAnimator* const target) const;
    void showDurationSettingsDialog(DurationRectangle* const target) const;
    /*bool execAnimationToPaint(const AnimationBox* const src,
                              int& firstAbsFrame, int& lastAbsFrame,
                              int& increment) const;*/
    void showSceneSettingsDialog(Canvas* const scene) const;
    void displayMessageToUser(const QString& message, const int ms) const;
    void showStatusMessage(const QString& message, const int ms) const;
};

#endif // DIALOGSINTERFACEIMPL_H
