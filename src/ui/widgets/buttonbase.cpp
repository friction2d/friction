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

#include "buttonbase.h"

#include "GUI/global.h"
#include <QPainter>

ButtonBase::ButtonBase(const QString &toolTip,
                       QWidget *parent) :
    QWidget(parent) {
    setToolTip(toolTip);
    setFocusPolicy(Qt::NoFocus);
    eSizesUI::button.add(this, [this](const int size) {
        setFixedSize(size, size);
    });
}

void ButtonBase::setCurrentIcon(const QImage &icon) {
    mCurrentIcon = &icon;
    update();
}

void ButtonBase::mousePressEvent(QMouseEvent *) {
    toggle();
    emit pressed();
}

void ButtonBase::enterEvent(QtEnterEvent *) {
    mHover = true;
    update();
}

void ButtonBase::leaveEvent(QEvent *) {
    mHover = false;
    update();
}

void ButtonBase::paintEvent(QPaintEvent *) {
    QPainter p(this);

    if(!isEnabled()) p.setOpacity(0.5);
    if(mCurrentIcon) p.drawImage(0, 0, *mCurrentIcon);
    if(isEnabled()) {
        if(mHover) {
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(QColor(255, 255, 255, 125), 2));
            p.drawRoundedRect(1, 1, width() - 2, height() - 2, 2.5, 2.5);
        }
    }

    p.end();
}

void ButtonBase::mouseReleaseEvent(QMouseEvent *) {
    emit released();
}
