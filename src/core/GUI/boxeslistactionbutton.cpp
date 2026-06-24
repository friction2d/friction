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

#include "boxeslistactionbutton.h"

#include <QPainter>

#include "themesupport.h"

BoxesListActionButton::BoxesListActionButton(QWidget * const parent)
    : QWidget(parent)
{
    eSizesUI::widget.add(this, [this](const int size) {
        setFixedSize(size, size);
    });
}

void BoxesListActionButton::mousePressEvent(QMouseEvent *)
{
    emit pressed();
}

void BoxesListActionButton::enterEvent(QtEnterEvent *)
{
    mHover = true;
    update();
}

void BoxesListActionButton::leaveEvent(QEvent *)
{
    mHover = false;
    update();
}

void PixmapActionButton::paintEvent(QPaintEvent *)
{
    if (!mPixmapChooser) { return; }
    const auto pix = mPixmapChooser();
    if (!pix) { return; }

    QPainter p(this);
    const int pX = 0;
    if (mHover) { p.fillRect(QRect(QPoint(pX, pX), pix->size()), ThemeSupport::getThemeHighlightColor(50)); }
    p.drawPixmap(pX, pX, *pix);
    p.end();
}
