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

#ifndef SCENESETTINGSDIALOG_H
#define SCENESETTINGSDIALOG_H

#include "ui_global.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QCheckBox>

#include "smartPointers/ememory.h"
#include "framerange.h"
#include "dialog.h"

class Document;
class Canvas;
class ColorAnimator;
class ColorAnimatorButton;

class UI_EXPORT SceneSettingsDialog : public Friction::Ui::Dialog
{
public:
    SceneSettingsDialog(Canvas * const canvas,
                        QWidget * const parent = nullptr);
    SceneSettingsDialog(const QString &defName,
                        QWidget * const parent = nullptr);
    SceneSettingsDialog(const QString &name,
                        const int width,
                        const int height,
                        const FrameRange &range,
                        const qreal fps,
                        ColorAnimator * const bg,
                        QWidget * const parent = nullptr,
                        bool isNew = false);

    int getCanvasWidth() const;
    int getCanvasHeight() const;
    QString getCanvasName() const;
    FrameRange getFrameRange() const;
    qreal getFps() const;

    void populateFpsPresets();
    void populateResPresets();

    void applySettingsToCanvas(Canvas * const canvas) const;

    static void sNewSceneDialog(Document &document, QWidget * const parent);

private:
    bool validate();
    void updateDuration(int index);

    Canvas * mTargetCanvas = nullptr;

    QVBoxLayout *mMainLayout;

    QHBoxLayout *mNameLayout;
    QLabel *mNameEditLabel;
    QLineEdit *mNameEdit;

    QHBoxLayout *mSizeLayout;
    QLabel *mWidthLabel;
    QSpinBox *mWidthSpinBox;
    QLabel *mHeightLabel;
    QSpinBox *mHeightSpinBox;

    QHBoxLayout *mFrameRangeLayout;
    QLabel *mFrameRangeLabel;
    QSpinBox *mMinFrameSpin;
    QSpinBox *mMaxFrameSpin;
    QComboBox *mTypeTime;

    QHBoxLayout *mFPSLayout;
    QLabel *mFPSLabel;
    QDoubleSpinBox *mFPSSpinBox;

    QHBoxLayout *mBgColorLayout;
    QLabel *mBgColorLabel;
    ColorAnimatorButton *mBgColorButton;

    QLabel* mErrorLabel;

    QPushButton *mOkButton;
    QPushButton *mCancelButton;
    QHBoxLayout *mButtonsLayout;

    QToolButton *mResToolButton;
    QToolButton *mFpsToolButton;

    QCheckBox *mSaveAsDefault = nullptr;

    bool mEnableResolutionPresets;
    bool mEnableResolutionPresetsAuto;
    bool mEnableFpsPresets;
    bool mEnableFpsPresetsAuto;
};

#endif // SCENESETTINGSDIALOG_H
