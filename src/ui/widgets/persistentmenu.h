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

#ifndef FRICTION_PERSISTENTMENU_H
#define FRICTION_PERSISTENTMENU_H

#include "ui_global.h"

#include <QMenu>
#include <QMouseEvent>
#include <QAction>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT PersistentMenu : public QMenu
        {
            Q_OBJECT
        public:
            using QMenu::QMenu;

            PersistentMenu* addPersistentMenu(const QIcon &icon,
                                              const QString &title,
                                              const bool onlyCheckable = false)
            {
                PersistentMenu *subMenu = new PersistentMenu(title, this);
                subMenu->setOnlyCheckable(onlyCheckable);
                subMenu->setIcon(icon);
                this->addMenu(subMenu);
                return subMenu;
            }
            void setOnlyCheckable(const bool checked)
            {
                mOnlyCheckable = checked;
            }

        private:
            bool mOnlyCheckable = false;

        protected:
            void mouseReleaseEvent(QMouseEvent *event) override
            {
                QAction *action = actionAt(event->pos());
                if (action && action->menu()) {
                    QMenu::mouseReleaseEvent(event);
                    return;
                }
                if ((action && !mOnlyCheckable) ||
                    (action && mOnlyCheckable && action->isCheckable())) {
                    action->activate(QAction::Trigger);
                } else {
                    QMenu::mouseReleaseEvent(event);
                }
            }
        };
    }
}

#endif // FRICTION_PERSISTENTMENU_H
