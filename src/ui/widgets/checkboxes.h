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

#ifndef FRICTION_CHECKBOXES_WIDGET_H
#define FRICTION_CHECKBOXES_WIDGET_H

#include "ui_global.h"

#include <QWidget>
#include <QStringList>
#include <QCheckBox>
#include <QPair>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT CheckBoxes : public QWidget
        {
            Q_OBJECT
            Q_PROPERTY(QStringList selectedBoxes READ selectedBoxes NOTIFY boxesChanged)

        public:
            CheckBoxes(const QList<QPair<QString, QString>>& items,
                       QWidget *parent = nullptr,
                       const int maxColumns = 2);

            QStringList selectedBoxes() const;

        signals:
            void boxesChanged();

        private:
            QList<QPair<QCheckBox*, QString>> mCheckboxes;
        };
    }
}

#endif // FRICTION_CHECKBOXES_WIDGET_H
