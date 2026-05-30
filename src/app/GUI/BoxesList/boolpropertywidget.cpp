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

#include "boolpropertywidget.h"
#include "GUI/mainwindow.h"
#include "GUI/global.h"
#include "themesupport.h"

BoolPropertyWidget::BoolPropertyWidget(QWidget *parent) :
    QWidget(parent) {
    eSizesUI::widget.add(this, [this](const int size) {
        setFixedSize(size, size);
    });
}

void BoolPropertyWidget::setTarget(BoolProperty *property) {
    mTarget = property;
    mTargetContainer = nullptr;
}

void BoolPropertyWidget::setTarget(BoolPropertyContainer *property) {
    mTargetContainer = property;
    mTarget = nullptr;
}

void BoolPropertyWidget::mousePressEvent(QMouseEvent *) {
    if(mTargetContainer) {
        mTargetContainer->setValue(!mTargetContainer->getValue());
    }
    if(mTarget) {
        mTarget->setValue(!mTarget->getValue());
    }
    Document::sInstance->actionFinished();
}

void BoolPropertyWidget::paintEvent(QPaintEvent *) {
    if(!mTarget && !mTargetContainer) return;
    QPainter p(this);
    if(mTarget) {
        if(mTarget->SWT_isDisabled()) p.setOpacity(.5);
    } else if(mTargetContainer) {
        if(mTargetContainer->SWT_isDisabled()) p.setOpacity(.5);
    }

    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(ThemeSupport::getThemeButtonBorderColor());
    if(mHovered) {
        p.setPen(ThemeSupport::getThemeHighlightSelectedColor());
    } else {
        p.setPen(ThemeSupport::getThemeButtonBaseColor());
    }

    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 5., 5.);

    bool value;
    if(mTargetContainer) {
        value = mTargetContainer->getValue();
    } else {
        value = mTarget->getValue();
    }
    if(value) {
        p.setPen(QPen(Qt::white, 2.));
        p.drawLine(QPoint(6, height()/2), QPoint(width()/2, height() - 6));
        p.drawLine(QPoint(width()/2, height() - 6), QPoint(width() - 6, 6));
    }

    p.end();
}

void BoolPropertyWidget::enterEvent(QtEnterEvent *) {
    mHovered = true;
    update();
}

void BoolPropertyWidget::leaveEvent(QEvent *) {
    mHovered = false;
    update();
}

