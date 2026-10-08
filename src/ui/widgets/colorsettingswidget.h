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

#ifndef COLORSETTINGSWIDGET_H
#define COLORSETTINGSWIDGET_H

#include "ui_global.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>

#include "widgets/colorvaluerect.h"
#include "widgets/qrealanimatorvalueslider.h"
#include "Animators/coloranimator.h"
#include "Animators/paintsettingsanimator.h"
#include "paintsettingsapplier.h"
#include "conncontextptr.h"
#include "widgets/actionbutton.h"
#include "widgets/colorlabel.h"
#include "widgets/savedcolorswidget.h"
#include "widgets/colorwheel.h"

class UI_EXPORT ColorSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ColorSettingsWidget(QWidget *parent = nullptr);

    QColor getCurrentQColor();
    void setDisplayedColor(const QColor& color);

    void setTarget(ColorAnimator * const target);

    ColorSetting getColorSetting(const ColorSettingType type,
                                 const ColorParameter parameter) const;
    void setColorModeVisible(const bool &visible);
    void setCurrentTab(const int &index);

signals:
    void colorSettingSignal(const ColorSetting&);

protected:
    void showEvent(QShowEvent *e) override;

private:
    void emitColorChangedSignal();
    void emitEditingFinishedSignal();
    void emitEditingStartedSignal();

    void emitEditingStartedRed();
    void emitEditingStartedGreen();
    void emitEditingStartedBlue();

    void emitEditingStartedHue();
    void emitEditingStartedHSVSaturation();
    void emitEditingStartedValue();

    void emitEditingStartedHSLSaturation();
    void emitEditingStartedLightness();

    void emitEditingStartedAlpha();

    void emitStartFullColorChangedSignal();
    void emitFinishFullColorChangedSignal();

    void moveAlphaWidgetToTab(const int tabId);

    void startColorPicking();

    void setAlpha(const qreal val);

    void updateValuesFromRGBSpins();
    void updateValuesFromHSVSpins();
    void updateValuesFromHSLSpins();
    void updateAlphaFromSpin();

    void setDisplayedRGB(const qreal red,
                         const qreal green,
                         const qreal blue);
    void setDisplayedHSV(const qreal hue,
                         const qreal saturation,
                         const qreal value);
    void setDisplayedHSL(const qreal hue,
                         const qreal saturation,
                         const qreal lightness);
    void setDisplayedAlpha(const qreal alpha);

    void setRGB(const qreal red,
                const qreal green,
                const qreal blue);
    void setHSV(const qreal hue,
                const qreal saturation,
                const qreal value);
    void setHSL(const qreal hue,
                const qreal saturation,
                const qreal lightness);

    void setRed(const qreal red);
    void setGreen(const qreal green);
    void setBlue(const qreal blue);

    void setHSVHue(const qreal hue);
    void setHSVSaturation(const qreal saturation);
    void setHSVValue(const qreal value);

    void setHSLHue(const qreal hue);
    void setHSLSaturation(const qreal saturation);
    void setHSLLightness(const qreal value);

    void setColorMode();
    void setColorModeFromCombo(int index);
    void updateWidgetTargets();

    qreal mLastNonZeroHue = 0;
    qreal mLastNonZeroHsvS = 0;
    qreal mLastNonZeroHslS = 0;

    ColorParameter mLastTriggered;

    ConnContextQPtr<ColorAnimator> mTarget;

    QHBoxLayout *mColorModeLayout;// = new QHBoxLayout();
    QLabel *mColorModeLabel = new QLabel(tr("Mode"), this);
    QComboBox *mColorModeCombo;
    SavedColorsWidget* mBookmarkedColors;

    QTabWidget *mTabWidget = new QTabWidget();
    QVBoxLayout *mWidgetsLayout = new QVBoxLayout();

    QWidget *mWheelWidget = new QWidget();
    QVBoxLayout *mWheelLayout = new QVBoxLayout();
    Friction::Ui::ColorWheel *mColorWheel = nullptr;

    QWidget *mRGBWidget = new QWidget();
    QVBoxLayout *mRGBLayout = new QVBoxLayout();
    QHBoxLayout *rLayout = new QHBoxLayout();
    QLabel *rLabel = new QLabel("R");
    ColorValueRect *rRect = nullptr;
    QrealAnimatorValueSlider *rSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);
    QHBoxLayout *gLayout = new QHBoxLayout();
    QLabel *gLabel = new QLabel("G");
    ColorValueRect *gRect = nullptr;
    QrealAnimatorValueSlider *gSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);
    QHBoxLayout *bLayout = new QHBoxLayout();
    QLabel *bLabel = new QLabel("B");
    ColorValueRect *bRect = nullptr;
    QrealAnimatorValueSlider *bSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);

    QWidget *mHSVWidget = new QWidget();
    QVBoxLayout *mHSVLayout = new QVBoxLayout();
    QHBoxLayout *hLayout = new QHBoxLayout();
    QLabel *hLabel = new QLabel("H");
    ColorValueRect *hRect = nullptr;
    QrealAnimatorValueSlider *hSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);
    QHBoxLayout *hsvSLayout = new QHBoxLayout();
    QLabel *hsvSLabel = new QLabel("S");
    ColorValueRect *hsvSatRect = nullptr;
    QrealAnimatorValueSlider *hsvSSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);
    QHBoxLayout *vLayout = new QHBoxLayout();
    QLabel *vLabel = new QLabel("V");
    ColorValueRect *vRect = nullptr;
    QrealAnimatorValueSlider *vSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);

    QWidget *mHSLWidget = new QWidget();
    QVBoxLayout *mHSLLayout = new QVBoxLayout();

    QHBoxLayout *hslSLayout = new QHBoxLayout();
    QLabel *hslSLabel = new QLabel("S");
    ColorValueRect *hslSatRect = nullptr;
    QrealAnimatorValueSlider *hslSSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);
    QHBoxLayout *lLayout = new QHBoxLayout();
    QLabel *lLabel = new QLabel("L");
    ColorValueRect *lRect = nullptr;
    QrealAnimatorValueSlider *lSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);

    bool mAlphaHidden = false;
    QHBoxLayout *aLayout = new QHBoxLayout();
    QLabel *aLabel = new QLabel("A");
    ColorValueRect *aRect = nullptr;
    QrealAnimatorValueSlider *aSpin =
            new QrealAnimatorValueSlider(0., 1., 0.01, this, false);

    QHBoxLayout *hexLayout;
    QLineEdit *mHexEdit;
    QLabel *mHexLabel;

    QHBoxLayout *mColorLabelLayout = new QHBoxLayout();
    QPushButton *mPickingButton;

    ColorLabel *mColorLabel = nullptr;

    ConnContext mUpdateConnections;
};

#endif // COLORSETTINGSWIDGET_H
