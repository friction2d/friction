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

#include "flowlayout.h"
#include <QtWidgets>

FlowLayout::FlowLayout(QWidget *parent,
                       const int margin,
                       const int hSpacing,
                       const int vSpacing)
    : QLayout(parent), mHSpace(hSpacing), mVSpace(vSpacing) {
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::FlowLayout(const int margin,
                       const int hSpacing,
                       const int vSpacing)
    : mHSpace(hSpacing), mVSpace(vSpacing) {
    setContentsMargins(margin, margin, margin, margin);
}

//FlowLayout::~FlowLayout() {
//    QLayoutItem *item;
//    while((item = takeAt(0))) delete item;
//}

void FlowLayout::addItem(QLayoutItem *item) {
    mItemList.append(item);
}

int FlowLayout::horizontalSpacing() const {
    if(mHSpace >= 0) {
        return mHSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
}

int FlowLayout::verticalSpacing() const {
    if(mVSpace >= 0) {
        return mVSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
}

int FlowLayout::count() const {
    return mItemList.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const {
    return mItemList.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index) {
    if(index >= 0 && index < mItemList.size())
        return mItemList.takeAt(index);
    else
        return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const {
    return Qt::Horizontal;
}

bool FlowLayout::hasHeightForWidth() const {
    return true;
}

int FlowLayout::heightForWidth(int width) const {
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

void FlowLayout::setGeometry(const QRect &rect) {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const {
    return minimumSize();
}

QSize FlowLayout::minimumSize() const {
    QSize size;
    for(const auto item : mItemList)
        size = size.expandedTo(item->minimumSize());

    QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(),
                  margins.top() + margins.bottom());
    return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const {
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    for(const auto item : mItemList) {
        QWidget *wid = item->widget();
        int spaceX = horizontalSpacing();
        if(spaceX == -1)
            spaceX = wid->style()->layoutSpacing(
                QSizePolicy::DefaultType, QSizePolicy::DefaultType, Qt::Horizontal);
        int spaceY = verticalSpacing();
        if(spaceY == -1)
            spaceY = wid->style()->layoutSpacing(
                QSizePolicy::DefaultType, QSizePolicy::DefaultType, Qt::Vertical);
        int nextX = x + item->sizeHint().width() + spaceX;
        if(nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + item->sizeHint().width() + spaceX;
            lineHeight = 0;
        }

        if(!testOnly)
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }
    return y + lineHeight - rect.y() + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const {
    QObject *parent = this->parent();
    if(!parent) {
        return -1;
    } else if(parent->isWidgetType()) {
        QWidget *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    } else {
        return static_cast<QLayout *>(parent)->spacing();
    }
}
