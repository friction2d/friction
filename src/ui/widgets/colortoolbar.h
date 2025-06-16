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

#ifndef FRICTION_COLOR_TOOLBAR_H
#define FRICTION_COLOR_TOOLBAR_H

#include "ui_global.h"

#include "Private/scene.h"
#include "Private/document.h"
#include "widgets/toolbar.h"
#include "widgets/colortoolbutton.h"

#include <QToolBar>
#include <QAction>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ColorToolBar : public QToolBar
        {
            Q_OBJECT
        public:
            explicit ColorToolBar(Document& document,
                                  QWidget *parent = nullptr);
            void setCurrentCanvas(Scene * const target);
            void setCurrentBox(BoundingBox *target);

        signals:
            void message(const QString &msg);

        private:
            void setupWidgets(Document& document);
            void adjustWidgets();
            QAction* addSpacer(QWidget *widget);
            ColorToolButton *mColorFill;
            ColorToolButton *mColorStroke;
            ColorToolButton *mColorBackground;
            QAction *mColorFillAct;
            QAction *mColorStrokeAct;
            QAction *mColorBackgroundAct;
            QWidget *mLeftSpacer;
            QWidget *mRightSpacer;
            QAction *mLeftSpacerAct;
            QAction *mRightSpacerAct;
            ConnContextQPtr<Scene> mScene;

            void showContextMenu(const QPoint &pos);
        };
    }
}

#endif // FRICTION_COLOR_TOOLBAR_H
