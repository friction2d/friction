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

#ifndef FRICTION_TOOL_INTERACT_H
#define FRICTION_TOOL_INTERACT_H

#include "ui_global.h"

#include "widgets/toolbar.h"
#include "gizmos.h"
#include "grid.h"

#include <QToolButton>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ToolInteract : public ToolBar
        {
            Q_OBJECT

        public:
            explicit ToolInteract(QWidget *parent = nullptr);

        private:
            void setupGizmoButton();
            void setupGizmoAction(QToolButton *button,
                                  const Core::Gizmos::Interact &ti);
            void setupSnapButton();
            void setupSnapAction(QToolButton *button,
                                 const Core::Grid::Option &option);
            void setupGridButton();
            void setupGridAction(QToolButton *button,
                                 const Core::Grid::Option &option);

        };
    }
}

#endif // FRICTION_TOOL_INTERACT_H
