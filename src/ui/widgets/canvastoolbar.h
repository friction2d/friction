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

#ifndef FRICTION_CANVAS_TOOLBAR_H
#define FRICTION_CANVAS_TOOLBAR_H

#include "ui_global.h"

#include "canvas.h"
#include "widgets/colortoolbutton.h"

#include <QToolBar>
#include <QSpinBox>
#include <QComboBox>
#include <QAction>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT CanvasToolBar : public QToolBar
        {
        public:
            explicit CanvasToolBar(QWidget *parent = nullptr);
            void setCurrentCanvas(Canvas * const target);
            QComboBox* getResolutionComboBox();
            void setMemoryUsage(const intMB &usage);

        private:
            void setupDimensions();
            void setupResolution();
            void addSpacer();
            void updateWidgets(Canvas * const target);
            void updateDimension(const QSize dim);
            void setResolution(QString text,
                               Canvas * const target);
            void setDimension(const QSize dim,
                              Canvas * const target);
            void showContextMenu(const QPoint &pos);

            QSpinBox *mSpinWidth;
            QSpinBox *mSpinHeight;
            QComboBox *mComboResolution;
            QAction *mMemoryLabel;

            ConnContextQPtr<Canvas> mCanvas;
            bool mIconsOnly;
        };
    }
}

#endif // FRICTION_CANVAS_TOOLBAR_H
