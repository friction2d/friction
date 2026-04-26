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

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#ifndef SAVEDCOLORSWIDGET_H
#define SAVEDCOLORSWIDGET_H

#include "ui_global.h"

#include <QWidget>

#include "widgets/flowlayout.h"

class SavedColorButton;

class UI_EXPORT SavedColorsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SavedColorsWidget(QWidget *parent = nullptr);

    void addColor(const QColor& color);
    void removeColor(const QColor& color);
    void setColor(const QColor& color);

private:
    void addBookmarkButton();

    FlowLayout *mMainLayout = nullptr;
    QList<SavedColorButton*> mButtons;
    QColor mCurrentColor;

signals:
    void colorSet(QColor);
};

#endif // SAVEDCOLORSWIDGET_H
