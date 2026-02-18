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

#include "fillstrokesettings.h"
#include "gradientwidgets/gradientwidget.h"
#include "canvas.h"
#include "widgets/qrealanimatorvalueslider.h"
#include "widgets/colorsettingswidget.h"
#include "paintsettingsapplier.h"
#include "Animators/gradient.h"
#include "Private/esettings.h"
#include "Private/document.h"
#include "actions.h"

#include "GUI/global.h"

#include <QSignalBlocker>

FillStrokeSettingsWidget::FillStrokeSettingsWidget(Document &document,
                                                   QWidget * const parent,
                                                   const bool noScroll,
                                                   const bool fillOnly,
                                                   const bool strokeOnly)
    : QWidget(parent)
    , mDocument(document)
    , mTarget(PaintSetting::FILL)
    , mCurrentFillPaintType(NOPAINT)
    , mCurrentStrokePaintType(NOPAINT)
    , mCurrentStrokeGradientType(GradientType::LINEAR)
    , mCurrentFillGradientType(GradientType::LINEAR)
    , mFillTargetButton(nullptr)
    , mStrokeTargetButton(nullptr)
    , mFillNoneButton(nullptr)
    , mFillFlatButton(nullptr)
    , mFillGradientButton(nullptr)
    , mStrokeSettingsWidget(nullptr)
    , mStrokeJoinCapWidget(nullptr)
    , mBevelJoinStyleButton(nullptr)
    , mMiterJointStyleButton(nullptr)
    , mRoundJoinStyleButton(nullptr)
    , mFlatCapStyleButton(nullptr)
    , mSquareCapStyleButton(nullptr)
    , mRoundCapStyleButton(nullptr)
    , mLineWidthSpin(nullptr)
    , mColorsSettingsWidget(nullptr)
    , mGradientWidget(nullptr)
    , mLinearGradientButton(nullptr)
    , mRadialGradientButton(nullptr)
    , mGradientTypeWidget(nullptr)
{
    setContentsMargins(0, 0, 0, 0);

    connect(&mDocument, &Document::selectedPaintSettingsChanged,
            this, &FillStrokeSettingsWidget::updateCurrentSettings);

    // main buttons
    mFillTargetButton = new QPushButton(QIcon::fromTheme("fill_flat_2"), tr("Fill"), this);
    mStrokeTargetButton = new QPushButton(QIcon::fromTheme("stroke_flat_2"), tr("Stroke"), this);
    mFillNoneButton = new QPushButton(QIcon::fromTheme("fill_none_2"), tr("None"), this);
    mFillFlatButton = new QPushButton(QIcon::fromTheme("fill_flat_2"), tr("Flat"), this);
    mFillGradientButton = new QPushButton(QIcon::fromTheme("fill_gradient_2"), tr("Gradient"), this);

    mFillTargetButton->setCheckable(true);
    mFillNoneButton->setCheckable(true);
    mFillFlatButton->setCheckable(true);
    mFillGradientButton->setCheckable(true);
    mStrokeTargetButton->setCheckable(true);

    mFillTargetButton->setFocusPolicy(Qt::NoFocus);
    mFillNoneButton->setFocusPolicy(Qt::NoFocus);
    mFillFlatButton->setFocusPolicy(Qt::NoFocus);
    mFillGradientButton->setFocusPolicy(Qt::NoFocus);
    mStrokeTargetButton->setFocusPolicy(Qt::NoFocus);

    connect(mFillTargetButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setFillTarget);
    connect(mFillNoneButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setNoneFillAction);
    connect(mFillFlatButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setFlatFillAction);
    connect(mFillGradientButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setGradientFillAction);
    connect(mStrokeTargetButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setStrokeTarget);

    // stroke width
    const auto lineWidget = new QWidget(this);
    const auto lineLayout = new QHBoxLayout(lineWidget);

    lineWidget->setContentsMargins(0, 0, 0 ,0);
    lineLayout->setMargin(0);

    const auto lineLabel = new QLabel(tr("Width"), this);

    mLineWidthSpin = new QrealAnimatorValueSlider(0, 1000, 0.5, this);
    mLineWidthSpin->setMinimumWidth(100);

    lineLayout->addWidget(lineLabel);
    lineLayout->addStretch();
    lineLayout->addWidget(mLineWidthSpin);
    lineLayout->addStretch();

    const auto actions = Actions::sInstance;
    connect(mLineWidthSpin, &QrealAnimatorValueSlider::editingStarted,
            actions, [actions]() {
        actions->strokeWidthAction(QrealAction::sMakeStart());
    });
    connect(mLineWidthSpin, &QrealAnimatorValueSlider::valueEdited,
            actions, [actions](const qreal value) {
        actions->strokeWidthAction(QrealAction::sMakeSet(value));
        eSettings::sInstance->fLastUsedStrokeWidth = value;
    });
    connect(mLineWidthSpin, &QrealAnimatorValueSlider::editingFinished,
            actions, [actions]() {
        actions->strokeWidthAction(QrealAction::sMakeFinish());
    });
    connect(mLineWidthSpin, &QrealAnimatorValueSlider::editingCanceled,
            actions, [actions]() {
        actions->strokeWidthAction(QrealAction::sMakeCancel());
    });

    // join & cap widget
    mStrokeJoinCapWidget = new QWidget(this);
    const auto mStrokeJoinCapLayout = new QVBoxLayout(mStrokeJoinCapWidget);

    mStrokeJoinCapWidget->setContentsMargins(0, 0, 0, 0);
    mStrokeJoinCapLayout->setMargin(0);

    // join
    const auto mJoinStyleWidget = new QWidget(this);
    const auto mJoinStyleLayout = new QHBoxLayout(mJoinStyleWidget);

    mJoinStyleWidget->setContentsMargins(0, 0, 0, 0);
    mJoinStyleLayout->setMargin(0);

    mBevelJoinStyleButton = new QPushButton(QIcon::fromTheme("joinBevel"),
                                            QString(), this);
    mMiterJointStyleButton = new QPushButton(QIcon::fromTheme("joinMiter"),
                                             QString(), this);
    mRoundJoinStyleButton = new QPushButton(QIcon::fromTheme("joinRound"),
                                            QString(), this);

    mBevelJoinStyleButton->setToolTip(tr("Bevel"));
    mMiterJointStyleButton->setToolTip(tr("Miter"));
    mRoundJoinStyleButton->setToolTip(tr("Round"));

    mBevelJoinStyleButton->setCheckable(true);
    mMiterJointStyleButton->setCheckable(true);
    mRoundJoinStyleButton->setCheckable(true);

    mBevelJoinStyleButton->setFocusPolicy(Qt::NoFocus);
    mMiterJointStyleButton->setFocusPolicy(Qt::NoFocus);
    mRoundJoinStyleButton->setFocusPolicy(Qt::NoFocus);

    connect(mBevelJoinStyleButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setBevelJoinStyleAction);
    connect(mMiterJointStyleButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setMiterJoinStyleAction);
    connect(mRoundJoinStyleButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setRoundJoinStyleAction);

    mJoinStyleLayout->addWidget(new QLabel(tr("Join"), this));
    mJoinStyleLayout->addStretch();
    mJoinStyleLayout->addWidget(mBevelJoinStyleButton);
    mJoinStyleLayout->addWidget(mMiterJointStyleButton);
    mJoinStyleLayout->addWidget(mRoundJoinStyleButton);

    // cap
    const auto mCapStyleWidget = new QWidget(this);
    const auto mCapStyleLayout = new QHBoxLayout(mCapStyleWidget);

    mCapStyleWidget->setContentsMargins(0, 0, 0, 0);
    mCapStyleLayout->setMargin(0);

    mFlatCapStyleButton = new QPushButton(QIcon::fromTheme("capFlat"),
                                          QString(), this);
    mSquareCapStyleButton = new QPushButton(QIcon::fromTheme("capSquare"),
                                            QString(), this);
    mRoundCapStyleButton = new QPushButton(QIcon::fromTheme("capRound"),
                                           QString(), this);

    mFlatCapStyleButton->setToolTip(tr("Flat"));
    mSquareCapStyleButton->setToolTip(tr("Square"));
    mRoundCapStyleButton->setToolTip(tr("Round"));

    mFlatCapStyleButton->setCheckable(true);
    mSquareCapStyleButton->setCheckable(true);
    mRoundCapStyleButton->setCheckable(true);

    mFlatCapStyleButton->setFocusPolicy(Qt::NoFocus);
    mSquareCapStyleButton->setFocusPolicy(Qt::NoFocus);
    mRoundCapStyleButton->setFocusPolicy(Qt::NoFocus);

    connect(mFlatCapStyleButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setFlatCapStyleAction);
    connect(mSquareCapStyleButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setSquareCapStyleAction);
    connect(mRoundCapStyleButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setRoundCapStyleAction);

    mCapStyleLayout->addWidget(new QLabel(tr("Cap"), this));
    mCapStyleLayout->addStretch();
    mCapStyleLayout->addWidget(mFlatCapStyleButton);
    mCapStyleLayout->addWidget(mSquareCapStyleButton);
    mCapStyleLayout->addWidget(mRoundCapStyleButton);

    mStrokeJoinCapLayout->addWidget(mJoinStyleWidget);
    mStrokeJoinCapLayout->addWidget(mCapStyleWidget);

    // gradient buttons
    mGradientTypeWidget = new QWidget(this);
    const auto mGradientTypeLayout = new QHBoxLayout(mGradientTypeWidget);

    mGradientTypeWidget->setContentsMargins(0, 0, 0, 0);
    mGradientTypeLayout->setMargin(0);

    mLinearGradientButton = new QPushButton(QIcon::fromTheme("fill_gradient_2"), tr("Linear"), this);
    mRadialGradientButton = new QPushButton(QIcon::fromTheme("fill_gradient_radial_2"), tr("Radial"), this);

    mLinearGradientButton->setCheckable(true);
    mRadialGradientButton->setCheckable(true);

    mLinearGradientButton->setFocusPolicy(Qt::NoFocus);
    mRadialGradientButton->setFocusPolicy(Qt::NoFocus);

    connect(mLinearGradientButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setLinearGradientAction);
    connect(mRadialGradientButton, &QPushButton::released,
            this, &FillStrokeSettingsWidget::setRadialGradientAction);

    mGradientTypeLayout->addWidget(mLinearGradientButton);
    mGradientTypeLayout->addWidget(mRadialGradientButton);

    // gradient widget
    mGradientWidget = new GradientWidget(this);

    // color settings
    mColorsSettingsWidget = new ColorSettingsWidget(this);
    connect(mColorsSettingsWidget, &ColorSettingsWidget::colorSettingSignal,
            this, &FillStrokeSettingsWidget::colorSettingReceived);

    // adjust sizes
    eSizesUI::widget.add(mBevelJoinStyleButton, [this](const int size) {
        Q_UNUSED(size)
        mBevelJoinStyleButton->setFixedHeight(eSizesUI::button);
        mMiterJointStyleButton->setFixedHeight(eSizesUI::button);
        mRoundJoinStyleButton->setFixedHeight(eSizesUI::button);
        mFlatCapStyleButton->setFixedHeight(eSizesUI::button);
        mSquareCapStyleButton->setFixedHeight(eSizesUI::button);
        mRoundCapStyleButton->setFixedHeight(eSizesUI::button);
        mFillTargetButton->setFixedHeight(eSizesUI::button);
        mStrokeTargetButton->setFixedHeight(eSizesUI::button);
        mFillNoneButton->setFixedHeight(eSizesUI::button);
        mFillFlatButton->setFixedHeight(eSizesUI::button);
        mFillGradientButton->setFixedHeight(eSizesUI::button);
        mLinearGradientButton->setFixedHeight(eSizesUI::button);
        mRadialGradientButton->setFixedHeight(eSizesUI::button);
        mLineWidthSpin->setFixedHeight(eSizesUI::button);
    });

    // layout
    mStrokeSettingsWidget = new QWidget(this);
    const auto mStrokeSettingsLayout = new QHBoxLayout(mStrokeSettingsWidget);

    mStrokeSettingsWidget->setContentsMargins(0, 0, 0, 0);
    mStrokeSettingsLayout->setMargin(0);

    mStrokeSettingsLayout->addWidget(lineWidget);
    mStrokeSettingsLayout->addWidget(mStrokeJoinCapWidget);

    const auto mTargetWidget = new QWidget(this);
    const auto mTargetLayout = new QHBoxLayout(mTargetWidget);

    mTargetWidget->setContentsMargins(0, 0, 0, 0);
    mTargetLayout->setMargin(0);

    mTargetLayout->addWidget(mFillTargetButton);
    mTargetLayout->addWidget(mStrokeTargetButton);

    const auto mColorTypeWidget = new QWidget(this);
    const auto mColorTypeLayout = new QHBoxLayout(mColorTypeWidget);

    mColorTypeWidget->setContentsMargins(0, 0, 0, 0);
    mColorTypeLayout->setMargin(0);

    mColorTypeLayout->addWidget(mFillNoneButton);
    mColorTypeLayout->addWidget(mFillFlatButton);
    mColorTypeLayout->addWidget(mFillGradientButton);

    const auto mFillAndStrokeWidget = new QWidget(this);
    mFillAndStrokeWidget->setContentsMargins(0, 0, 0, 0);

    const auto mMainLayout = new QVBoxLayout(mFillAndStrokeWidget);

    mMainLayout->addWidget(mTargetWidget);
    mMainLayout->addWidget(mColorTypeWidget);
    mMainLayout->addWidget(mGradientTypeWidget);
    mMainLayout->addWidget(mStrokeSettingsWidget);
    mMainLayout->addWidget(mGradientWidget);
    mMainLayout->addWidget(mColorsSettingsWidget);
    mMainLayout->addStretch();

    const auto mLayout = new QVBoxLayout(this);
    mLayout->setContentsMargins(0, 0, 0, 0);
    mLayout->setMargin(0);

    mFillAndStrokeWidget->setObjectName("DarkWidget");
    if (noScroll) {
        mLayout->addWidget(mFillAndStrokeWidget);
    } else {
        const auto mFillStrokeArea = new ScrollArea(this);
        mFillStrokeArea->setWidget(mFillAndStrokeWidget);
        mLayout->addWidget(mFillStrokeArea);
    }

    // defaults
    mLinearGradientButton->setChecked(true);
    mGradientTypeWidget->hide();
    setFillTarget();
    setCapStyle(SkPaint::kRound_Cap);
    setJoinStyle(SkPaint::kRound_Join);

    if (fillOnly || strokeOnly) {
        mTargetWidget->setVisible(false);
        if (fillOnly) { setFillTarget(); }
        else { setStrokeTarget(); }
    }
}

void FillStrokeSettingsWidget::setLinearGradientAction()
{
    setGradientType(GradientType::LINEAR);
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setRadialGradientAction()
{
    setGradientType(GradientType::RADIAL);
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setGradientFillAction()
{
    if (mTarget == PaintSetting::OUTLINE) { mStrokeJoinCapWidget->show(); }
    mFillGradientButton->setChecked(true);
    mFillFlatButton->setChecked(false);
    mFillNoneButton->setChecked(false);
    paintTypeSet(GRADIENTPAINT);

    const auto scene = *mDocument.fActiveScene;
    if (scene && scene->gradients().count() < 1) {
        const auto grad = scene->createNewGradient();
        grad->addColor(Qt::black);
        grad->addColor(Qt::white);
    }

    mDocument.actionFinished();
}

// remove
void FillStrokeSettingsWidget::setBrushFillAction()
{
    qWarning() << "setBrushFillAction";
}

void FillStrokeSettingsWidget::setFlatFillAction()
{
    if (mTarget == PaintSetting::OUTLINE) { mStrokeJoinCapWidget->show(); }
    mFillGradientButton->setChecked(false);
    mFillFlatButton->setChecked(true);
    mFillNoneButton->setChecked(false);
    paintTypeSet(FLATPAINT);
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setNoneFillAction()
{
    const bool isStrokeTarget = (mTarget == PaintSetting::OUTLINE);
    if (isStrokeTarget) { mStrokeJoinCapWidget->show(); }
    mFillGradientButton->setChecked(false);
    mFillFlatButton->setChecked(false);
    mFillNoneButton->setChecked(true);
    if (isStrokeTarget) {
        if (mLineWidthSpin) {
            QSignalBlocker blocker(mLineWidthSpin);
            mLineWidthSpin->setDisplayedValue(0.0);
        }
        if (const auto actions = Actions::sInstance) {
            actions->strokeWidthAction(QrealAction::sMakeStart());
            actions->strokeWidthAction(QrealAction::sMakeSet(0.0));
            actions->strokeWidthAction(QrealAction::sMakeFinish());
        }
        eSettings::sInstance->fLastUsedStrokeWidth = 0.0;
    }
    paintTypeSet(NOPAINT);
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::updateColorAnimator()
{
    switch (getCurrentPaintTypeVal()) {
    case NOPAINT:
        setColorAnimatorTarget(nullptr);
        break;
    case FLATPAINT:
        setColorAnimatorTarget(mTarget == PaintSetting::FILL ? mCurrentFillColorAnimator : mCurrentStrokeColorAnimator);
        break;
    case GRADIENTPAINT:
        setColorAnimatorTarget(mGradientWidget->getColorAnimator());
        break;
    default:;
    }
}

void FillStrokeSettingsWidget::updateAfterTargetChanged() {
    setCurrentPaintType(getCurrentPaintTypeVal());
    switch (getCurrentPaintTypeVal()) {
    case NOPAINT:
        mFillGradientButton->setChecked(false);
        mFillFlatButton->setChecked(false);
        mFillNoneButton->setChecked(true);
        break;
    case FLATPAINT:
        mFillGradientButton->setChecked(false);
        mFillFlatButton->setChecked(true);
        mFillNoneButton->setChecked(false);
        break;
    case GRADIENTPAINT:
        mFillGradientButton->setChecked(true);
        mFillFlatButton->setChecked(false);
        mFillNoneButton->setChecked(false);
        mGradientWidget->setCurrentGradient(getCurrentGradientVal());
        mLinearGradientButton->setChecked(getCurrentGradientTypeVal() == GradientType::LINEAR);
        mRadialGradientButton->setChecked(getCurrentGradientTypeVal() == GradientType::RADIAL);
        break;
    default:;
    }
}

void FillStrokeSettingsWidget::setCurrentPaintType(const PaintType paintType)
{
    switch (paintType) {
    case NOPAINT:
        setNoPaintType();
        break;
    case FLATPAINT:
        setFlatPaintType();
        break;
    case GRADIENTPAINT:
        setGradientPaintType();
        break;
    default:;
    }
}

// remove
void FillStrokeSettingsWidget::setStrokeBrush(BrushContexedWrapper * const brush)
{
    Q_UNUSED(brush)
    qWarning() << "setStrokeBrush";
}

// remove
void FillStrokeSettingsWidget::applyBrushSpacingAction(const SegAction& action)
{
    Q_UNUSED(action)
    qWarning() << "applyBrushSpacingAction";
}

// remove
void FillStrokeSettingsWidget::applyBrushPressureAction(const SegAction& action)
{
    Q_UNUSED(action)
    qWarning() << "applyBrushPressureAction";
}

// remove ???
void FillStrokeSettingsWidget::applyBrushWidthAction(const SegAction& action)
{
    Q_UNUSED(action)
    qWarning() << "applyBrushWidthAction";
    const auto scene = *mDocument.fActiveScene;
    if(scene) scene->applyStrokeBrushWidthActionToSelected(action);
    mDocument.actionFinished();
}

// remove
void FillStrokeSettingsWidget::applyBrushTimeAction(const SegAction& action)
{
    Q_UNUSED(action)
    qWarning() << "applyBrushTimeAction";
}

// remove
void FillStrokeSettingsWidget::setDisplayedBrush(SimpleBrushWrapper* const brush)
{
    Q_UNUSED(brush)
    qWarning() << "setDisplayedBrush";
}

// remove
void FillStrokeSettingsWidget::setCurrentBrushSettings(BrushSettingsAnimator * const brushSettings)
{
    Q_UNUSED(brushSettings)
}

void FillStrokeSettingsWidget::setCurrentBox(BoundingBox* const box)
{
    const auto fillSettings = box ? box->getFillSettings() : nullptr;
    const auto strokeSettings = box ? box->getStrokeSettings() : nullptr;
    setCurrentSettings(fillSettings, strokeSettings);
}

void FillStrokeSettingsWidget::updateCurrentSettings()
{
    const auto scene = *mDocument.fActiveScene;
    if (scene) {
        const auto currentBox = scene->getCurrentBox();
        setCurrentBox(currentBox);
    } else {
        setCurrentBox(nullptr);
    }
}

void FillStrokeSettingsWidget::setCurrentSettings(PaintSettingsAnimator *fillPaintSettings,
                                                  OutlineSettingsAnimator *strokePaintSettings)
{
    setFillValuesFromFillSettings(fillPaintSettings);
    setStrokeValuesFromStrokeSettings(strokePaintSettings);

    /*const bool hasFill = fillPaintSettings ? fillPaintSettings->getPaintType() != PaintType::NOPAINT : false;
    const bool hasStroke = strokePaintSettings ? strokePaintSettings->getPaintType() != PaintType::NOPAINT : false;
    if (hasStroke && !hasFill) { setStrokeTarget(); }
    else { setFillTarget(); }*/

    if (mTarget == PaintSetting::FILL) { setFillTarget(); }
    else { setStrokeTarget(); }
}

void FillStrokeSettingsWidget::clearAll()
{
    mGradientWidget->clearAll();
}

void FillStrokeSettingsWidget::paintTypeSet(const PaintType type)
{
    if (type == NOPAINT) {
        setNoPaintType();
    } else if (type == FLATPAINT) {
        setFlatPaintType();
    } else if (type == GRADIENTPAINT) {
        if (mTarget == PaintSetting::FILL ? !mCurrentFillGradient :
                !mCurrentStrokeGradient) { // ???
            mGradientWidget->setCurrentGradient(nullptr);
        }
        setGradientPaintType();
    } else {
        RuntimeThrow(tr("Invalid fill type."));
    }

    PaintType currentPaintType;
    Gradient *currentGradient;
    GradientType currentGradientType;
    if (mTarget == PaintSetting::FILL) {
        currentPaintType = mCurrentFillPaintType;
        currentGradient = mCurrentFillGradient;
        currentGradientType = mCurrentFillGradientType;
    } else {
        currentPaintType = mCurrentStrokePaintType;
        currentGradient = mCurrentStrokeGradient;
        currentGradientType = mCurrentStrokeGradientType;
    }
    PaintSettingsApplier paintSetting;
    if (currentPaintType == FLATPAINT) { // ???
//        const auto colorSetting = mColorsSettingsWidget->getColorSetting(ColorSettingType::apply,
//                                                                         ColorParameter::all);
//        paintSetting << std::make_shared<ColorPaintSetting>(
//                            mTarget, colorSetting);
    } else if (currentPaintType == GRADIENTPAINT) {
        paintSetting << std::make_shared<GradientPaintSetting>(mTarget, currentGradient);
        paintSetting << std::make_shared<GradientTypePaintSetting>(mTarget, currentGradientType);
    }
    paintSetting << std::make_shared<PaintTypePaintSetting>(mTarget, currentPaintType);
    const auto scene = *mDocument.fActiveScene;
    if (scene) { scene->applyPaintSettingToSelected(paintSetting); }
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::colorSettingReceived(const ColorSetting &colorSetting)
{
    const auto scene = *mDocument.fActiveScene;
    if (getCurrentPaintTypeVal() != PaintType::GRADIENTPAINT && scene) {
        PaintSettingsApplier paintSetting;
        paintSetting << std::make_shared<ColorPaintSetting>(mTarget, colorSetting);
        scene->applyPaintSettingToSelected(paintSetting);
    }

    if (mTarget == PaintSetting::OUTLINE &&
        getCurrentPaintTypeVal() == PaintType::FLATPAINT) {
        // store as last used stroke color
        eSettings::sInstance->fLastUsedStrokeColor = colorSetting.getColor();
    }

    if (mTarget == PaintSetting::FILL &&
        getCurrentPaintTypeVal() == PaintType::FLATPAINT) {
        // store as last used fill color
        eSettings::sInstance->fLastUsedFillColor = colorSetting.getColor();
    }
}

void FillStrokeSettingsWidget::connectGradient()
{
    connect(mGradientWidget,
            &GradientWidget::selectedColorChanged,
            mColorsSettingsWidget,
            &ColorSettingsWidget::setTarget);
    connect(mGradientWidget,
            &GradientWidget::triggered,
            this, &FillStrokeSettingsWidget::setGradientAction);
}

void FillStrokeSettingsWidget::disconnectGradient()
{
    disconnect(mGradientWidget,
               &GradientWidget::selectedColorChanged,
               mColorsSettingsWidget,
               &ColorSettingsWidget::setTarget);
    disconnect(mGradientWidget, &GradientWidget::triggered,
               this, &FillStrokeSettingsWidget::setGradientAction);
}

void FillStrokeSettingsWidget::setColorAnimatorTarget(ColorAnimator *animator)
{
    mColorsSettingsWidget->setTarget(animator);
}

void FillStrokeSettingsWidget::setJoinStyle(const SkPaint::Join joinStyle)
{
    mCurrentJoinStyle = joinStyle;
    mBevelJoinStyleButton->setChecked(joinStyle == SkPaint::kBevel_Join);
    mMiterJointStyleButton->setChecked(joinStyle == SkPaint::kMiter_Join);
    mRoundJoinStyleButton->setChecked(joinStyle == SkPaint::kRound_Join);
}

void FillStrokeSettingsWidget::setCapStyle(const SkPaint::Cap capStyle)
{
    mCurrentCapStyle = capStyle;
    mFlatCapStyleButton->setChecked(capStyle == SkPaint::kButt_Cap);
    mSquareCapStyleButton->setChecked(capStyle == SkPaint::kSquare_Cap);
    mRoundCapStyleButton->setChecked(capStyle == SkPaint::kRound_Cap);
}

PaintType FillStrokeSettingsWidget::getCurrentPaintTypeVal()
{
    if (mTarget == PaintSetting::FILL) { return mCurrentFillPaintType; }
    else { return mCurrentStrokePaintType; }
}

void FillStrokeSettingsWidget::setCurrentPaintTypeVal(const PaintType paintType)
{
    if (mTarget == PaintSetting::FILL) {
        if (!mCurrentFillColorAnimator && (mCurrentFillPaintType != paintType)) {
            eSettings::sInstance->fLastFillFlatEnabled = (paintType == PaintType::FLATPAINT);
        }
        mCurrentFillPaintType = paintType;
    } else {
        if (!mCurrentStrokeColorAnimator && (mCurrentStrokePaintType != paintType)) {
            eSettings::sInstance->fLastStrokeFlatEnabled = (paintType == PaintType::FLATPAINT);
        }
        mCurrentStrokePaintType = paintType;
    }
}

QColor FillStrokeSettingsWidget::getColorVal()
{
    if (mTarget == PaintSetting::FILL) { return mCurrentFillColor; }
    else { return mCurrentStrokeColor; }
}

void FillStrokeSettingsWidget::setCurrentColorVal(const QColor& color)
{
    if (mTarget == PaintSetting::FILL) { mCurrentFillColor = color; }
    else { mCurrentStrokeColor = color; }
}

Gradient *FillStrokeSettingsWidget::getCurrentGradientVal()
{
    if (mTarget == PaintSetting::FILL) { return mCurrentFillGradient; }
    else { return mCurrentStrokeGradient; }
}

GradientType FillStrokeSettingsWidget::getCurrentGradientTypeVal() const
{
    if (mTarget == PaintSetting::FILL) { return mCurrentFillGradientType; }
    else { return mCurrentStrokeGradientType; }
}

void FillStrokeSettingsWidget::setCurrentGradientVal(Gradient *gradient)
{
    if (mTarget == PaintSetting::FILL) { mCurrentFillGradient = gradient; }
    else { mCurrentStrokeGradient = gradient; }
}

void FillStrokeSettingsWidget::setCurrentGradientTypeVal(const GradientType type)
{
    if (mTarget == PaintSetting::FILL) { mCurrentFillGradientType = type; }
    else { mCurrentStrokeGradientType = type; }
}

void FillStrokeSettingsWidget::setFillValuesFromFillSettings(PaintSettingsAnimator *settings)
{
    if (settings) {
        mCurrentFillGradientType = settings->getGradientType();
        mCurrentFillColor = settings->getColor();
        mCurrentFillColorAnimator = settings->getColorAnimator();
        mCurrentFillGradient = settings->getGradient();
        mCurrentFillPaintType = settings->getPaintType();
    } else {
        mCurrentFillColorAnimator = nullptr;
    }
}

void FillStrokeSettingsWidget::setStrokeValuesFromStrokeSettings(OutlineSettingsAnimator *settings)
{
    if (settings) {
        mCurrentStrokeGradientType = settings->getGradientType();
        mCurrentStrokeColor = settings->getColor();
        mCurrentStrokeColorAnimator = settings->getColorAnimator();
        mCurrentStrokeGradient = settings->getGradient();
        setCurrentBrushSettings(settings->getBrushSettings());
        mCurrentStrokePaintType = settings->getPaintType();
        mCurrentStrokeWidth = settings->getCurrentStrokeWidth();
        mLineWidthSpin->setTarget(settings->getLineWidthAnimator());
        mCurrentCapStyle = settings->getCapStyle();
        mCurrentJoinStyle = settings->getJoinStyle();
        setCapStyle(mCurrentCapStyle);
        setJoinStyle(mCurrentJoinStyle);
    } else {
        setCurrentBrushSettings(nullptr);
        mCurrentStrokeColorAnimator = nullptr;
        mLineWidthSpin->setTarget(nullptr);
    }
}

void FillStrokeSettingsWidget::emitStrokeBrushChanged(SimpleBrushWrapper* const brush)
{
    const auto scene = *mDocument.fActiveScene;
    if (scene) { scene->setSelectedStrokeBrush(brush); }
}

void FillStrokeSettingsWidget::emitCapStyleChanged()
{
    const auto scene = *mDocument.fActiveScene;
    if (scene) { scene->setSelectedCapStyle(mCurrentCapStyle); }
}

void FillStrokeSettingsWidget::emitJoinStyleChanged()
{
    const auto scene = *mDocument.fActiveScene;
    if (scene) { scene->setSelectedJoinStyle(mCurrentJoinStyle); }
}

void FillStrokeSettingsWidget::applyGradient()
{
    Gradient *currentGradient;
    GradientType currentGradientType;
    if (mTarget == PaintSetting::FILL) {
        currentGradient = mCurrentFillGradient;
        currentGradientType = mCurrentFillGradientType;
    } else {
        currentGradient = mCurrentStrokeGradient;
        currentGradientType = mCurrentStrokeGradientType;
    }
    PaintSettingsApplier applier;
    applier << std::make_shared<GradientPaintSetting>(mTarget, currentGradient) <<
               std::make_shared<GradientTypePaintSetting>(mTarget, currentGradientType) <<
               std::make_shared<PaintTypePaintSetting>(mTarget, PaintType::GRADIENTPAINT);

    const auto scene = *mDocument.fActiveScene;
    if (scene) { scene->applyPaintSettingToSelected(applier); }
}

void FillStrokeSettingsWidget::setGradientAction(Gradient *gradient)
{
    setCurrentGradientVal(gradient);
    applyGradient();
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setGradientType(const GradientType type)
{
    setCurrentGradientTypeVal(type);
    mLinearGradientButton->setChecked(type == GradientType::LINEAR);
    mRadialGradientButton->setChecked(type == GradientType::RADIAL);
    applyGradient();
}

void FillStrokeSettingsWidget::setBevelJoinStyleAction()
{
    setJoinStyle(SkPaint::kBevel_Join);
    emitJoinStyleChanged();
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setMiterJoinStyleAction()
{
    setJoinStyle(SkPaint::kMiter_Join);
    emitJoinStyleChanged();
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setRoundJoinStyleAction()
{
    setJoinStyle(SkPaint::kRound_Join);
    emitJoinStyleChanged();
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setFlatCapStyleAction()
{
    setCapStyle(SkPaint::kButt_Cap);
    emitCapStyleChanged();
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setSquareCapStyleAction()
{
    setCapStyle(SkPaint::kSquare_Cap);
    emitCapStyleChanged();
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setRoundCapStyleAction()
{
    setCapStyle(SkPaint::kRound_Cap);
    emitCapStyleChanged();
    mDocument.actionFinished();
}

void FillStrokeSettingsWidget::setFillTarget()
{
    mTarget = PaintSetting::FILL;
    mFillTargetButton->setChecked(true);
    mStrokeTargetButton->setChecked(false);
    mStrokeSettingsWidget->hide();
    updateAfterTargetChanged();
    updateColorAnimator();
}

void FillStrokeSettingsWidget::setStrokeTarget()
{
    mTarget = PaintSetting::OUTLINE;
    mStrokeTargetButton->setChecked(true);
    mFillTargetButton->setChecked(false);
    mStrokeSettingsWidget->show();
    updateAfterTargetChanged();
    updateColorAnimator();
}

void FillStrokeSettingsWidget::setNoPaintType()
{
    setCurrentPaintTypeVal(NOPAINT);
    mColorsSettingsWidget->hide();
    mGradientWidget->hide();
    mGradientTypeWidget->hide();
    updateColorAnimator();
}

void FillStrokeSettingsWidget::setFlatPaintType()
{
    disconnectGradient();
    mColorsSettingsWidget->show();
    mGradientWidget->hide();
    mGradientTypeWidget->hide();
    setCurrentPaintTypeVal(FLATPAINT);
    updateColorAnimator();
    if (mTarget == PaintSetting::OUTLINE) { mStrokeJoinCapWidget->show(); }
}

void FillStrokeSettingsWidget::setGradientPaintType()
{
    connectGradient();
    if (mTarget == PaintSetting::FILL) {
        mCurrentFillPaintType = GRADIENTPAINT;
    } else {
        mCurrentStrokePaintType = GRADIENTPAINT;
    }
    if (mColorsSettingsWidget->isHidden()) { mColorsSettingsWidget->show(); }
    if (mGradientWidget->isHidden()) { mGradientWidget->show(); }
    if (mGradientTypeWidget->isHidden()) { mGradientTypeWidget->show(); }

    updateColorAnimator();

    if (mTarget == PaintSetting::OUTLINE) { mStrokeJoinCapWidget->show(); }
}
