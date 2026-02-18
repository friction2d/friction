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

#ifndef SCROLLWIDGET_H
#define SCROLLWIDGET_H

#include "ui_global.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "minimalscrollwidget.h"
#include "smartPointers/ememory.h"
#include "swt_rulescollection.h"

class SingleWidget;
class ScrollWidgetVisiblePart;
class SWT_Abstraction;
class SingleWidgetTarget;
class ScrollArea;

enum class SWT_BoxRule : short;
enum class SWT_Target : short;
enum class SWT_Type : short;
enum class SWT_ParamRule : short;

class UI_EXPORT ScrollWidget : public MinimalScrollWidget
{
    Q_OBJECT

public:
    explicit ScrollWidget(ScrollWidgetVisiblePart* const visiblePart,
                          ScrollArea * const parent);

    void updateHeight();

    void setMainTarget(SingleWidgetTarget *target);
    void setCurrentRule(const SWT_BoxRule rule);
    void setCurrentTarget(SingleWidgetTarget* targetP,
                          const SWT_Target target);
    void setCurrentSearchText(const QString &text);
    void setCurrentType(const SWT_Type type);
    void setCurrentParamRule(const SWT_ParamRule rule);

    void scheduleContentUpdateIfIsCurrentRule(const SWT_BoxRule rule);
    bool isCurrentRule(const SWT_BoxRule rule);

    SWT_RulesCollection getRulesCollection();
    bool getAlwaysShowChildren();

    SWT_Abstraction* getMainAbstration() const;

    void scheduleContentUpdateIfIsCurrentTarget(SingleWidgetTarget *targetP,
                                                const SWT_Target target);
    int getId() const;
    int visibleCount() const;

    int getContentHeight() { return mContentHeight; }

    void updateVisible();
    const QList<QWidget*> &visibleWidgets();

signals:
    void boxRuleChanged(const SWT_BoxRule rule);

protected:
    ScrollWidgetVisiblePart* visiblePartWidget() const
    { return mVisiblePartWidget; }

private:
    void updateAbstraction();
    void updateHeightAfterScrollAreaResize(const int parentHeight);

    int mContentHeight = 0;
    qptr<SingleWidgetTarget> mMainTarget;
    stdptr<SWT_Abstraction> mMainAbstraction;
    ScrollWidgetVisiblePart * const mVisiblePartWidget;
};

#endif // SCROLLWIDGET_H
