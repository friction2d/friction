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

#include "colorpickingwidget.h"
#include "Private/document.h"
#include "GUI/global.h"

#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include <QWindow>

ColorPickingWidget::ColorPickingWidget(QScreen* const screen,
                                       QWidget * const parent)
    : QWidget(parent) {
    mScreenshot = screen->grabWindow(0).toImage();

    setCursor(Qt::PointingHandCursor);

    setMouseTracking(true);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose);

    showFullScreen();
    setFocus();
}

void ColorPickingWidget::mousePressEvent(QMouseEvent *e) {
    if(e->button() == Qt::RightButton) {
        close();
    } else if(e->button() == Qt::LeftButton) {
        emit colorSelected(mCurrentColor);
        Document::sInstance->actionFinished();
        close();
    }
}

void ColorPickingWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.drawImage(0, 0, mScreenshot);
    p.fillRect(mCursorX + 2*eSizesUI::widget - 4,
               mCursorY - eSizesUI::widget - 4,
               eSizesUI::widget + 8,
               eSizesUI::widget + 8, Qt::black);
    p.fillRect(mCursorX + 2*eSizesUI::widget - 2,
               mCursorY - eSizesUI::widget - 2,
               eSizesUI::widget + 4,
               eSizesUI::widget + 4, Qt::white);
    p.fillRect(mCursorX + 2*eSizesUI::widget,
               mCursorY - eSizesUI::widget,
               eSizesUI::widget, eSizesUI::widget,
               mCurrentColor);
    p.end();
}

void ColorPickingWidget::keyPressEvent(QKeyEvent *e) {
    if(e->isAutoRepeat()) return;
    close();
}

void ColorPickingWidget::mouseMoveEvent(QMouseEvent *e) {
    updateBox(e->pos());
}

void ColorPickingWidget::focusOutEvent(QFocusEvent*) {
    close();
}

QColor ColorPickingWidget::colorFromPoint(const int x, const int y) {
    const QRgb rgb = mScreenshot.pixel(x, y);
    return QColor::fromRgb(rgb);
}

void ColorPickingWidget::updateBox(const QPoint& pos) {
    mCursorX = pos.x();
    mCursorY = pos.y();
    qreal pixelRatio = devicePixelRatioF();
    mCurrentColor = colorFromPoint(pos.x() * pixelRatio, pos.y() * pixelRatio);
    update();
}
