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

#ifndef FRICTION_TOOLBOX_H
#define FRICTION_TOOLBOX_H

#include "ui_global.h"

#include <QObject>
#include <QActionGroup>
#include <QPushButton>
#include <QLabel>

#include "Private/document.h"

#include "widgets/toolbar.h"
#include "widgets/toolboxtoolbar.h"
#include "widgets/toolcontrols.h"
#include "widgets/qdoubleslider.h"

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ToolBox : public QObject
        {
            Q_OBJECT
        public:
            enum Type {
                Main,
                Controls,
                Extra
            };
            enum Node {
                NodeConnect,
                NodeDisconnect,
                NodeMerge,
                NodeNew,
                NodeSymmetric,
                NodeSmooth,
                NodeCorner,
                NodeSegmentLine,
                NodeSegmentCurve
            };

            explicit ToolBox(Actions &actions,
                             Document &document,
                             QWidget *parent);

            QToolBar *getToolBar(const Type &type);

            const QList<QAction*> getMainActions();
            const QList<QAction*> getNodeActions();

            void setMovable(const bool movable);

        private:
            Actions &mActions;
            Document &mDocument;
            ToolBar *mMain;
            ToolControls *mControls;
            ToolboxToolBar *mExtra;

            QActionGroup *mGroupMain;
            QActionGroup *mGroupNodes;
            QActionGroup *mGroupDraw;
            QActionGroup *mGroupColorPicker;

            QDoubleSlider *mDrawPathMaxError;
            QDoubleSlider *mDrawPathSmooth;

            QAction *mLocalPivot;

            QToolButton *mColorPickerButton;
            QLabel *mColorPickerLabel;

            void setupToolBox(QWidget *parent);
            void setupDocument();
            void setupMainAction(const QIcon &icon,
                                 const QString &title,
                                 const QKeySequence &shortcut,
                                 const QList<CanvasMode> &modes,
                                 const bool checked);
            void setupMainActions();
            void setupNodesAction(const QIcon &icon,
                                  const QString &title,
                                  const Node &node);
            void setupNodesActions();
            void setupDrawActions();
            void setupColorPickerActions();

            void setCurrentCanvas(Canvas * const target);
            void setCanvasMode(const CanvasMode &mode);

            void updateColorPicker(const QColor &color);
        };
    }
}

#endif // FRICTION_TOOLBOX_H
