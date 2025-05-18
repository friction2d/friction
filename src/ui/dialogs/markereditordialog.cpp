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

#include "markereditordialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QIcon>

#include "widgets/markereditor.h"

using namespace Friction::Ui;

MarkerEditorDialog::MarkerEditorDialog(Scene *scene,
                                       QWidget *parent)
    : Dialog{parent}
{
    setWindowTitle(tr("Marker Editor"));

    setContentsMargins(0, 0, 0, 0);
    const auto lay = new QVBoxLayout(this);
    const auto editor = new MarkerEditor(scene, this);
    const auto footer = new QHBoxLayout();

    const auto addButton = new QPushButton(QIcon::fromTheme("plus"),
                                           QString(),
                                           this);
    const auto remButton = new QPushButton(QIcon::fromTheme("minus"),
                                           QString(),
                                           this);
    const auto clearButton = new QPushButton(QIcon::fromTheme("trash"),
                                             QString(),
                                             this);
    const auto closeButton = new QPushButton(QIcon::fromTheme("close"),
                                             tr("Close"),
                                             this);

    addButton->setToolTip(tr("Add a new marker"));
    remButton->setToolTip(tr("Remove the selected marker from the list"));
    clearButton->setToolTip(tr("Clear all markers"));

    addButton->setFocusPolicy(Qt::NoFocus);
    remButton->setFocusPolicy(Qt::NoFocus);
    clearButton->setFocusPolicy(Qt::NoFocus);
    closeButton->setFocusPolicy(Qt::StrongFocus);
    closeButton->setFocus();

    connect(addButton, &QPushButton::clicked,
            editor, &MarkerEditor::addMarker);
    connect(remButton, &QPushButton::clicked,
            editor, &MarkerEditor::removeMarker);
    connect(clearButton, &QPushButton::clicked,
            editor, &MarkerEditor::clearMarkers);
    connect(closeButton, &QPushButton::clicked,
            this, &QDialog::close);

    footer->addWidget(addButton);
    footer->addWidget(remButton);
    footer->addWidget(clearButton);
    footer->addStretch();
    footer->addWidget(closeButton);

    lay->addWidget(editor);
    lay->addLayout(footer);
}
