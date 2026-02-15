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
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSize>
#include <QSizePolicy>
#include <QVBoxLayout>

#include "Private/document.h"
#include "GUI/global.h"

SavedColorsWidget::SavedColorsWidget(QWidget *parent)
    : QWidget(parent)
{
    const auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 10, 0, 0);
    verticalLayout->setSpacing(0);
    setSizePolicy(QSizePolicy::Preferred,
                  QSizePolicy::Maximum);

    const auto bookmarksHeader = new QWidget(this);
    const auto bookmarksLayout = new QHBoxLayout(bookmarksHeader);
    bookmarksLayout->setContentsMargins(0, 0, 0, 0);
    bookmarksHeader->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Preferred);

    const auto bookmarkButton = new QPushButton(QIcon::fromTheme("color"),
                                                tr("Bookmarks"),
                                                this);
    bookmarkButton->setFocusPolicy(Qt::NoFocus);
    bookmarkButton->setObjectName("NoButton");

    bookmarksLayout->addWidget(bookmarkButton);
    bookmarksLayout->addStretch();
    verticalLayout->addWidget(bookmarksHeader);
    verticalLayout->addSpacing(4);

    const auto colorsContainer = new QWidget(this);
    colorsContainer->setSizePolicy(QSizePolicy::Preferred,
                                   QSizePolicy::Maximum);
    mMainLayout = new FlowLayout(colorsContainer);
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    colorsContainer->setLayout(mMainLayout);
    verticalLayout->addWidget(colorsContainer);

    const auto addButton = new QPushButton(this);
    addButton->setCursor(Qt::PointingHandCursor);
    addButton->setFocusPolicy(Qt::NoFocus);
    addButton->setToolTip(tr("Add active color to Bookmarks"));
    addButton->setIcon(QIcon::fromTheme("plus"));

    eSizesUI::widget.add(addButton, [addButton](int size) {
        addButton->setFixedSize(size, size);
    });

    connect(addButton, &QPushButton::clicked,
            this, &SavedColorsWidget::addBookmarkButton);
    mMainLayout->addWidget(addButton);

    for (const auto& color : qAsConst(Document::sInstance->fColors)) { addColor(color); }

    connect(Document::sInstance, &Document::bookmarkColorAdded,
            this, &SavedColorsWidget::addColor);
    connect(Document::sInstance, &Document::bookmarkColorRemoved,
            this, &SavedColorsWidget::removeColor);
}

void SavedColorsWidget::addBookmarkButton()
{
    if (!mCurrentColor.isValid()) { return; }
    Document::sInstance->addBookmarkColor(mCurrentColor);
}

void SavedColorsWidget::addColor(const QColor& color)
{
    const auto button = new SavedColorButton(color, this);
    connect(button, &SavedColorButton::selected,
            this, &SavedColorsWidget::colorSet);
    mMainLayout->addWidget(button);
    mButtons << button;
}

void SavedColorsWidget::removeColor(const QColor &color)
{
    const auto rgba = color.rgba();
    for (const auto wid : qAsConst(mButtons)) {
        if (wid->getColor().rgba() == rgba) {
            mButtons.removeOne(wid);
            wid->deleteLater();
            break;
        }
    }
}

void SavedColorsWidget::setColor(const QColor &color)
{
    mCurrentColor = color;
    for (const auto wid : qAsConst(mButtons)) {
        wid->setSelected(wid->getColor().rgba() == color.rgba());
    }
}
