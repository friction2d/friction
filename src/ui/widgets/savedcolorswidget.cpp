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

#include "savedcolorswidget.h"
#include "widgets/savedcolorbutton.h"

#include <QIcon>
#include <QPalette>
#include <QSize>
#include <QToolButton>
#include <QSizePolicy>

#include "colorhelpers.h"
#include "Private/document.h"
#include "GUI/global.h"
#include "themesupport.h"

SavedColorsWidget::SavedColorsWidget(QWidget *parent)
    : QWidget(parent) {
    mMainLayout = new FlowLayout(this);
    setLayout(mMainLayout);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    mAddButton = new QToolButton(this);
    mAddButton->setCursor(Qt::PointingHandCursor);
    mAddButton->setFocusPolicy(Qt::NoFocus);
    mAddButton->setToolTip(tr("Add bookmarked color"));
    mAddButton->setIcon(QIcon::fromTheme("plus"));
    mAddButton->setAutoRaise(false);
    mAddButton->setCheckable(false);
    eSizesUI::widget.add(mAddButton, [this](int size) {
        mAddButton->setFixedSize(size, size);
        const int iconSide = qMax(0, size);
        mAddButton->setIconSize(QSize(iconSide, iconSide));
    });
    const QColor baseColor = ThemeSupport::getThemeButtonBaseColor();
    const QColor baseDarkerColor = ThemeSupport::getThemeBaseDarkerColor();
    const QColor borderColor = ThemeSupport::getThemeButtonBorderColor();
    const QColor hoverColor = ThemeSupport::getThemeHighlightColor();
    const QString style = QStringLiteral(
                "QToolButton { background-color: %1; border: 1px solid %2; padding: 0; }"
                "QToolButton:hover { background-color: %4; border: 1px solid %3; }"
                "QToolButton:pressed { border: 2px solid %3; }")
            .arg(baseColor.name(), borderColor.name(), hoverColor.name(), baseDarkerColor.name());
    mAddButton->setStyleSheet(style);
    connect(mAddButton, &QToolButton::clicked,
            this, &SavedColorsWidget::addCurrentColorRequested);
    mMainLayout->addWidget(mAddButton);

    for(const auto& color : Document::sInstance->fColors) {
        addColor(color);
    }
    connect(Document::sInstance, &Document::bookmarkColorAdded,
            this, &SavedColorsWidget::addColor);
    connect(Document::sInstance, &Document::bookmarkColorRemoved,
            this, &SavedColorsWidget::removeColor);
}

void SavedColorsWidget::addColor(const QColor& color) {
    const auto button = new SavedColorButton(color, this);
    connect(button, &SavedColorButton::selected,
            this, &SavedColorsWidget::colorSet);
    mMainLayout->addWidget(button);
    mButtons << button;
}

void SavedColorsWidget::removeColor(const QColor &color) {
    const auto rgba = color.rgba();
    for(const auto wid : mButtons) {
        if(wid->getColor().rgba() == rgba) {
            mButtons.removeOne(wid);
            wid->deleteLater();
            break;
        }
    }
}

void SavedColorsWidget::setColor(const QColor &color) {
    for(const auto wid : mButtons) {
        wid->setSelected(wid->getColor().rgba() == color.rgba());
    }
}
