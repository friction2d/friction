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

#ifndef FILLSTROKESETTINGS_H
#define FILLSTROKESETTINGS_H

#include "ui_global.h"

#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QPen>
#include <QGradient>
#include <QDebug>
#include <QTimer>
#include <QPainterPathStroker>
#include <QLabel>
#include <QHBoxLayout>

#include "Animators/coloranimator.h"
#include "Animators/paintsettingsanimator.h"
#include "Private/document.h"
#include "gradientwidgets/gradientwidget.h"
#include "paintsettingsapplier.h"
#include "Animators/brushsettingsanimator.h"
#include "Paint/brushcontexedwrapper.h"
#include "widgets/colorsettingswidget.h"
#include "widgets/qrealanimatorvalueslider.h"

class UI_EXPORT FillStrokeSettingsWidget : public QWidget
{
    Q_OBJECT
    typedef qCubicSegment1DAnimator::Action SegAction;

public:
    explicit FillStrokeSettingsWidget(Document& document,
                                      QWidget * const parent = nullptr,
                                      const bool noScroll = false,
                                      const bool fillOnly = false,
                                      const bool strokeOnly = false,
                                      bool loadState = true,
                                      bool saveState = true,
                                      bool defWheel = true,
                                      bool defMap = false,
                                      const QString &confPrefix = QString());

    void setCurrentBox(BoundingBox * const box);
    void clearAll();

private:
    void setDisplayedBrush(SimpleBrushWrapper* const brush);
    void setCurrentSettings(PaintSettingsAnimator *fillPaintSettings,
                            OutlineSettingsAnimator *strokePaintSettings);
    void updateAfterTargetChanged();
    void setCurrentPaintType(const PaintType paintType);

    void setFillValuesFromFillSettings(PaintSettingsAnimator *settings);
    void setStrokeValuesFromStrokeSettings(OutlineSettingsAnimator *settings);

    void updateColorAnimator();
    void setCurrentBrushSettings(BrushSettingsAnimator * const brushSettings);

    void emitStrokeBrushChanged(SimpleBrushWrapper* const brush);

    void emitCapStyleChanged();
    void emitJoinStyleChanged();

    void setLinearGradientAction();
    void setRadialGradientAction();

    void setStrokeBrush(BrushContexedWrapper * const brush);

    void applyBrushSpacingAction(const SegAction& action);
    void applyBrushPressureAction(const SegAction& action);
    void applyBrushWidthAction(const SegAction& action);
    void applyBrushTimeAction(const SegAction& action);

    void paintTypeSet(const PaintType type);
    void setFillTarget();
    void setStrokeTarget();

    void setGradientAction(Gradient *gradient);
    void setGradientType(const GradientType type);

    void setBevelJoinStyleAction();
    void setMiterJoinStyleAction();
    void setRoundJoinStyleAction();

    void setFlatCapStyleAction();
    void setSquareCapStyleAction();
    void setRoundCapStyleAction();

    void setGradientFillAction();
    void setBrushFillAction();
    void setFlatFillAction();
    void setNoneFillAction();

    void setColorAnimatorTarget(ColorAnimator *animator);
    void colorSettingReceived(const ColorSetting &colorSetting);

    void updateCurrentSettings();
    void applyGradient();

    void connectGradient();
    void disconnectGradient();

    void setJoinStyle(const SkPaint::Join joinStyle);
    void setCapStyle(const SkPaint::Cap capStyle);

    PaintType getCurrentPaintTypeVal();

    void setCurrentPaintTypeVal(const PaintType paintType);

    QColor getColorVal();

    void setCurrentColorVal(const QColor& color);

    Gradient *getCurrentGradientVal();

    GradientType getCurrentGradientTypeVal() const;

    void setCurrentGradientVal(Gradient *gradient);
    void setCurrentGradientTypeVal(const GradientType type);

    Document& mDocument;
    PaintSetting::Target mTarget;

    qptr<ColorAnimator> mCurrentFillColorAnimator;
    qptr<ColorAnimator> mCurrentStrokeColorAnimator;
    PaintType mCurrentFillPaintType;
    PaintType mCurrentStrokePaintType;
    QColor mCurrentFillColor;
    QColor mCurrentStrokeColor;

    GradientType mCurrentStrokeGradientType;
    GradientType mCurrentFillGradientType;

    qptr<Gradient> mCurrentStrokeGradient;
    qptr<Gradient> mCurrentFillGradient;
    SkPaint::Cap mCurrentCapStyle;
    SkPaint::Join mCurrentJoinStyle;
    qreal mCurrentStrokeWidth;

    void setNoPaintType();
    void setFlatPaintType();
    void setGradientPaintType();

    QPushButton *mFillTargetButton;
    QPushButton *mStrokeTargetButton;

    QPushButton *mFillNoneButton;
    QPushButton *mFillFlatButton;
    QPushButton *mFillGradientButton;

    QWidget *mStrokeSettingsWidget;
    QWidget *mStrokeJoinCapWidget;

    QPushButton *mBevelJoinStyleButton;
    QPushButton *mMiterJointStyleButton;
    QPushButton *mRoundJoinStyleButton;

    QPushButton *mFlatCapStyleButton;
    QPushButton *mSquareCapStyleButton;
    QPushButton *mRoundCapStyleButton;

    QrealAnimatorValueSlider *mLineWidthSpin;

    Friction::Ui::ColorSettingsWidget *mColorsSettingsWidget;

    GradientWidget *mGradientWidget;

    QPushButton *mLinearGradientButton;
    QPushButton *mRadialGradientButton;
    QWidget *mGradientTypeWidget;

    ConnContextQPtr<BoundingBox> mCurrentBox;
};

#endif // FILLSTROKESETTINGS_H
