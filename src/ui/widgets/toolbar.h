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

#ifndef FRICTION_TOOLBAR_H
#define FRICTION_TOOLBAR_H

#include "ui_global.h"

#include <QToolBar>
#include <QAction>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ToolBar : public QToolBar
        {
            Q_OBJECT
        public:
            explicit ToolBar(const QString &title,
                             QWidget *parent = nullptr,
                             const bool &iconsOnly = false);
            explicit ToolBar(const QString &title,
                             const QString &objectName,
                             QWidget *parent = nullptr,
                             const bool iconsOnly = false);
            void updateActions();
            QAction* addSpacer(const bool &horizontal = true,
                               const bool &fixed = false,
                               const int &width = 0,
                               const int &height = 0);

        private:
            bool mIconsOnly;
            void setup();
            void showContextMenu(const QPoint &pos);
            QAction* addSpacer(QWidget *widget);
            void setEnableAction(const QString &title,
                                 const bool &enable);
        };
    }
}

#endif // FRICTION_TOOLBAR_H
