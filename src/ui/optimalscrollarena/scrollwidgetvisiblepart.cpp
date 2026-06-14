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

#include "scrollwidgetvisiblepart.h"
#include <QPainter>
#include "swt_abstraction.h"
#include "singlewidget.h"
#include "scrollwidget.h"
#include "singlewidgettarget.h"
#include "GUI/global.h"

int ScrollWidgetVisiblePart::sNextId = 0;

ScrollWidgetVisiblePart::ScrollWidgetVisiblePart(ScrollWidget * const parent) :
    ScrollVisiblePartBase(parent), mId(sNextId++) {
    mRulesCollection.fRule = SWT_BoxRule::all;
    setupUpdateFuncs();
}

void ScrollWidgetVisiblePart::setCurrentRule(const SWT_BoxRule rule) {
    mRulesCollection.fRule = rule;
    updateParentHeightAndContent();
}

void ScrollWidgetVisiblePart::setCurrentTarget(SingleWidgetTarget* targetP,
                                               const SWT_Target target) {
    mRulesCollection.fTarget = target;
    const auto parent = static_cast<ScrollWidget*>(parentWidget());
    parent->setMainTarget(targetP);
    updateParentHeightAndContent();
}

void ScrollWidgetVisiblePart::setCurrentType(const SWT_Type type) {
    mRulesCollection.fType = type;
    updateParentHeightAndContent();
}

void ScrollWidgetVisiblePart::setCurrentParamRule(const SWT_ParamRule rule) {
    mRulesCollection.fParamRule = rule;
    updateParentHeightAndContent();
}

void ScrollWidgetVisiblePart::setupUpdateFuncs() {
    const QPointer<ScrollWidgetVisiblePart> thisQPtr = this;
    mUpdateFuncs.fContentUpdateIfIsCurrentRule =
            [thisQPtr](const SWT_BoxRule rule) {
        if(!thisQPtr) return;
        thisQPtr->scheduleContentUpdateIfIsCurrentRule(rule);
    };
    mUpdateFuncs.fContentUpdateIfIsCurrentTarget =
            [thisQPtr](SingleWidgetTarget* targetP,
            const SWT_Target target) {
        if(!thisQPtr) return;
        thisQPtr->scheduleContentUpdateIfIsCurrentTarget(targetP, target);
    };
    mUpdateFuncs.fContentUpdateIfSearchNotEmpty = [thisQPtr]() {
        if(!thisQPtr) return;
        thisQPtr->scheduleSearchUpdate();
    };
    mUpdateFuncs.fUpdateParentHeight = [thisQPtr]() {
        if(!thisQPtr) return;
        thisQPtr->planScheduleUpdateParentHeight();
    };
    mUpdateFuncs.fUpdateVisibleWidgetsContent = [thisQPtr]() {
        if(!thisQPtr) return;
        thisQPtr->planScheduleUpdateVisibleWidgetsContent();
    };
}

void ScrollWidgetVisiblePart::setAlwaysShowChildren(const bool alwaysShowChildren) {
    mRulesCollection.fAlwaysShowChildren = alwaysShowChildren;
    updateParentHeightAndContent();
}

void ScrollWidgetVisiblePart::setCurrentSearchText(const QString &text) {
    mRulesCollection.fSearchString = text;
    updateParentHeightAndContent();
}

void ScrollWidgetVisiblePart::scheduleContentUpdateIfIsCurrentRule(const SWT_BoxRule rule) {
    if(isCurrentRule(rule)) {
        planScheduleUpdateParentHeight();
        planScheduleUpdateVisibleWidgetsContent();
    }
}

void ScrollWidgetVisiblePart::scheduleContentUpdateIfIsCurrentTarget(
        SingleWidgetTarget* targetP, const SWT_Target target) {
    if(mRulesCollection.fTarget == target) {
        const auto parent = static_cast<ScrollWidget*>(parentWidget());
        parent->setMainTarget(targetP);
        updateParentHeightAndContent();
        //planScheduleUpdateParentHeight();
        //planScheduleUpdateVisibleWidgetsContent();
    }
}

void ScrollWidgetVisiblePart::scheduleSearchUpdate() {
    if(mRulesCollection.fSearchString.isEmpty()) return;
    planScheduleUpdateParentHeight();
    planScheduleUpdateVisibleWidgetsContent();
}

bool ScrollWidgetVisiblePart::isCurrentRule(const SWT_BoxRule rule) {
    return rule == mRulesCollection.fRule;
}

void ScrollWidgetVisiblePart::updateVisibleWidgetsContent() {
    if(!mMainAbstraction) {
        const auto& wids = widgets();
        for(const auto& wid : wids) wid->hide();
        return;
    }

    int currentWidgetId = 0;
    SetAbsFunc setAbsFunc = [this, &currentWidgetId](
            SWT_Abstraction* newAbs, const int currX) {
        const auto& wids = widgets();
        if(currentWidgetId < wids.count()) {
            const auto currWidget = static_cast<SingleWidget*>(
                        wids.at(currentWidgetId));
            currWidget->setTargetAbstraction(newAbs);
            const int currWx = currWidget->x();
            currWidget->move(currX, currWidget->y());
            currWidget->setFixedWidth(currWx - currX + currWidget->width());
            currentWidgetId = currentWidgetId + 1;
            currWidget->setVisible(newAbs);
        }
    };
    int currY = eSizesUI::widget/2;
    const int top = visibleTop();
    const int bottom = top + visibleHeight() + eSizesUI::widget/2;
    mMainAbstraction->setAbstractions(
                top, bottom,
                currY, 0, eSizesUI::widget,
                setAbsFunc, mRulesCollection,
                true, false);

    mVisibleCount = currentWidgetId;
    const auto& wids = widgets();
    const int nWidgets = wids.count();
    for(int i = currentWidgetId; i < nWidgets; i++) {
        wids.at(i)->hide();
    }
}

void ScrollWidgetVisiblePart::setMainAbstraction(SWT_Abstraction* abs) {
    mMainAbstraction = abs;
    planScheduleUpdateVisibleWidgetsContent();
//    if(!abs) return;
//    abs->setContentVisible(true);
//    updateVisibleWidgetsContent();
//    updateParentHeight();
}

QWidget *ScrollWidgetVisiblePart::createNewSingleWidget() {
    return new SingleWidget(this);
}
