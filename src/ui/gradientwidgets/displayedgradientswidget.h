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

#ifndef DISPLACYEDGRADIENTSWIDGET_H
#define DISPLACYEDGRADIENTSWIDGET_H

#include "ui_global.h"

#include "widgets/glwidget.h"
#include "GUI/global.h"
#include "conncontextobjlist.h"
#include "conncontextptr.h"

class Gradient;
class GradientWidget;
class Canvas;

class UI_EXPORT DisplayedGradientsWidget : public GLWidget
{
    Q_OBJECT

public:
    explicit DisplayedGradientsWidget(QWidget *parent = nullptr);

    void setScene(Canvas* const scene);

    void incTop(const int inc);
    void setTop(const int top);

    void setSelectedGradient(Gradient *gradient);

    void newGradient();
    void addGradient(Gradient* const gradient);
    void removeGradient(Gradient* const gradient);
    void removeSelectedGradient();
    void duplicateSelectedGradient();
    void bookmarkColorsSelectedGradient();

    void updateHeight();

signals:
    void selectionChanged(Gradient*);
    void triggered(Gradient*);

protected:
    void paintGL();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);

private:
    void updateTopGradientId();
    void gradientContextMenuReq(const int gradId, const QPoint &globalPos);
    void gradientLeftPressed(const int gradId);

    ConnContextQPtr<Canvas> mScene;

    int mContextMenuGradientId = -1;
    int mDisplayedTop = 0;
    int mTopGradientId = 0;
    int mHoveredGradientId = -1;

    int mMaxVisibleGradients = 6;

    QPointer<Gradient> mSelectedGradient;
    ConnContextObjList<Gradient*> mGradients;
};

#endif // DISPLACYEDGRADIENTSWIDGET_H
