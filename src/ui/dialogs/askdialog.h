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

#ifndef FRICTION_ASK_DIALOG_H
#define FRICTION_ASK_DIALOG_H

#include "ui_global.h"

#include <QMessageBox>
#include <QCheckBox>
#include <QString>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT AskDialog : public QMessageBox
        {
            Q_OBJECT

        public:
            explicit AskDialog(QWidget *parent = nullptr);
            static QMessageBox::StandardButton ask(QWidget *parent,
                                                   const QString &title,
                                                   const QString &text,
                                                   const QString &group,
                                                   const QString &key,
                                                   const QMessageBox::Icon icon = QMessageBox::Question,
                                                   const bool question = true,
                                                   const QMessageBox::StandardButton defaultButton = QMessageBox::Yes);
        private:
            QCheckBox *mCheckBox;
        };
    }
}

#endif // FRICTION_ASK_DIALOG_H
