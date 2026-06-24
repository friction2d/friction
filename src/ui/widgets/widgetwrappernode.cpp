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

#include "widgetwrappernode.h"

StackWrapperMenu::StackWrapperMenu() {
    //setStyleSheet("QMenuBar#cornerMenuBar::item { padding-top: 1px; margin-bottom: 2px; }");
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

WidgetWrapperNode::WidgetWrapperNode(const WrapperNode::WidgetCreator &creator) :
    WrapperNode(WrapperNodeType::widget, creator) {
    mCornerMenu = new WidgetWrapperCornerMenu(this);
    eSizesUI::widget.add(mCornerMenu, [this](const int size) {
        mCornerMenu->setFixedHeight(size);
    });
    mLayout = new QVBoxLayout(this);
    mMenuLayout = new QHBoxLayout();
    mMenuLayout->addWidget(mCornerMenu);
    mLayout->addLayout(mMenuLayout);
    setLayout(mLayout);
    mLayout->setSpacing(0);
    mLayout->setContentsMargins(0, 0, 0, 0);
}

void WidgetWrapperNode::setMenuBar(StackWrapperMenu * const menu) {
    if(mMenuBar) delete mMenuBar;
    mMenuBar = menu;
    mMenuLayout->addWidget(mMenuBar);
    mMenuLayout->addWidget(mCornerMenu);
}

void WidgetWrapperNode::setCentralWidget(QWidget * const widget) {
    if(mCenterWidget) delete mCenterWidget;
    mCenterWidget = widget;
    mLayout->addWidget(mCenterWidget);
    mCenterWidget->setSizePolicy(QSizePolicy::MinimumExpanding,
                                 QSizePolicy::MinimumExpanding);
}

void WidgetWrapperNode::close() {
    if(fParent) fParent->closeChild(this);
}

void WidgetWrapperNode::splitV() {
    Q_ASSERT(fParent);
    const auto vStack = new VWidgetStackNode(fCreator);
    vStack->fChild1 = this;
    vStack->fChild2 = fCreator(nullptr);

    fParent->replaceChild(this, vStack);

    vStack->appendWidget(vStack->fChild1->widget());
    vStack->appendWidget(vStack->fChild2->widget());

    vStack->fChild1->fParent = vStack;
    vStack->fChild2->fParent = vStack;
}

void WidgetWrapperNode::splitH() {
    Q_ASSERT(fParent);
    const auto hStack = new HWidgetStackNode(fCreator);
    hStack->fChild1 = this;
    hStack->fChild2 = fCreator(nullptr);

    fParent->replaceChild(this, hStack);

    hStack->appendWidget(hStack->fChild1->widget());
    hStack->appendWidget(hStack->fChild2->widget());

    hStack->fChild1->fParent = hStack;
    hStack->fChild2->fParent = hStack;
}
