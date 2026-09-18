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

#include "scrollwidget.h"
#include "scrollwidgetvisiblepart.h"
#include "singlewidget.h"
#include "swt_abstraction.h"
#include "singlewidgettarget.h"
#include "scrollarea.h"
#include "GUI/global.h"

ScrollWidget::ScrollWidget(ScrollWidgetVisiblePart * const visiblePart,
                           ScrollArea * const parent) :
    MinimalScrollWidget(visiblePart, parent),
    mVisiblePartWidget(visiblePart) {
    connect(parent, &ScrollArea::heightChanged,
            this, &ScrollWidget::updateHeightAfterScrollAreaResize);
    connect(&eSizesUI::widget, &SizeSetter::sizeChanged,
            this, &ScrollWidget::updateHeight);
}

void ScrollWidget::updateAbstraction() {
    if(mMainAbstraction) mMainAbstraction->setIsMainTarget(false);
    if(!mMainTarget) {
        mMainAbstraction = nullptr;
    } else {
        const int widId = mVisiblePartWidget->getId();
        mMainAbstraction = mMainTarget->SWT_getAbstractionForWidget(widId);
        mMainAbstraction->setIsMainTarget(true);
    }
    mVisiblePartWidget->setMainAbstraction(mMainAbstraction);
    updateHeight();
}

void ScrollWidget::setMainTarget(SingleWidgetTarget *target) {
    mMainTarget = target;
    updateAbstraction();
}

void ScrollWidget::setCurrentRule(const SWT_BoxRule rule) {
    mVisiblePartWidget->setCurrentRule(rule);
    emit boxRuleChanged(rule);
}

void ScrollWidget::setCurrentTarget(SingleWidgetTarget *targetP,
                                    const SWT_Target target) {
    mVisiblePartWidget->setCurrentTarget(targetP, target);
}

void ScrollWidget::setCurrentSearchText(const QString &text) {
    mVisiblePartWidget->setCurrentSearchText(text);
}

void ScrollWidget::setCurrentType(const SWT_Type type) {
    mVisiblePartWidget->setCurrentType(type);
}

void ScrollWidget::setCurrentParamRule(const SWT_ParamRule rule) {
    mVisiblePartWidget->setCurrentParamRule(rule);
}

SWT_RulesCollection ScrollWidget::getRulesCollection()
{ return mVisiblePartWidget->getRulesCollection(); }

bool ScrollWidget::getAlwaysShowChildren()
{ return mVisiblePartWidget->getAlwaysShowChildren();; }

SWT_Abstraction *ScrollWidget::getMainAbstration() const
{ return mMainAbstraction; }

void ScrollWidget::scheduleContentUpdateIfIsCurrentTarget(
        SingleWidgetTarget *targetP, const SWT_Target target) {
    mVisiblePartWidget->scheduleContentUpdateIfIsCurrentTarget(
                targetP, target);
}

int ScrollWidget::getId() const {
    return mVisiblePartWidget->getId();
}

int ScrollWidget::visibleCount() const {
    return mVisiblePartWidget->visibleCount();
}

void ScrollWidget::updateVisible() {
    mVisiblePartWidget->update();
}

const QList<QWidget*>& ScrollWidget::visibleWidgets() {
    return mVisiblePartWidget->widgets();
}

void ScrollWidget::updateHeight() {
    if(!mMainAbstraction) return hide();
    mContentHeight = mMainAbstraction->updateHeight(
                mVisiblePartWidget->getRulesCollection(),
                false, false, eSizesUI::widget) + 3*eSizesUI::widget/2;
    setFixedHeight(qMax(mContentHeight, minHeight()));
    if(isHidden()) show();
}

void ScrollWidget::updateHeightAfterScrollAreaResize(const int parentHeight) {
    setFixedHeight(qMax(mContentHeight, parentHeight));
}
