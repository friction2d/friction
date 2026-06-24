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

#include "timelinehighlightwidget.h"

#include <QMouseEvent>
#include <QPainter>

#include "GUI/global.h"
#include "Private/esettings.h"

TimelineHighlightWidget::TimelineHighlightWidget(const bool track,
                                                 QWidget * const parent,
                                                 const bool alt)
    : QWidget(parent)
    , mAlt(alt)
    , mSettings(*eSettings::sInstance)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(track);
}

void TimelineHighlightWidget::setOther(TimelineHighlightWidget * const other)
{ mOther = other; }

void TimelineHighlightWidget::mouseMoveEvent(QMouseEvent *event) {
    setHoverRow(AppSupport::getMouseY(event) / eSizesUI::widget);
    event->ignore();
}

void TimelineHighlightWidget::leaveEvent(QEvent *) {
    setHoverRow(-1);
}

void TimelineHighlightWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    if (mSettings.fTimelineAlternateRow && mAlt) {
        const int height = this->height();
        const QColor color = mSettings.fTimelineAlternateRowColor;
        for (int i = 0; i < height; i += 2*eSizesUI::widget) {
            p.fillRect(0, i, width(), eSizesUI::widget, color);
        }
    }
    if (mSettings.fTimelineHighlightRow && mHoverRow >= 0) {
        p.fillRect(0, mHoverRow*eSizesUI::widget,
                   width(), eSizesUI::widget,
                   mSettings.fTimelineHighlightRowColor);
    }
    p.end();
}

void TimelineHighlightWidget::setHoverRow(const int row) {
    if(mHoverRow == row) return;
    mHoverRow = row;
    update();
    if(mOther) mOther->setHoverRow(row);
}
