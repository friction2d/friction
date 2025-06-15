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

#include "colorsettingswidget.h"
#include "colorhelpers.h"
#include "widgets/colorlabel.h"
#include "GUI/global.h"
#include "widgets/savedcolorswidget.h"
#include "appsupport.h"
#include "Private/esettings.h"
#include "Private/document.h"
#include "Private/scene.h"

#include <QResizeEvent>
#include <QWindow>
#include <QMenu>
#include <QShortcut>

void ColorSettingsWidget::updateWidgetTargets()
{
    mUpdateConnections.clear();

    rSpin->setTarget(nullptr);
    gSpin->setTarget(nullptr);
    bSpin->setTarget(nullptr);

    hSpin->setTarget(nullptr);
    hsvSSpin->setTarget(nullptr);
    vSpin->setTarget(nullptr);

    hslSSpin->setTarget(nullptr);
    lSpin->setTarget(nullptr);

    aSpin->setTarget(nullptr);

    if (mTarget) {
        auto& conn = mUpdateConnections;

        const auto anim1 = mTarget->getVal1Animator();
        const auto anim2 = mTarget->getVal2Animator();
        const auto anim3 = mTarget->getVal3Animator();

        const auto colorMode = mTarget->getColorMode();
        mColorModeCombo->setCurrentIndex(static_cast<int>(colorMode));
        if (!mAlphaHidden) { aSpin->setTarget(mTarget->getAlphaAnimator()); }
        if (mTarget->getColorMode() == ColorMode::rgb) {
            rSpin->setTarget(anim1);
            gSpin->setTarget(anim2);
            bSpin->setTarget(anim3);
            updateValuesFromRGBSpins();
            conn << connect(mTarget, &ColorAnimator::colorChanged,
                            this, &ColorSettingsWidget::updateValuesFromRGBSpins);
        } else if (mTarget->getColorMode() == ColorMode::hsv) {
            hSpin->setTarget(anim1);
            hsvSSpin->setTarget(anim2);
            vSpin->setTarget(anim3);
            updateValuesFromHSVSpins();
            conn << connect(mTarget, &ColorAnimator::colorChanged,
                            this, &ColorSettingsWidget::updateValuesFromHSVSpins);
        } else if (mTarget->getColorMode() == ColorMode::hsl) {
            hSpin->setTarget(anim1);
            hslSSpin->setTarget(anim2);
            lSpin->setTarget(anim3);
            updateValuesFromHSLSpins();
            conn << connect(mTarget, &ColorAnimator::colorChanged,
                            this, &ColorSettingsWidget::updateValuesFromHSLSpins);
        }
        updateAlphaFromSpin();
        const auto currColor = getCurrentQColor();
        mHexEdit->setText(currColor.name(QColor::HexArgb));
    }
}

void ColorSettingsWidget::setTarget(ColorAnimator * const target) {
    auto& conn = mTarget.assign(target);

    updateWidgetTargets();
    if(target) {
        conn << connect(target, &ColorAnimator::colorModeChanged,
                        this, &ColorSettingsWidget::updateWidgetTargets);
        conn << connect(target, &ColorAnimator::colorChanged,
                        mBookmarkedColors, &SavedColorsWidget::setColor);
        conn << connect(target, &ColorAnimator::colorChanged,
                        this, [this]() {
            if (isHidden()) { updateWidgetTargets(); }
        });
        mBookmarkedColors->setColor(getCurrentQColor());

    }
}

ColorSetting ColorSettingsWidget::getColorSetting(
        const ColorSettingType type,
        const ColorParameter parameter) const {
    const int tabId = mTabWidget->currentIndex();
    qreal alphaVal = 1;
    if(!mAlphaHidden) alphaVal = aSpin->value();
    if(tabId == 0) {
        return ColorSetting(
                    ColorMode::rgb, parameter,
                    rSpin->value(),
                    gSpin->value(),
                    bSpin->value(),
                    alphaVal,
                    type);
    } else if(tabId == 1) {
        return ColorSetting(
                    ColorMode::hsv, parameter,
                    hSpin->value(),
                    hsvSSpin->value(),
                    vSpin->value(),
                    alphaVal,
                    type);
    } else { //if(tabId == 2) {
        return ColorSetting(
                    ColorMode::hsl, parameter,
                    hSpin->value(),
                    hslSSpin->value(),
                    lSpin->value(),
                    alphaVal,
                    type);
    }
}

void ColorSettingsWidget::setColorModeVisible(const bool &visible)
{
    mColorModeLabel->setVisible(visible);
    mColorModeCombo->setVisible(visible);
}

void ColorSettingsWidget::setCurrentTab(const int &index)
{
    if (index >= 0 && index < mTabWidget->count()) {
        mTabWidget->setCurrentIndex(index);
        moveAlphaWidgetToTab(index);
    }
}

void ColorSettingsWidget::showEvent(QShowEvent *e)
{
    updateWidgetTargets();
    QWidget::showEvent(e);
}

void ColorSettingsWidget::emitColorChangedSignal() {
    const auto colorSetting = getColorSetting(ColorSettingType::change,
                                              mLastTriggered);
    mBookmarkedColors->setColor(colorSetting.getColor());
    emit colorSettingSignal(colorSetting);
}

void ColorSettingsWidget::emitEditingFinishedSignal() {
    const auto colorSetting = getColorSetting(ColorSettingType::finish,
                                              mLastTriggered);
    mLastTriggered = ColorParameter::none;
    emit colorSettingSignal(colorSetting);
}

void ColorSettingsWidget::emitEditingStartedSignal() {
    const auto colorSetting = getColorSetting(ColorSettingType::start,
                                              mLastTriggered);
    emit colorSettingSignal(colorSetting);
}

void ColorSettingsWidget::emitEditingStartedRed() {
    mLastTriggered = ColorParameter::red;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitEditingStartedGreen() {
    mLastTriggered = ColorParameter::green;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitEditingStartedBlue() {
    mLastTriggered = ColorParameter::blue;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitEditingStartedHue() {
    mLastTriggered = ColorParameter::hue;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitEditingStartedHSVSaturation() {
    mLastTriggered = ColorParameter::hsvSaturation;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitEditingStartedValue() {
    mLastTriggered = ColorParameter::value;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitEditingStartedHSLSaturation() {
    mLastTriggered = ColorParameter::hslSaturation;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitEditingStartedLightness() {
    mLastTriggered = ColorParameter::lightness;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitEditingStartedAlpha() {
    mLastTriggered = ColorParameter::alpha;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitStartFullColorChangedSignal() {
    mLastTriggered = ColorParameter::all;
    emitEditingStartedSignal();
}

void ColorSettingsWidget::emitFinishFullColorChangedSignal() {
    emitColorChangedSignal();
    emitEditingFinishedSignal();
}

void ColorSettingsWidget::moveAlphaWidgetToTab(const int tabId) {
    if(hLayout->parent()) ((QLayout*)hLayout->parent())->removeItem(hLayout);
    if(tabId == 1) {
        mHSVLayout->insertLayout(0, hLayout);
    } else if(tabId == 2) {
        mHSLLayout->insertLayout(0, hLayout);
    }/* else if(tabId == 3) {
        mWheelLayout->addLayout(hLayout);
    }*/
    if(!mAlphaHidden) {
        ((QLayout*)aLayout->parent())->removeItem(aLayout);
        if(tabId == 0) {
            mRGBLayout->addLayout(aLayout);
        } else if(tabId == 1) {
            mHSVLayout->addLayout(aLayout);
        } else if(tabId == 2) {
            mHSLLayout->addLayout(aLayout);
        }/* else if(tabId == 3) {
            mWheelLayout->addLayout(aLayout);
        }*/
    }
    ((QLayout*)hexLayout->parent())->removeItem(hexLayout);
    if(tabId == 0) {
        mRGBLayout->addLayout(hexLayout);
    } else if(tabId == 1) {
        mHSVLayout->addLayout(hexLayout);
    } else if(tabId == 2) {
        mHSLLayout->addLayout(hexLayout);
    }/* else if(tabId == 3) {
        mWheelLayout->addLayout(hexLayout);
    }*/
    /*for(int i=0;i < mTabWidget->count();i++)
        if(i!=tabId)
            mTabWidget->widget(i)->setSizePolicy(QSizePolicy::Minimum,
                                                 QSizePolicy::Ignored);

    mTabWidget->widget(tabId)->setSizePolicy(QSizePolicy::Minimum,
                                             QSizePolicy::Preferred);
    mTabWidget->widget(tabId)->resize(
                mTabWidget->widget(tabId)->minimumSizeHint());*/
}

void ColorSettingsWidget::startColorPicking()
{
    const auto scene = *Document::sInstance->fActiveScene;
    if (!scene) { return; }
    CanvasMode lastMode = Document::sInstance->fCanvasMode;
    Document::sInstance->setSceneMode(CanvasMode::pickFillStrokeEvent);
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(scene, &Scene::currentPickedColor,
                    this, [this, conn, lastMode](const QColor &color) {
        qDebug() << "selected color" << color << color.isValid();
        if (color.isValid()) {
            emitStartFullColorChangedSignal();
            setDisplayedColor(color);
            emitFinishFullColorChangedSignal();
        }
        disconnect(*conn);
        Document::sInstance->setSceneMode(lastMode);
    });
}

ColorSettingsWidget::ColorSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setContentsMargins(0, 0, 0, 0);
    mWidgetsLayout->setMargin(0);

    mTabWidget->setFocusPolicy(Qt::NoFocus);

    mColorModeCombo = new QComboBox(this);
    mColorModeCombo->setMinimumWidth(60);
    mColorModeCombo->setFocusPolicy(Qt::NoFocus);

    mWidgetsLayout->setAlignment(Qt::AlignTop);
    setLayout(mWidgetsLayout);

    mColorLabel = new ColorLabel(this);
    mColorLabel->setSizePolicy(QSizePolicy::Expanding,
                               QSizePolicy::Expanding);

    const auto book = new QShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                         "colorBookmark",
                                                                         "B").toString()),
                                    this);
    connect(book, &QShortcut::activated,
            mColorLabel, &ColorLabel::addBookmark);

//    mWheelWidget->setLayout(mWheelLayout);
//    mWheelLayout->setAlignment(Qt::AlignTop);
//    wheel_triangle_widget = new H_Wheel_SV_Triangle(this);
//    mWheelLayout->addWidget(wheel_triangle_widget, Qt::AlignHCenter);
//    mWheelLayout->setAlignment(wheel_triangle_widget, Qt::AlignHCenter);


    int spinWidth = eSizesUI::widget * 3;
    rSpin->setFixedWidth(spinWidth);
    gSpin->setFixedWidth(spinWidth);
    bSpin->setFixedWidth(spinWidth);
    hSpin->setFixedWidth(spinWidth);
    hsvSSpin->setFixedWidth(spinWidth);
    vSpin->setFixedWidth(spinWidth);
    hslSSpin->setFixedWidth(spinWidth);
    lSpin->setFixedWidth(spinWidth);
    aSpin->setFixedWidth(spinWidth);

    rRect = new ColorValueRect(RED_PROGRAM, this);
    rLayout->addWidget(rLabel);
    rLayout->addWidget(rRect);
    rLayout->addWidget(rSpin);
    gRect = new ColorValueRect(GREEN_PROGRAM, this);
    gLayout->addWidget(gLabel);
    gLayout->addWidget(gRect);
    gLayout->addWidget(gSpin);
    bRect = new ColorValueRect(BLUE_PROGRAM, this);
    bLayout->addWidget(bLabel);
    bLayout->addWidget(bRect);
    bLayout->addWidget(bSpin);
    mRGBLayout->setAlignment(Qt::AlignTop);
    mRGBLayout->addLayout(rLayout);
    mRGBLayout->addLayout(gLayout);
    mRGBLayout->addLayout(bLayout);
    mRGBWidget->setLayout(mRGBLayout);

    mRGBLayout->setContentsMargins(0, 0, 0, 0);
    mRGBWidget->setContentsMargins(0, 0, 0, 0);

    hRect = new ColorValueRect(HUE_PROGRAM, this);
    hLayout->addWidget(hLabel);
    hLayout->addWidget(hRect);
    hLayout->addWidget(hSpin);
    hsvSatRect = new ColorValueRect(HSV_SATURATION_PROGRAM, this);
    hsvSLayout->addWidget(hsvSLabel);
    hsvSLayout->addWidget(hsvSatRect);
    hsvSLayout->addWidget(hsvSSpin);
    vRect = new ColorValueRect(VALUE_PROGRAM, this);
    vLayout->addWidget(vLabel);
    vLayout->addWidget(vRect);
    vLayout->addWidget(vSpin);
    mHSVLayout->setAlignment(Qt::AlignTop);
    mHSVLayout->addLayout(hLayout);
    mHSVLayout->addLayout(hsvSLayout);
    mHSVLayout->addLayout(vLayout);
    mHSVWidget->setLayout(mHSVLayout);

    mHSVLayout->setContentsMargins(0, 0, 0, 0);
    mHSVWidget->setContentsMargins(0, 0, 0, 0);

    hslSatRect = new ColorValueRect(HSL_SATURATION_PROGRAM, this);
    hslSLayout->addWidget(hslSLabel);
    hslSLayout->addWidget(hslSatRect);
    hslSLayout->addWidget(hslSSpin);
    lRect = new ColorValueRect(LIGHTNESS_PROGRAM, this);
    lLayout->addWidget(lLabel);
    lLayout->addWidget(lRect);
    lLayout->addWidget(lSpin);
    mHSLLayout->setAlignment(Qt::AlignTop);
    mHSLLayout->addLayout(hslSLayout);
    mHSLLayout->addLayout(lLayout);
    mHSLWidget->setLayout(mHSLLayout);

    mHSLLayout->setContentsMargins(0, 0, 0, 0);
    mHSLWidget->setContentsMargins(0, 0, 0, 0);

    aRect = new ColorValueRect(ALPHA_PROGRAM, this);
    aLayout->addWidget(aLabel);
    aLayout->addWidget(aRect);
    aLayout->addWidget(aSpin);

    mPickingButton = new QPushButton(QIcon::fromTheme("pick"), QString(), this);
    mPickingButton->setFocusPolicy(Qt::NoFocus);
    mPickingButton->setToolTip(tr("Pick Color"));
    connect(mPickingButton, &QPushButton::released,
            this, &ColorSettingsWidget::startColorPicking);
    eSizesUI::widget.add(mPickingButton, [this](const int size) {
        mPickingButton->setFixedSize(size, size);
    });

    mColorLabelLayout->addWidget(mColorLabel);
    mColorLabelLayout->addWidget(mPickingButton);
    mWidgetsLayout->addLayout(mColorLabelLayout);

    mTabWidget->setObjectName("ColorTabWidget");
    mTabWidget->setTabPosition(QTabWidget::South);
    mTabWidget->addTab(mRGBWidget, "RGB");
    mTabWidget->addTab(mHSVWidget, "HSV");
    mTabWidget->addTab(mHSLWidget, "HSL");
    //mTabWidget->addTab(mWheelWidget, "Wheel");
    mWidgetsLayout->addWidget(mTabWidget);
    mRGBLayout->addLayout(aLayout);

    mHexLabel = new QLabel("Hex", this);

    hexLayout = new QHBoxLayout;
    hexLayout->addWidget(mHexLabel);
    mHexEdit = new QLineEdit("#FF000000", this);
    mHexEdit->setFocusPolicy(Qt::ClickFocus);
    hexLayout->addWidget(mHexEdit);
    mRGBLayout->addLayout(hexLayout);

    QWidget *mColorModeWidget = new QWidget(this);
    mColorModeWidget->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Expanding);
    mColorModeWidget->setContentsMargins(0, 0, 0, 0);
    mColorModeLayout = new QHBoxLayout(mColorModeWidget);
    mColorModeLayout->setContentsMargins(0, 0, 0, 0);
    mColorModeLayout->addWidget(mColorModeLabel);
    mColorModeLayout->addWidget(mColorModeCombo);
    mColorModeCombo->addItem("RGB");
    mColorModeCombo->addItem("HSV");
    mColorModeCombo->addItem("HSL");
    connect(mColorModeCombo, qOverload<int>(&QComboBox::activated),
            this, &ColorSettingsWidget::setColorModeFromCombo);

    mTabWidget->setCornerWidget(mColorModeWidget);
    //mWidgetsLayout->addLayout(mColorModeLayout);

    mBookmarkedColors = new SavedColorsWidget(this);
    mWidgetsLayout->addWidget(mBookmarkedColors);
    connect(mBookmarkedColors, &SavedColorsWidget::colorSet,
            this, [this](const QColor& color) {
        emitStartFullColorChangedSignal();
        setDisplayedColor(color);
        emitFinishFullColorChangedSignal();
        Document::sInstance->actionFinished();
    });

    eSizesUI::widget.add(mColorLabel, [this](const int size) {
        mColorLabel->setFixedHeight(size);
        rRect->setFixedHeight(size);
        rLabel->setFixedWidth(size);
        gRect->setFixedHeight(size);
        gLabel->setFixedWidth(size);
        bRect->setFixedHeight(size);
        bLabel->setFixedWidth(size);
        hRect->setFixedHeight(size);
        hLabel->setFixedWidth(size);
        hsvSatRect->setFixedHeight(size);
        hsvSLabel->setFixedWidth(size);
        vRect->setFixedHeight(size);
        vLabel->setFixedWidth(size);
        hslSatRect->setFixedHeight(size);
        hslSLabel->setFixedWidth(size);
        lRect->setFixedHeight(size);
        lLabel->setFixedWidth(size);
        aRect->setFixedHeight(size);
        aLabel->setFixedWidth(size);
    });

    connect(mTabWidget, &QTabWidget::currentChanged,
            this, &ColorSettingsWidget::moveAlphaWidgetToTab);

    connect(mTabWidget, &QTabWidget::currentChanged,
            this, [](int index){
        const auto settings = eSettings::sInstance;
        if (settings->fDefaultFillStrokeIndex != index) {
            settings->fDefaultFillStrokeIndex = index;
            settings->saveKeyToFile("DefaultFillStrokeIndex");
        }
    });

    connect(rSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setRed);
    connect(gSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setGreen);
    connect(bSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setBlue);

    connect(hSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setHSVHue);
    connect(hsvSSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setHSVSaturation);
    connect(vSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setHSVValue);

    connect(hslSSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setHSLSaturation);
    connect(lSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setHSLLightness);

    connect(aSpin, &QrealAnimatorValueSlider::valueEdited,
            this, &ColorSettingsWidget::setAlpha);

    connect(rSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedRed);
    connect(gSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedGreen);
    connect(bSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedBlue);

    connect(hSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedHue);
    connect(hsvSSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedHSVSaturation);
    connect(vSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedValue);

    connect(hslSSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedHSLSaturation);
    connect(lSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedLightness);

    connect(aSpin, &QrealAnimatorValueSlider::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedAlpha);

    connect(rSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(gSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(bSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);

    connect(hSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(hsvSSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(vSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);

    connect(hslSSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(lSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);

    connect(aSpin, &QrealAnimatorValueSlider::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);

    connect(this, &ColorSettingsWidget::colorSettingSignal,
            this, [this](const ColorSetting& sett) {
        if(mHexEdit->hasFocus()) return;
        mHexEdit->setText(sett.getColor().name(QColor::HexArgb));
    });
    connect(mHexEdit, &QLineEdit::textEdited,
            this, [this](const QString& text) {
        const QColor color(text);
        emitStartFullColorChangedSignal();
        setDisplayedColor(color);
        emitFinishFullColorChangedSignal();
    });

    connect(rRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setRed);
    connect(gRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setGreen);
    connect(bRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setBlue);

    connect(hRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setHSVHue);
    connect(hsvSatRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setHSVSaturation);
    connect(vRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setHSVValue);

    connect(hslSatRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setHSLSaturation);
    connect(lRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setHSLLightness);

    connect(aRect, &ColorValueRect::valueChanged,
            this, &ColorSettingsWidget::setAlpha);

    connect(rRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedRed);
    connect(gRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedGreen);
    connect(bRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedBlue);

    connect(hRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedHue);
    connect(hsvSatRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedHSVSaturation);
    connect(vRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedValue);

    connect(hslSatRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedHSLSaturation);
    connect(lRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedLightness);

    connect(aRect, &ColorValueRect::editingStarted,
            this, &ColorSettingsWidget::emitEditingStartedAlpha);

    connect(rRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(gRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(bRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);

    connect(hRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(hsvSatRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(vRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);

    connect(hslSatRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);
    connect(lRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);

    connect(aRect, &ColorValueRect::editingFinished,
            this, &ColorSettingsWidget::emitEditingFinishedSignal);

    connect(this, &ColorSettingsWidget::colorSettingSignal,
            this, [this](const ColorSetting& setting) {
        if(mTarget) setting.apply(mTarget);
    });


    //setMinimumSize(250, 200);
    mTabWidget->setSizePolicy(QSizePolicy::MinimumExpanding,
                              QSizePolicy::Maximum);
    setDisplayedColor(Qt::black);
    setCurrentTab(eSettings::instance().fDefaultFillStrokeIndex);
}

QColor ColorSettingsWidget::getCurrentQColor() {
    const qreal red = rSpin->value();
    const qreal green = gSpin->value();
    const qreal blue = bSpin->value();
    const qreal alpha = aSpin->value();
    return QColor::fromRgbF(red, green, blue, alpha);
}

void ColorSettingsWidget::updateValuesFromRGBSpins() {
    setDisplayedColor(QColor::fromRgbF(rSpin->value(),
                                       gSpin->value(),
                                       bSpin->value(),
                                       aSpin->value()));
}

void ColorSettingsWidget::updateValuesFromHSVSpins() {
    setDisplayedColor(QColor::fromHsvF(hSpin->value(),
                                       hsvSSpin->value(),
                                       vSpin->value(),
                                       aSpin->value()));
}

void ColorSettingsWidget::updateValuesFromHSLSpins() {
    setDisplayedColor(QColor::fromHslF(hSpin->value(),
                                       hslSSpin->value(),
                                       lSpin->value(),
                                       aSpin->value()));
}

void ColorSettingsWidget::setDisplayedRGB(const qreal red,
                                          const qreal green,
                                          const qreal blue) {
    rSpin->setDisplayedValue(red);
    gSpin->setDisplayedValue(green);
    bSpin->setDisplayedValue(blue);

    rRect->setDisplayedValue(red);
    gRect->setDisplayedValue(green);
    bRect->setDisplayedValue(blue);
}

void ColorSettingsWidget::setDisplayedHSV(const qreal hue,
                                          const qreal saturation,
                                          const qreal value) {
    hSpin->setDisplayedValue(hue);
    hsvSSpin->setDisplayedValue(saturation);
    vSpin->setDisplayedValue(value);

    hRect->setDisplayedValue(hue);
    hsvSatRect->setDisplayedValue(saturation);
    vRect->setDisplayedValue(value);

    rRect->setColor(hue, saturation, value);
    gRect->setColor(hue, saturation, value);
    bRect->setColor(hue, saturation, value);

    hRect->setColor(hue, saturation, value);
    hsvSatRect->setColor(hue, saturation, value);
    vRect->setColor(hue, saturation, value);

    hslSatRect->setColor(hue, saturation, value);
    lRect->setColor(hue, saturation, value);

    if(!mAlphaHidden) aRect->setColor(hue, saturation, value);

    mColorLabel->setColor(hue, saturation, value);
}

void ColorSettingsWidget::setDisplayedHSL(const qreal hue,
                                          const qreal saturation,
                                          const qreal lightness) {
    hSpin->setDisplayedValue(hue);
    hslSSpin->setDisplayedValue(saturation);
    lSpin->setDisplayedValue(lightness);

    hRect->setDisplayedValue(hue);
    hslSatRect->setDisplayedValue(saturation);
    lRect->setDisplayedValue(lightness);
}

void ColorSettingsWidget::setDisplayedAlpha(const qreal alpha) {
    if(mAlphaHidden) return;
    mColorLabel->setAlpha(alpha);
    aRect->setDisplayedValue(alpha);
    aSpin->setDisplayedValue(alpha);
}

void ColorSettingsWidget::setDisplayedColor(const QColor& color) {
    qreal hue = qBound(0., color.hueF(), 1.);
    if(isZero4Dec(hue) &&
       (mLastTriggered == ColorParameter::value ||
        mLastTriggered == ColorParameter::lightness ||
        mLastTriggered == ColorParameter::hsvSaturation ||
        mLastTriggered == ColorParameter::hslSaturation)) {
        hue = mLastNonZeroHue;
    } else mLastNonZeroHue = hue;
    qreal hsvS = color.hsvSaturationF();
    if(isZero4Dec(hsvS) &&
       (mLastTriggered == ColorParameter::value ||
        mLastTriggered == ColorParameter::lightness)) {
        hsvS = mLastNonZeroHsvS;
    } else mLastNonZeroHsvS = hsvS;
    qreal hslS = color.hslSaturationF();
    if(isZero4Dec(hslS) &&
       (mLastTriggered == ColorParameter::value ||
        mLastTriggered == ColorParameter::lightness)) {
        hslS = mLastNonZeroHslS;
    } else mLastNonZeroHslS = hslS;
    setDisplayedRGB(color.redF(), color.greenF(), color.blueF());
    setDisplayedHSV(hue, hsvS, color.valueF());
    setDisplayedHSL(hue, hslS, color.lightnessF());
    setDisplayedAlpha(color.alphaF());
    mBookmarkedColors->setColor(color);
}

void ColorSettingsWidget::setRGB(const qreal red,
                                 const qreal green,
                                 const qreal blue) {
    setDisplayedColor(QColor::fromRgbF(red, green, blue,
                                       aSpin->value()));
    emitColorChangedSignal();
}

void ColorSettingsWidget::setHSV(const qreal hue,
                                 const qreal saturation,
                                 const qreal value) {
    setDisplayedColor(QColor::fromHsvF(hue, saturation, value,
                                       aSpin->value()));
    emitColorChangedSignal();
}

void ColorSettingsWidget::setHSL(const qreal hue,
                                 const qreal saturation,
                                 const qreal lightness) {
    setDisplayedColor(QColor::fromHslF(hue, saturation, lightness,
                                       aSpin->value()));
    emitColorChangedSignal();
}

void ColorSettingsWidget::setRed(const qreal red) {
    setRGB(red, gSpin->value(), bSpin->value());
}

void ColorSettingsWidget::setGreen(const qreal green) {
    setRGB(rSpin->value(), green, bSpin->value());
}

void ColorSettingsWidget::setBlue(const qreal blue) {
    setRGB(rSpin->value(), gSpin->value(), blue);
}

void ColorSettingsWidget::setHSVHue(const qreal hue) {
    setHSV(hue, hsvSSpin->value(), vSpin->value());
}

void ColorSettingsWidget::setHSVSaturation(const qreal saturation) {
    setHSV(hSpin->value(), saturation, vSpin->value());
}

void ColorSettingsWidget::setHSVValue(const qreal value) {
    setHSV(hSpin->value(), hsvSSpin->value(), value);
}

void ColorSettingsWidget::setHSLHue(const qreal hue) {
    setHSL(hue, hslSSpin->value(), lSpin->value());
}

void ColorSettingsWidget::setHSLSaturation(const qreal saturation) {
    setHSL(hSpin->value(), saturation, lSpin->value());
}

void ColorSettingsWidget::setHSLLightness(const qreal value) {
    setHSL(hSpin->value(), hslSSpin->value(), value);
}

void ColorSettingsWidget::updateAlphaFromSpin() {
    if(mAlphaHidden) return;
    mColorLabel->setAlpha(aSpin->value());
    aRect->setDisplayedValue(aSpin->value());
}

void ColorSettingsWidget::setColorMode() {
    const auto colorSetting = getColorSetting(ColorSettingType::apply,
                                              ColorParameter::colorMode);
    emit colorSettingSignal(colorSetting);
    Document::sInstance->actionFinished();
}

void ColorSettingsWidget::setColorModeFromCombo(int index)
{
    qDebug() << "setColorModeFromCombo" << index;
    auto colorSetting = getColorSetting(ColorSettingType::apply,
                                        ColorParameter::colorMode);
    switch(index) {
    case 1:
        colorSetting.fSettingMode = ColorMode::hsv;
        break;
    case 2:
        colorSetting.fSettingMode = ColorMode::hsl;
        break;
    default:
        colorSetting.fSettingMode = ColorMode::rgb;
    }
    emit colorSettingSignal(colorSetting);
    Document::sInstance->actionFinished();
}

void ColorSettingsWidget::setAlpha(const qreal val) {
    if(mAlphaHidden) return;
    aSpin->setDisplayedValue(val);
    mColorLabel->setAlpha(val);
    aRect->setDisplayedValue(val);
    emitColorChangedSignal();
}
