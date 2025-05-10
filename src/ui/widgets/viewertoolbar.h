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

#ifndef FRICTION_VIEWER_TOOLBAR_H
#define FRICTION_VIEWER_TOOLBAR_H

#include "ui_global.h"

#include "widgets/toolbar.h"

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ViewerToolBar : public ToolBar
        {
        public:
            explicit ViewerToolBar(const QString &name,
                                   const QString &title,
                                   QWidget *parent = nullptr);
        };
    }
}

#endif // FRICTION_VIEWER_TOOLBAR_H
