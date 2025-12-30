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

#ifndef COLORANIMATORBUTTON_H
#define COLORANIMATORBUTTON_H
#include "boxeslistactionbutton.h"
#include "smartPointers/selfref.h"
#include "conncontextptr.h"

class ColorAnimator;

class CORE_EXPORT ColorAnimatorButton : public BoxesListActionButton
{
    Q_OBJECT
    ColorAnimatorButton(QWidget * const parent = nullptr);

public:
    ColorAnimatorButton(ColorAnimator * const colorTarget,
                        QWidget * const parent = nullptr);
    ColorAnimatorButton(const QColor& color,
                        QWidget * const parent = nullptr);

    void setColorTarget(ColorAnimator * const target);
    void openColorSettingsDialog();

    void setColor(const QColor& color);
    QColor color() const;

signals:
    void colorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *);

private:
    QColor mColor;
    ConnContextQPtr<ColorAnimator> mColorTarget;
};

#endif // COLORANIMATORBUTTON_H
