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

#include "checkboxes.h"

#include <QGridLayout>

using namespace Friction::Ui;

CheckBoxes::CheckBoxes(const QList<QPair<QString, QString> > &items,
                       QWidget *parent,
                       const int maxColumns)
    : QWidget(parent)
{
    setContentsMargins(0, 0, 0, 0);
    const auto grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);

    int row = 0;
    int col = 0;

    for (const auto &item : items) {
        const auto check = new QCheckBox(item.first, this);
        check->setChecked(true);

        mCheckboxes.append({check, item.second});
        grid->addWidget(check, row, col);

        connect(check, &QCheckBox::toggled,
                this, &CheckBoxes::boxesChanged);

        col++;
        if (col >= maxColumns) {
            col = 0;
            row++;
        }
    }
}

QStringList CheckBoxes::selectedBoxes() const
{
    QStringList selected;
    for (const auto &pair : mCheckboxes) {
        if (pair.first->isChecked()) { selected.append(pair.second); }
    }
    return selected;
}
