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

#include "savedcolorbutton.h"
#include <QPainter>
#include <QMenu>
#include <QAction>
#include "GUI/global.h"
#include "Private/document.h"

SavedColorButton::SavedColorButton(const QColor &colorT,
                                   QWidget *parent) :
    QWidget(parent) {
    mColor = colorT;
    setFixedSize(eSizesUI::widget, eSizesUI::widget);
}

void SavedColorButton::setSelected(const bool selected) {
    mSelected = selected;
    update();
}

void SavedColorButton::mousePressEvent(QMouseEvent *e) {
    if(e->button() == Qt::LeftButton) {
        emit selected(mColor);
    } else if(e->button() == Qt::RightButton) {
        QMenu menu(this);
        menu.addAction(QIcon::fromTheme("minus"), tr("Unbookmark Color"));
        const auto act = menu.exec(e->globalPos());
        if(act) {
            if(act->text() == tr("Unbookmark Color")) {
                Document::sInstance->removeBookmarkColor(getColor());
            }
        }
    }
}

void SavedColorButton::paintEvent(QPaintEvent *) {
    QPainter p(this);
    if(mColor.alpha() != 255) {
        p.drawTiledPixmap(rect(), *ALPHA_MESH_PIX);
    }
    const QRectF rect(0.0, 0.0, width(), height());
    const float borderWidth = 2.0;
    const QRectF innerRect = rect.adjusted(borderWidth, borderWidth, -borderWidth, -borderWidth);

    if(mSelected) {
        if(mHovered) {
            p.setPen(mColor.darker(170));
            p.setBrush(mColor.darker(170));
            p.drawRoundedRect(rect, borderWidth + 2, borderWidth + 2);
            p.setBrush(mColor);
            p.drawRoundedRect(innerRect, borderWidth, borderWidth);
        } else {
            p.setPen(mColor.darker(150));
            p.setBrush(mColor.darker(150));
            p.drawRoundedRect(rect, borderWidth + 2, borderWidth + 2);
            p.setBrush(mColor);
            p.drawRoundedRect(innerRect, borderWidth, borderWidth);
        }
    } else if(mHovered) {
        p.setPen(mColor.darker(130));
        p.setBrush(mColor.darker(130));
        p.drawRoundedRect(rect, borderWidth + 2, borderWidth + 2);
        p.setBrush(mColor);
        p.drawRoundedRect(innerRect, borderWidth, borderWidth);
    } else {
        p.setPen(mColor);
        p.setBrush(mColor);
        p.drawRoundedRect(rect, borderWidth + 2, borderWidth + 2);
    }
    p.end();
}
