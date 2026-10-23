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

#ifndef SCROLLWIDGETVISIBLEPART_H
#define SCROLLWIDGETVISIBLEPART_H

#include "ui_global.h"

#include "scrollvisiblepartbase.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QtMath>
#include <memory>
#include "smartPointers/ememory.h"
#include "singlewidgettarget.h"

class ScrollWidget;
class SingleWidgetTarget;

class UI_EXPORT ScrollWidgetVisiblePart : public ScrollVisiblePartBase
{
    Q_OBJECT

public:
    ScrollWidgetVisiblePart(ScrollWidget * const parent);

    virtual QWidget *createNewSingleWidget();
    void updateVisibleWidgetsContent();

    void setMainAbstraction(SWT_Abstraction *abs);
    void setCurrentRule(const SWT_BoxRule rule);
    void setCurrentTarget(SingleWidgetTarget *targetP,
                          const SWT_Target target);
    void setAlwaysShowChildren(const bool alwaysShowChildren);
    void setCurrentSearchText(const QString &text);
    void setCurrentType(const SWT_Type type);
    void setCurrentParamRule(const SWT_ParamRule rule);

    void scheduleContentUpdateIfIsCurrentRule(const SWT_BoxRule rule);
    bool isCurrentRule(const SWT_BoxRule rule);

    SWT_RulesCollection getRulesCollection()
    { return mRulesCollection; }

    bool getAlwaysShowChildren()
    { return mAlwaysShowChildren; }

    void scheduleContentUpdateIfIsCurrentTarget(SingleWidgetTarget *targetP,
                                                const SWT_Target target);
    int getId() const { return mId; }
    int visibleCount() const { return mVisibleCount; }

    const UpdateFuncs& getUpdateFuncs() const
    { return mUpdateFuncs; }

    SWT_Abstraction* getMainAbstration() const
    { return mMainAbstraction; }

private:
    void setupUpdateFuncs();
    void scheduleSearchUpdate();

    UpdateFuncs mUpdateFuncs;
    static int sNextId;
    const int mId;
    int mVisibleCount = 0;
    bool mAlwaysShowChildren = false;
    SWT_RulesCollection mRulesCollection;
    stdptr<SWT_Abstraction> mMainAbstraction;
};


#endif // SCROLLWIDGETVISIBLEPART_H
