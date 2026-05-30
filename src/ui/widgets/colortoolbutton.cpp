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

#include "widgets/colortoolbutton.h"
#include "Animators/coloranimator.h"
#include "Animators/outlinesettingsanimator.h"
#include "Animators/paintsettingsanimator.h"
#include "GUI/global.h"
#include "Private/document.h"
#include "colorsetting.h"
#include "themesupport.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>

using namespace Friction::Ui;

ColorToolButton::ColorToolButton(Document& document,
                                 QWidget *parent,
                                 const bool fillOnly,
                                 const bool strokeOnly,
                                 const bool flatOnly)
    : QToolButton(parent)
    , mIsFillOnly(fillOnly)
    , mIsStrokeOnly(strokeOnly)
    , mIsFlatOnly(flatOnly)
    , mPop(nullptr)
    , mScroll(nullptr)
    , mColor(Qt::transparent)
    , mColorLabel(nullptr)
    , mBackgroundWidget(nullptr)
    , mFillStrokeWidget(nullptr)
    , mColorAct(nullptr)
    , mDocument(document)
{
    setFocusPolicy(Qt::NoFocus);
    setPopupMode(ToolButtonPopupMode::InstantPopup);

    mPop = new QFrame(this);
    mPop->setObjectName("PopWidget");
    const auto popLay = new QVBoxLayout(mPop);

    mPop->setMinimumSize({300, 235});

    mScroll = new QScrollArea(this);
    mScroll->setObjectName("DarkWidget");
    mScroll->setFocusPolicy(Qt::NoFocus);

    mScroll->setBackgroundRole(QPalette::Window);
    mScroll->setFrameShadow(QFrame::Plain);
    mScroll->setFrameShape(QFrame::NoFrame);
    mScroll->setWidgetResizable(true);

    popLay->addWidget(mScroll);

    if (!mIsFlatOnly) {
        mFillStrokeWidget = new FillStrokeSettingsWidget(mDocument,
                                                         this,
                                                         true,
                                                         mIsFillOnly,
                                                         mIsStrokeOnly);
        mFillStrokeWidget->setContentsMargins(0, 0, 0, 0);
        mScroll->setWidget(mFillStrokeWidget);
    } else {
        mBackgroundWidget = new ColorSettingsWidget(this);
        mBackgroundWidget->setObjectName("DarkWidget");
        mBackgroundWidget->setColorModeVisible(false);
        mScroll->setWidget(mBackgroundWidget);
    }

    mColorAct = new QWidgetAction(this);
    mColorAct->setDefaultWidget(mPop);

    mColorLabel = new ColorLabel(this, false);

    const auto mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->addWidget(mColorLabel);

    addAction(mColorAct);
    updateColor();
}

void ColorToolButton::setCurrentBox(BoundingBox *target)
{
    mFillStrokeWidget->setCurrentBox(target);
}

void ColorToolButton::setColorFillTarget(FillSettingsAnimator * const target)
{
    if (mIsFlatOnly) { return; }
    mColorFillTarget.assign(target);
    setColorTarget(target ? target->getColorAnimator() : nullptr);
}

void ColorToolButton::setColorStrokeTarget(OutlineSettingsAnimator * const target)
{
    if (mIsFlatOnly) { return; }
    mColorStrokeTarget.assign(target);
    setColorTarget(target ? target->getColorAnimator() : nullptr);
}

void ColorToolButton::setColorTarget(ColorAnimator * const target)
{
    if (mBackgroundWidget) { mBackgroundWidget->setTarget(target); }
    mColorTarget.assign(target);
    if (target) {
        mColorTarget << connect(target->getVal1Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorToolButton::updateColor));
        mColorTarget << connect(target->getVal2Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorToolButton::updateColor));
        mColorTarget << connect(target->getVal3Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorToolButton::updateColor));
    }
    updateColor();
}

void ColorToolButton::updateColor()
{
    PaintType type = mIsFlatOnly ? PaintType::FLATPAINT : PaintType::NOPAINT;
    if (mColorFillTarget || mColorStrokeTarget) {
        if (mColorFillTarget) {
            type = mColorFillTarget->getPaintType();
            mColorLabel->setGradient(type == PaintType::GRADIENTPAINT ?
                                         mColorFillTarget->getGradient() : nullptr);
        } else if (mColorStrokeTarget) {
            type = mColorStrokeTarget->getPaintType();
            mColorLabel->setGradient(type == PaintType::GRADIENTPAINT ?
                                         mColorStrokeTarget->getGradient() : nullptr);
        }
    } else {
        if (!mIsFlatOnly) { mColorLabel->setGradient(nullptr); }
    }

    const QColor color = mColorTarget ? mColorTarget->getColor() : mColor;

    mColor = type == PaintType::NOPAINT ? Qt::transparent : color;
    mColorLabel->setColor(mColor);
    mColorLabel->setAlpha(mColor.alphaF());
}

QColor ColorToolButton::color() const
{
    if (mColorTarget) { return mColorTarget->getColor(); }
    return mColor;
}

void ColorToolButton::mousePressEvent(QMouseEvent *e)
{
    const auto button = e->button();
    if (button == Qt::LeftButton) {
        if (mFillStrokeWidget) { mFillStrokeWidget->adjustSize(); }
        else if (mBackgroundWidget) { mBackgroundWidget->adjustSize(); }

        mScroll->setWidget(mScroll->takeWidget());
        mScroll->adjustSize();
        mScroll->setMinimumHeight(mScroll->widget()->sizeHint().height() + 10);

        mPop->adjustSize();
    }

    QToolButton::mousePressEvent(e);
}

void ColorToolButton::wheelEvent(QWheelEvent *e)
{
#ifdef Q_OS_MAC
    if (e->angleDelta().y() == 0) { return; }
#endif
    if (mColorTarget) {
        const bool ctrl = QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
        const bool shift = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
        QColor color = mColorTarget->getColor().toHsv();

        qreal h = clamp(color.hueF(), 0., 1.);
        qreal s = clamp(color.saturationF(), 0., 1.);
        qreal v = clamp(color.valueF(), 0., 1.);
        qreal a = clamp(color.alphaF(), 0., 1.);

        qreal step = 0.01;
        const int y = e->angleDelta().y();

        if (ctrl && shift) {
            a = clamp(y > 0 ? a + step : a - step, 0., 1.);
            emit message(tr("Alpha: %1").arg(QString::number(a)));
        } else if (ctrl) {
            s = clamp(y > 0 ? s + step : s - step, 0., 1.);
            emit message(tr("Saturation: %1").arg(QString::number(s)));
        } else if (shift) {
            v = clamp(y > 0 ? v + step : v - step, 0., 1.);
            emit message(tr("Value: %1").arg(QString::number(v)));
        } else {
            h = clamp(y > 0 ? h + step : h - step, 0., 1.);
            emit message(tr("Hue: %1").arg(QString::number(h)));
        }

        mColorTarget->setColor(QColor::fromHsvF(h, s, v, a));
        mDocument.actionFinished();
    } else {
        QToolButton::wheelEvent(e);
    }
}
