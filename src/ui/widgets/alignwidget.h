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

#ifndef FRICTION_ALIGN_WIDGET_H
#define FRICTION_ALIGN_WIDGET_H

#include "ui_global.h"

#include <QWidget>
#include <QComboBox>
#include <QToolBar>
#include <QPushButton>

#include "canvas.h"

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT AlignWidget : public QWidget
        {
            Q_OBJECT

        public:
            explicit AlignWidget(QWidget* const parent = nullptr,
                                 QToolBar* const toolbar = nullptr);

        signals:
            void alignTriggered(const Qt::Alignment,
                                const AlignPivot,
                                const AlignRelativeTo);

        private:
            void setup();
            void setupToolbar();
            QAction* addAlignAction(const Qt::Alignment &align,
                                    const QString &icon,
                                    const QString &title);
            QPushButton* addAlignButton(const Qt::Alignment &align,
                                        const QString &icon,
                                        const QString &title);
            void connectAlignPivot();
            void triggerAlign(const Qt::Alignment align);
            void setComboBoxItemState(QComboBox *box,
                                      int index,
                                      bool enabled);

            QComboBox *mAlignPivot;
            QComboBox *mRelativeTo;
            QToolBar *mToolbar;
        };
    }
}

#endif // FRICTION_ALIGN_WIDGET_H
