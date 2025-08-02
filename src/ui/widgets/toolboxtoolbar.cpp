/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "toolboxtoolbar.h"
#include "Private/document.h"
#include "Boxes/textbox.h"

using namespace Friction::Ui;

ToolboxToolBar::ToolboxToolBar(const QString &name,
                               const QString &title,
                               QWidget *parent)
    : ToolBar(name, parent, true)
    , mCanvasMode(CanvasMode::boxTransform)
    , mGroupCommon(nullptr)
    , mGroupTransform(nullptr)
    , mGroupPath(nullptr)
    , mGroupCircle(nullptr)
    , mGroupRectangle(nullptr)
    , mGroupText(nullptr)
    , mGroupDraw(nullptr)
    , mGroupPick(nullptr)
    , mGroupSelected(nullptr)
    , mGroupSelectedTransform(nullptr)
    , mGroupSelectedPath(nullptr)
    , mGroupSelectedCircle(nullptr)
    , mGroupSelectedRectangle(nullptr)
    , mGroupSelectedText(nullptr)
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setWindowTitle(title);

    mGroupCommon = new QActionGroup(this);
    mGroupTransform = new QActionGroup(this);
    mGroupPath = new QActionGroup(this);
    mGroupCircle = new QActionGroup(this);
    mGroupRectangle = new QActionGroup(this);
    mGroupText = new QActionGroup(this);
    mGroupDraw = new QActionGroup(this);
    mGroupPick = new QActionGroup(this);
    mGroupSelected = new QActionGroup(this);
    mGroupSelectedTransform = new QActionGroup(this);
    mGroupSelectedPath = new QActionGroup(this);
    mGroupSelectedCircle = new QActionGroup(this);
    mGroupSelectedRectangle = new QActionGroup(this);
    mGroupSelectedText = new QActionGroup(this);

    mGroupCommon->setVisible(false);
    mGroupTransform->setVisible(false);
    mGroupPath->setVisible(false);
    mGroupCircle->setVisible(false);
    mGroupRectangle->setVisible(false);
    mGroupText->setVisible(false);
    mGroupDraw->setVisible(false);
    mGroupPick->setVisible(false);
    mGroupSelected->setVisible(false);
    mGroupSelectedTransform->setVisible(false);
    mGroupSelectedPath->setVisible(false);
    mGroupSelectedCircle->setVisible(false);
    mGroupSelectedRectangle->setVisible(false);
    mGroupSelectedText->setVisible(false);
}

void ToolboxToolBar::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    if (target) {
        mCanvas << connect(mCanvas, &Canvas::currentBoxChanged,
                           this, &ToolboxToolBar::setCurrentBox);
        mCanvas << connect(mCanvas, &Canvas::canvasModeSet,
                           this, &ToolboxToolBar::setCanvasMode);
    }
    setCurrentBox(target ? target->getCurrentBox() : nullptr);
}

void ToolboxToolBar::setCurrentBox(BoundingBox * const target)
{
    mGroupSelected->setVisible(target);
    mGroupSelectedTransform->setVisible(target && mCanvasMode == CanvasMode::boxTransform);
    mGroupSelectedPath->setVisible(target && mCanvasMode == CanvasMode::pointTransform);
    mGroupSelectedCircle->setVisible(target && mCanvasMode == CanvasMode::circleCreate);
    mGroupSelectedRectangle->setVisible(target && mCanvasMode == CanvasMode::rectCreate);

    mGroupSelectedText->setVisible(enve_cast<TextBox*>(target) &&
                                   (mCanvasMode == CanvasMode::boxTransform ||
                                    mCanvasMode == CanvasMode::textCreate));
}

void ToolboxToolBar::setCanvasMode(const CanvasMode &mode)
{
    mCanvasMode = mode;
    mGroupCommon->setVisible(true);
    mGroupTransform->setVisible(mode == CanvasMode::boxTransform);
    mGroupPath->setVisible(mode == CanvasMode::pointTransform);
    mGroupCircle->setVisible(mode == CanvasMode::circleCreate);
    mGroupRectangle->setVisible(mode == CanvasMode::rectCreate ||
                                mode == CanvasMode::circleCreate);
    mGroupText->setVisible(mode == CanvasMode::textCreate);
    mGroupDraw->setVisible(mode == CanvasMode::drawPath);
    mGroupPick->setVisible(mode == CanvasMode::pickFillStroke ||
                           mode == CanvasMode::pickFillStrokeEvent);

    if (mCanvas) { setCurrentBox(mCanvas->getCurrentBox()); }
}

void ToolboxToolBar::addCanvasAction(QAction *action)
{
    if (!action) { return; }
    addAction(mGroupCommon->addAction(action));
}

void ToolboxToolBar::addCanvasAction(const CanvasMode &mode,
                                     QAction *action)
{
    if (!action) { return; }
    switch (mode) {
    case CanvasMode::boxTransform:
        addAction(mGroupTransform->addAction(action));
        break;
    case CanvasMode::pointTransform:
        addAction(mGroupPath->addAction(action));
        break;
    case CanvasMode::circleCreate:
        addAction(mGroupCircle->addAction(action));
        break;
    case CanvasMode::rectCreate:
        addAction(mGroupRectangle->addAction(action));
        break;
    case CanvasMode::textCreate:
        addAction(mGroupText->addAction(action));
        break;
    case CanvasMode::drawPath:
        addAction(mGroupDraw->addAction(action));
        break;
    case CanvasMode::pickFillStroke:
    case CanvasMode::pickFillStrokeEvent:
        addAction(mGroupPick->addAction(action));
        break;
    default:;
    }
}

void ToolboxToolBar::addCanvasSelectedAction(QAction *action)
{
    if (!action) { return; }
    addAction(mGroupSelected->addAction(action));
}

void ToolboxToolBar::addCanvasSelectedAction(const CanvasMode &mode,
                                             QAction *action)
{
    if (!action) { return; }
    switch (mode) {
    case CanvasMode::boxTransform:
        addAction(mGroupSelectedTransform->addAction(action));
        break;
    case CanvasMode::pointTransform:
        addAction(mGroupSelectedPath->addAction(action));
        break;
    case CanvasMode::circleCreate:
        addAction(mGroupSelectedCircle->addAction(action));
        break;
    case CanvasMode::rectCreate:
        addAction(mGroupSelectedRectangle->addAction(action));
        break;
    case CanvasMode::textCreate:
        addAction(mGroupSelectedText->addAction(action));
        break;
    default:;
    }
}

void ToolboxToolBar::addCanvasWidget(QWidget *widget)
{
    if (!widget) { return; }
    mGroupCommon->addAction(addWidget(widget));
}

void ToolboxToolBar::addCanvasWidget(const CanvasMode &mode,
                                     QWidget *widget)
{
    if (!widget) { return; }
    switch (mode) {
    case CanvasMode::boxTransform:
        mGroupTransform->addAction(addWidget(widget));
        break;
    case CanvasMode::pointTransform:
        mGroupPath->addAction(addWidget(widget));
        break;
    case CanvasMode::circleCreate:
        mGroupCircle->addAction(addWidget(widget));
        break;
    case CanvasMode::rectCreate:
        mGroupRectangle->addAction(addWidget(widget));
        break;
    case CanvasMode::textCreate:
        mGroupText->addAction(addWidget(widget));
        break;
    case CanvasMode::drawPath:
        mGroupDraw->addAction(addWidget(widget));
        break;
    case CanvasMode::pickFillStroke:
    case CanvasMode::pickFillStrokeEvent:
        mGroupPick->addAction(addWidget(widget));
        break;
    default:;
    }
}

void ToolboxToolBar::addCanvasSelectedWidget(QWidget *widget)
{
    if (!widget) { return; }
    mGroupSelected->addAction(addWidget(widget));
}

void ToolboxToolBar::addCanvasSelectedWidget(const CanvasMode &mode,
                                             QWidget *widget)
{
    if (!widget) { return; }
    switch (mode) {
    case CanvasMode::boxTransform:
        mGroupSelectedTransform->addAction(addWidget(widget));
        break;
    case CanvasMode::pointTransform:
        mGroupSelectedPath->addAction(addWidget(widget));
        break;
    case CanvasMode::circleCreate:
        mGroupSelectedCircle->addAction(addWidget(widget));
        break;
    case CanvasMode::rectCreate:
        mGroupSelectedRectangle->addAction(addWidget(widget));
        break;
    case CanvasMode::textCreate:
        mGroupSelectedText->addAction(addWidget(widget));
        break;
    default:;
    }
}
