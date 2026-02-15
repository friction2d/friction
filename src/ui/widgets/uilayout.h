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

#ifndef UILAYOUT_H
#define UILAYOUT_H

#include "ui_global.h"

#include <QWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <vector>

class UI_EXPORT UIDock : public QWidget
{
    Q_OBJECT

public:
    enum Position
    {
        Left,
        Right,
        Up,
        Down
    };
    explicit UIDock(QWidget *parent = nullptr,
                    QWidget *widget = nullptr,
                    const QString &label = tr("Dock"),
                    const Position &pos = Position::Left,
                    const bool &showLabel = true,
                    const bool &showHeader = true,
                    const bool &darkHeader = false);
    ~UIDock();
    void setPosition(const Position &pos);
    Position getPosition();
    void setIndex(const int &index);
    int getIndex();
    const QString getLabel();
    const QString getId();
    void addWidget(QWidget *widget);

signals:
    void changePosition(const UIDock::Position &pos,
                        const UIDock::Position &trigger);

private:
    QVBoxLayout *mLayout;
    QString mLabel;
    Position mPos;
    int mIndex;
    void writeSettings();
};

class UI_EXPORT UILayout : public QSplitter
{
    Q_OBJECT

public:
    struct Item
    {
        int pos = UIDock::Position::Left;
        int index = -1;
        QString label;
        QWidget *widget;
        bool showLabel = true;
        bool showHeader = true;
        bool darkHeader = false;
        bool operator<(const Item &a) const
        {
            return index < a.index;
        }
    };
    explicit UILayout(QWidget *parent = nullptr);
    ~UILayout();
    void readSettings();
    void writeSettings();
    void addDocks(std::vector<Item> items);
    void setDockVisible(const QString &label,
                        bool visible);
    void addDockWidget(const QString &label,
                       QWidget *widget);

signals:
    void updateDockVisibility(const QString &label,
                              bool visible);
    void updateDockWidget(const QString &label,
                          QWidget *widget);

private:
    QSplitter *mLeft;
    QSplitter *mMiddle;
    QSplitter *mRight;
    QSplitter *mTop;
    QSplitter *mBottom;
    void addDock(const Item &item);
    void connectDock(UIDock *dock);
    void updateDock(QSplitter *container,
                    const UIDock::Position &pos);
    void updateDocks();
};

#endif // UILAYOUT_H
