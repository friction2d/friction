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

#include "scrollvisiblepartbase.h"
#include <QPainter>
#include "swt_abstraction.h"
#include "singlewidget.h"
#include "scrollwidget.h"
#include "singlewidgettarget.h"
#include "GUI/global.h"
#include <QApplication>

ScrollVisiblePartBase::ScrollVisiblePartBase(
        MinimalScrollWidget * const parent) {
    Q_ASSERT(parent);
    mParentWidget = parent;
    connect(&eSizesUI::widget, &SizeSetter::sizeChanged, this, [this]() {
        updateParentHeight();
        updateVisibleWidgets();
    });
}

void ScrollVisiblePartBase::updateParentHeightAndContent() {
    updateParentHeight();
    updateVisibleWidgetsContent();
}

void ScrollVisiblePartBase::setVisibleTop(const int top) {
    mVisibleTop = top;
    updateVisibleWidgetsContent();
}

void ScrollVisiblePartBase::setVisibleHeight(const int height) {
    setFixedHeight(height);
    mVisibleHeight = height;
    updateVisibleWidgets();
}

void ScrollVisiblePartBase::updateWidgetsWidth() {
    for(const auto& widget : qAsConst(mSingleWidgets)) {
        widget->setFixedWidth(width() - widget->x());
    }
}

void ScrollVisiblePartBase::callUpdaters() {
    updateParentHeightIfNeeded();
    updateVisibleWidgetsContentIfNeeded();
    update();
}

void ScrollVisiblePartBase::scheduleContentUpdate() {
    planScheduleUpdateParentHeight();
    planScheduleUpdateVisibleWidgetsContent();
}

bool ScrollVisiblePartBase::event(QEvent *event) {
    if(event->type() == QEvent::User) {
        mEventSent = false;
        callUpdaters();
        return true;
    } else return QWidget::event(event);
}

void ScrollVisiblePartBase::postUpdateEvent() {
    if(mEventSent) return;
    QApplication::postEvent(this, new QEvent(QEvent::User));
    mEventSent = true;
}

void ScrollVisiblePartBase::planScheduleUpdateVisibleWidgetsContent() {
    if(mContentUpdateScheduled) return;
    mContentUpdateScheduled = true;
    postUpdateEvent();
}

void ScrollVisiblePartBase::updateVisibleWidgetsContentIfNeeded() {
    if(mContentUpdateScheduled) {
        mContentUpdateScheduled = false;
        updateVisibleWidgetsContent();
    }
}

void ScrollVisiblePartBase::planScheduleUpdateParentHeight() {
    if(mParentHeightUpdateScheduled) return;
    mParentHeightUpdateScheduled = true;
    postUpdateEvent();
}

void ScrollVisiblePartBase::updateParentHeightIfNeeded() {
    if(mParentHeightUpdateScheduled) {
        mParentHeightUpdateScheduled = false;
        updateParentHeight();
    }
}

void ScrollVisiblePartBase::updateVisibleWidgets() {
    const int neededWidgets = qCeil(mVisibleHeight/
                              static_cast<qreal>(eSizesUI::widget));
    const int currentNWidgets = mSingleWidgets.count();

    if(neededWidgets == currentNWidgets) return;
    if(neededWidgets > currentNWidgets) {
        for(int i = neededWidgets - currentNWidgets; i > 0; i--) {
            mSingleWidgets.append(createNewSingleWidget());
        }
    } else {
        for(int i = currentNWidgets - neededWidgets; i > 0; i--) {
            delete mSingleWidgets.takeLast();
        }
    }

    int y = 0;
    for(const auto& widget : qAsConst(mSingleWidgets)) {
        widget->move(widget->x(), y);
        widget->setFixedWidth(width() - widget->x());
        y += eSizesUI::widget;
    }

    updateVisibleWidgetsContent();
}

void ScrollVisiblePartBase::updateParentHeight() {
    mParentWidget->updateHeight();
}
