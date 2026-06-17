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

#include "askdialog.h"

#include "appsupport.h"

using namespace Friction::Ui;

AskDialog::AskDialog(QWidget *parent)
    : QMessageBox(parent)
    , mCheckBox(nullptr)
{
    mCheckBox = new QCheckBox(tr("Don't ask again"), this);
    setCheckBox(mCheckBox);
}

QMessageBox::StandardButton AskDialog::ask(QWidget *parent,
                                           const QString &title,
                                           const QString &text,
                                           const QString &group,
                                           const QString &key,
                                           const QMessageBox::Icon icon,
                                           const bool question,
                                           const QMessageBox::StandardButton defaultButton)
{
    const int savedResult = AppSupport::getSettings(group,
                                                    key,
                                                    QMessageBox::NoButton).toInt();

    if (savedResult == QMessageBox::Yes ||
        savedResult == QMessageBox::No ||
        savedResult == QMessageBox::Ok) {
        return static_cast<QMessageBox::StandardButton>(savedResult);
    }

    AskDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setText(text);
    dialog.setIcon(icon);

    if (question) {
        dialog.setStandardButtons(QMessageBox::Yes |
                                  QMessageBox::No);
        dialog.setDefaultButton(defaultButton);
    } else {
        dialog.setStandardButtons(QMessageBox::Ok);
        dialog.setDefaultButton(QMessageBox::Ok);
        dialog.checkBox()->setText(tr("Don't show this message again"));
    }

    const int btnResult = dialog.exec();
    const bool dontAsk = dialog.checkBox()->isChecked();
    const bool btnOk = btnResult == QMessageBox::Yes ||
                       btnResult == QMessageBox::No ||
                       btnResult == QMessageBox::Ok;

    if (dontAsk && btnOk) {
        AppSupport::setSettings(group, key, btnResult);
    }

    return static_cast<QMessageBox::StandardButton>(btnResult);
}
