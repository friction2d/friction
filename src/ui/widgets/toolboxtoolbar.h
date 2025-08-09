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

#ifndef FRICTION_TOOLBOX_TOOLBAR_H
#define FRICTION_TOOLBOX_TOOLBAR_H

#include "ui_global.h"

#include "widgets/toolbar.h"

#include "canvas.h"

#include <QActionGroup>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ToolboxToolBar : public ToolBar
        {
            Q_OBJECT

        public:
            explicit ToolboxToolBar(const QString &name,
                                    const QString &title,
                                    QWidget *parent = nullptr);
            void setCurrentCanvas(Canvas * const target);
            void setCurrentBox(BoundingBox * const target);
            void setCanvasMode(const CanvasMode &mode);

            void addCanvasAction(QAction *action);
            void addCanvasAction(const CanvasMode &mode,
                                 QAction *action);
            void addCanvasSelectedAction(QAction *action);
            void addCanvasSelectedAction(const CanvasMode &mode,
                                         QAction *action);
            void addCanvasWidget(QWidget *widget);
            void addCanvasWidget(const CanvasMode &mode,
                                 QWidget *widget);
            void addCanvasSelectedWidget(QWidget *widget);
            void addCanvasSelectedWidget(const CanvasMode &mode,
                                         QWidget *widget);

        private:
            ConnContextQPtr<Canvas> mCanvas;
            CanvasMode mCanvasMode;

            QActionGroup *mGroupCommon;
            QActionGroup *mGroupTransform;
            QActionGroup *mGroupPath;
            QActionGroup *mGroupCircle;
            QActionGroup *mGroupRectangle;
            QActionGroup *mGroupText;
            QActionGroup *mGroupDraw;
            QActionGroup *mGroupPick;
            QActionGroup *mGroupSelected;
            QActionGroup *mGroupSelectedTransform;
            QActionGroup *mGroupSelectedPath;
            QActionGroup *mGroupSelectedCircle;
            QActionGroup *mGroupSelectedRectangle;
            QActionGroup *mGroupSelectedText;
        };
    }
}

#endif // FRICTION_TOOLBOX_TOOLBAR_H
