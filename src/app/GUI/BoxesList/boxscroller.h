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

#ifndef BOXSCROLLWIDGETVISIBLEPART_H
#define BOXSCROLLWIDGETVISIBLEPART_H

#include <QWidget>
#include "optimalscrollarena/scrollwidgetvisiblepart.h"
#include "singlewidgettarget.h"
#include "framerange.h"

class BoxSingleWidget;
class TimelineMovable;
class Key;
class KeysView;
class Canvas;
class TimelineHighlightWidget;

class BoxScroller : public ScrollWidgetVisiblePart {
public:
    explicit BoxScroller(ScrollWidget * const parent);

    QWidget *createNewSingleWidget();

    void updateDropTarget();

    void stopScrolling();
    void scrollUp();
    void scrollDown();

    KeysView *getKeysView() const
    { return mKeysView; }

    Scene* currentScene() const
    { return mCurrentScene; }

    void setCurrentScene(Scene* const scene)
    { mCurrentScene = scene; }

    void setKeysView(KeysView *keysView)
    { mKeysView = keysView; }

    TimelineHighlightWidget* requestHighlighter();
protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *e);
    void dropEvent(QDropEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
private:
    enum class DropType {
        none, on, into
    };

    struct DropTarget {
        SWT_Abstraction * fTargetParent = nullptr;
        int fTargetId = 0;
        DropType fDropType = DropType::none;

        bool isValid() const {
            return fTargetParent && fDropType != DropType::none;
        }

        void reset() {
            fTargetParent = nullptr;
            fDropType = DropType::none;
        }
    };

    DropTarget getClosestDropTarget(const int yPos);

    bool tryDropIntoAbs(SWT_Abstraction * const abs,
                        const int idInAbs, DropTarget &dropTarget);

    TimelineHighlightWidget* mHighlighter = nullptr;
    Scene* mCurrentScene = nullptr;

    QRect mCurrentDragRect;
    int mLastDragMoveY;

    QTimer *mScrollTimer = nullptr;
    KeysView *mKeysView = nullptr;

    const QMimeData* mCurrentMimeData = nullptr;

    DropTarget mDropTarget{nullptr, 0, DropType::none};
};

#endif // BOXSCROLLWIDGETVISIBLEPART_H
