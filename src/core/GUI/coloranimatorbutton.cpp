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

#include "coloranimatorbutton.h"
#include "Animators/coloranimator.h"
#include "themesupport.h"
#include "colorsetting.h"
#include "GUI/ewidgets.h"
#include <QVBoxLayout>
#include <QDialog>

ColorAnimatorButton::ColorAnimatorButton(QWidget * const parent)
    : BoxesListActionButton(parent)
{
    connect(this, &BoxesListActionButton::pressed,
            this, &ColorAnimatorButton::openColorSettingsDialog);
}

ColorAnimatorButton::ColorAnimatorButton(ColorAnimator * const colorTarget,
                                         QWidget * const parent)
    : ColorAnimatorButton(parent)
{
    setColorTarget(colorTarget);
}

ColorAnimatorButton::ColorAnimatorButton(const QColor &color,
                                         QWidget * const parent)
    : ColorAnimatorButton(parent)
{
    mColor = color;
}

void ColorAnimatorButton::setColorTarget(ColorAnimator * const target)
{
    mColorTarget.assign(target);
    if (target) {
        mColorTarget << connect(target->getVal1Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorAnimatorButton::update));
        mColorTarget << connect(target->getVal2Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorAnimatorButton::update));
        mColorTarget << connect(target->getVal3Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorAnimatorButton::update));
    }
    update();
}

void ColorAnimatorButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    const QRectF rect(0.0, 0.0, width(), height());
    const float borderWidth = 2.0;
    const QRectF innerRect = rect.adjusted(borderWidth,
                                           borderWidth,
                                           -borderWidth,
                                           -borderWidth);

    if (mHover) {
        const QColor color = ThemeSupport::getLightDarkColor(mColor, 130);
        p.setPen(color);
        p.setBrush(color);
        p.drawRoundedRect(rect,
                          borderWidth + 2,
                          borderWidth + 2);
        p.setBrush(mColor);
        p.drawRoundedRect(innerRect,
                          borderWidth,
                          borderWidth);
    } else {
        p.setPen(mColor);
        p.setBrush(mColor);
        p.drawRoundedRect(rect,
                          borderWidth + 2,
                          borderWidth + 2);
    }
}

void ColorAnimatorButton::openColorSettingsDialog()
{
    const auto dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setLayout(new QVBoxLayout(dialog));
    QWidget* colorWidget = nullptr;

    if (mColorTarget) {
        colorWidget = eWidgets::sColorWidget(dialog,
                                             mColorTarget);
    } else {
        const auto colOp = [this](const ColorSetting& setting) {
            setColor(setting.getColor());
        };
        colorWidget = eWidgets::sColorWidget(dialog,
                                             mColor,
                                             this,
                                             colOp);
    }
    dialog->layout()->addWidget(colorWidget);

    dialog->raise();
    dialog->show();
}

void ColorAnimatorButton::setColor(const QColor &color)
{
    mColor = color;
    update();

    emit colorChanged(color);
}

QColor ColorAnimatorButton::color() const
{
    if (mColorTarget) { return mColorTarget->getColor(); }
    return mColor;
}
