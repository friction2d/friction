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

#ifndef BOXESLISTKEYSVIEWWIDGET_H
#define BOXESLISTKEYSVIEWWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QDomElement>

#include "smartPointers/stdselfref.h"
#include "framerange.h"
#include "widgets/fakemenubar.h"
#include "XML/runtimewriteid.h"

#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"

class SWT_Abstraction;
class FrameScrollBar;
class KeysView;
class ChangeWidthWidget;
class MainWindow;
class ScrollArea;
class AnimationDockWidget;
class BoxScrollWidget;
class Document;
class Canvas;
class SceneChooser;
class StackWrapperCornerMenu;
class BoxScroller;
class XevReadBoxesHandler;

enum class SWT_Type : short;
enum class SWT_BoxRule : short;
enum class SWT_Target : short;
enum class SWT_ParamRule : short;

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(Document& document,
                            QWidget * const menu,
                            QWidget *parent);

    ~TimelineWidget();
    Canvas* getCurrrentScene() const {
        return mCurrentScene;
    }

    void setCurrentScene(Canvas* const scene);
    void setBoxesListWidth(const int width);
    void setGraphEnabled(const bool enabled);

    void writeState(eWriteStream& dst) const;
    void readState(eReadStream& src);

    void readStateXEV(XevReadBoxesHandler& boxReadHandler,
                      const QDomElement& ele,
                      RuntimeIdToWriteId& objListIdConv);
    void writeStateXEV(QDomElement& ele, QDomDocument& doc,
                       RuntimeIdToWriteId& objListIdConv) const;
    void readSettings(ChangeWidthWidget *chww);
    void writeSettings();

private:
    void setViewedFrameRange(const FrameRange &range);
    void setCanvasFrameRange(const FrameRange &range);

    void setSearchText(const QString &text);
    void moveSlider(int val);
signals:
    void typeChanged(const SWT_Type target);
    void targetChanged(const SWT_Target target);
    void boxRuleChanged(const SWT_BoxRule rule);
    void paramRuleChanged(const SWT_ParamRule rule);
private:
    void setType(const SWT_Type type);
    void setBoxRule(const SWT_BoxRule rule);
    void setTarget(const SWT_Target target);
    void setParamRule(const SWT_ParamRule rule);

    Canvas* mCurrentScene = nullptr;

    Document& mDocument;

    SceneChooser* mSceneChooser;

    FrameScrollBar* mFrameScrollBar;
    FrameScrollBar* mFrameRangeScrollBar;

    QGridLayout *mMainLayout;
    QVBoxLayout *mBoxesListLayout;
    QVBoxLayout *mKeysViewLayout;
    QHBoxLayout *mMenuLayout;
    QHBoxLayout *mMenuWidgetsLayout;
    QAction *mGraphAct = nullptr;
    QWidget* mMenuWidget;
    QWidget *mMenuWidgetsCont;
    FakeMenuBar *mBoxesListMenuBar;
    FakeMenuBar *mCornerMenuBar;
    QLineEdit *mSearchLine;
    ScrollArea *mBoxesListScrollArea;
    BoxScrollWidget *mBoxesListWidget;
    KeysView *mKeysView;
    AnimationDockWidget *mAnimationDockWidget;
};

#endif // BOXESLISTKEYSVIEWWIDGET_H
