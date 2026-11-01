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

#include "layouthandler.h"
#include "Private/document.h"
#include "widgets/editablecombobox.h"

#include <QPushButton>

LayoutHandler::LayoutHandler(Document& document,
                             AudioHandler& audioHandler,
                             QWidget* const parent) :
    mDocument(document), mAudioHandler(audioHandler) {
    const auto canvasComboLayout = new QHBoxLayout;
    canvasComboLayout->setSpacing(0);
    canvasComboLayout->setMargin(0);
    canvasComboLayout->setContentsMargins(0, 0, 0, 0);

    mComboWidget = new QWidget(parent);
    mComboWidget->setContentsMargins(0, 0, 0, 0);
    mComboWidget->setSizePolicy(QSizePolicy::Expanding,
                                QSizePolicy::Expanding);
    mComboWidget->setLayout(canvasComboLayout);

    mComboBox = new EditableComboBox(mComboWidget, true);
    mComboBox->setSizePolicy(QSizePolicy::Preferred,
                             QSizePolicy::Expanding);
    mComboBox->setMinimumContentsLength(20);

    const auto newLayPush = new QPushButton(QIcon::fromTheme("plus"), QString(), mComboWidget);
    newLayPush->setFocusPolicy(Qt::NoFocus);
    newLayPush->setToolTip(tr("Add"));
    newLayPush->setObjectName("FlatButton");
    eSizesUI::widget.add(newLayPush, [newLayPush](const int size) {
        newLayPush->setFixedSize(QSize(size, size));
    });

    const auto removeLayPush = new QPushButton(QIcon::fromTheme("minus"), QString(), mComboWidget);
    removeLayPush->setFocusPolicy(Qt::NoFocus);
    removeLayPush->setToolTip(tr("Remove"));
    removeLayPush->setObjectName("FlatButton");
    eSizesUI::widget.add(removeLayPush, [removeLayPush](const int size) {
        removeLayPush->setFixedSize(QSize(size, size));
    });

    canvasComboLayout->addWidget(mComboBox);
    canvasComboLayout->addWidget(newLayPush);
    canvasComboLayout->addWidget(removeLayPush);

    mSceneLayout = new QStackedWidget();
    mTimelineLayout = new QStackedWidget();

    connect(mComboBox, qOverload<int>(&QComboBox::activated),
            this, &LayoutHandler::setCurrent);
    connect(mComboBox->lineEdit(), &QLineEdit::textEdited,
            this, &LayoutHandler::renameCurrent);

    connect(newLayPush, &QPushButton::pressed,
            this, &LayoutHandler::newLayoutAction);
    connect(removeLayPush, &QPushButton::pressed,
            this, &LayoutHandler::removeCurrent);

    connect(&document, &Document::sceneCreated,
            this, &LayoutHandler::newForSceneAction);
    connect(&document, qOverload<Canvas*>(&Document::sceneRemoved),
            this, &LayoutHandler::removeForScene);
}

void LayoutHandler::clear() {
    for(int i = 0; i < mComboBox->count(); i++)
        removeAt(0);
}
