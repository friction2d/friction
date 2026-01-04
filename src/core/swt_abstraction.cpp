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

#include "swt_abstraction.h"
#include "singlewidgettarget.h"
#include "Animators/customproperties.h"
#include "BlendEffects/blendeffectcollection.h"
#include "BlendEffects/blendeffectboxshadow.h"

SWT_Abstraction::SWT_Abstraction(
        SingleWidgetTarget * const target,
        const UpdateFuncs &updateFuncs,
        const int visiblePartId) :
    mVisiblePartWidgetId(visiblePartId),
    mUpdateFuncs(updateFuncs),
    mTarget_k(target) {}

bool SWT_Abstraction::setAbstractions(
        const int minY, const int maxY,
        int &currY, int currX, const int swtHeight,
        const SetAbsFunc& setAbsFunc,
        const SWT_RulesCollection &rules,
        const bool parentSatisfiesRule,
        const bool parentMainTarget) { // returns whether should abort
    if(!mTarget_k->SWT_isVisible()) return false;
    if(currY > maxY) return true;
    const bool satisfiesRule = mTarget_k->SWT_shouldBeVisible(
                rules, parentSatisfiesRule, parentMainTarget);
    const bool passThrough = (parentSatisfiesRule || parentMainTarget) &&
                             !satisfiesRule &&
                             mTarget_k->SWT_shouldPassThrough(rules);
    if(currY > minY && satisfiesRule && !mIsMainTarget) {
        setAbsFunc(this, currX);
    }
    if(satisfiesRule && !mIsMainTarget) {
        currX += swtHeight;
        currY += swtHeight;
    }
    const bool childrenVisible = (satisfiesRule && mContentVisible) ||
                                 passThrough || mIsMainTarget;
    for(const auto& abs : mChildren) {
        if(abs->setAbstractions(minY, maxY, currY, currX,
                                swtHeight, setAbsFunc, rules,
                                childrenVisible, mIsMainTarget)) {
            return true;
        }
    }

    return false;
}

int SWT_Abstraction::updateHeight(const SWT_RulesCollection &rules,
                                  const bool parentSatisfiesRule,
                                  const bool parentMainTarget,
                                  const int swtHeight) {
    mHeight = 0;
    if(mTarget_k->SWT_isVisible()) {
        const bool satisfiesRule = mTarget_k->SWT_shouldBeVisible(
                    rules, parentSatisfiesRule, parentMainTarget);
        const bool passThrough = (parentSatisfiesRule || parentMainTarget) &&
                                 !satisfiesRule &&
                                 mTarget_k->SWT_shouldPassThrough(rules);
        if(satisfiesRule && !mIsMainTarget)
            mHeight += swtHeight;
        const bool childrenVisible = (satisfiesRule && mContentVisible) ||
                                     passThrough || mIsMainTarget;
        if(childrenVisible) {
            for(const auto& abs : mChildren) {
                mHeight += abs->updateHeight(rules, childrenVisible,
                                             mIsMainTarget, swtHeight);
            }
        }
    }

    return mHeight;
}

void SWT_Abstraction::addChildAbstraction(
        const stdsptr<SWT_Abstraction>& abs) {
    addChildAbstractionAt(abs, mChildren.count());
}

void SWT_Abstraction::addChildAbstractionAt(
        const stdsptr<SWT_Abstraction>& abs, const int id) {
    mChildren.insert(id, abs);
    abs->setParent(this);
    updateChildrenIds(id, mChildren.count() - 1);

    mUpdateFuncs.fUpdateVisibleWidgetsContent();
    mUpdateFuncs.fUpdateParentHeight();
}

SWT_Abstraction *SWT_Abstraction::getChildAbsFor(
        const SingleWidgetTarget* const target) {
    for(const auto& abs : mChildren) {
        if(abs->getTarget() == target) return abs.get();
    }
    return nullptr;
}

void SWT_Abstraction::removeChild(
        const SingleWidgetTarget* const target) {
    const auto childAbs = getChildAbsFor(target);
    if(childAbs) removeChild(childAbs->ref<SWT_Abstraction>());
}

void SWT_Abstraction::removeChild(const stdsptr<SWT_Abstraction>& abs) {
    const int currId = abs->getIdInParent();
    mChildren.removeOne(abs);
    if(abs->getParent() == this) abs->setParent(nullptr);
    updateChildrenIds(currId, mChildren.count() - 1);

    mUpdateFuncs.fUpdateVisibleWidgetsContent();
    mUpdateFuncs.fUpdateParentHeight();
}

void SWT_Abstraction::switchContentVisible() {
    setContentVisible(!mContentVisible);
}

bool SWT_Abstraction::contentVisible() {
    return mContentVisible;
}

void SWT_Abstraction::scheduleContentUpdate(const SWT_BoxRule rule) {
    mUpdateFuncs.fContentUpdateIfIsCurrentRule(rule);
}

void SWT_Abstraction::scheduleSearchContentUpdate() {
    mUpdateFuncs.fContentUpdateIfSearchNotEmpty();
}

void SWT_Abstraction::setContentVisible(const bool visible) {
    //if(bT == mContentVisible) return;
    mContentVisible = visible;
    afterContentVisibilityChanged();
}

SingleWidgetTarget *SWT_Abstraction::getTarget() const {
    return mTarget_k;
}

void SWT_Abstraction::afterContentVisibilityChanged() {
    mUpdateFuncs.fUpdateParentHeight();
    mUpdateFuncs.fUpdateVisibleWidgetsContent();
}

void SWT_Abstraction::removeAlongWithAllChildren_k() {
    for(const auto& child : mChildren) child->removeAlongWithAllChildren_k();
    mChildren.clear();
    if(mTarget_k) mTarget_k->SWT_removeAbstractionForWidget(mVisiblePartWidgetId);
}

void SWT_Abstraction::setParent(SWT_Abstraction * const parent) {
    mParent = parent;
}

void SWT_Abstraction::setIdInParent(const int id) {
    mIdInParent = id;
}

void SWT_Abstraction::updateChildrenIds(const int minId, const int maxId) const {
    for(int i = minId; i <= maxId; i++) {
        mChildren.at(i)->setIdInParent(i);
    }
}

void SWT_Abstraction::addChild(SingleWidgetTarget * const target) {
    auto childAbs = target->SWT_abstractionForWidget(mUpdateFuncs, mVisiblePartWidgetId);
    addChildAbstraction(childAbs->ref<SWT_Abstraction>());
}

void SWT_Abstraction::addChildAt(SingleWidgetTarget * const target, const int id) {
    auto childAbs = target->SWT_abstractionForWidget(mUpdateFuncs, mVisiblePartWidgetId);
    addChildAbstractionAt(childAbs->ref<SWT_Abstraction>(), id);
}

void SWT_Abstraction::moveChildTo(SingleWidgetTarget * const target, const int id) {
    const auto abs = getChildAbsFor(target);
    int targetId = id;
//    if(!abs->getTarget()->SWT_visibleOnlyIfParentDescendant()) {
//        for(int i = 0; i < mChildren.count(); i++) {
//            const stdsptr<SWT_Abstraction>& abs1 = mChildren.at(i);
//            if(abs1->getTarget()->SWT_visibleOnlyIfParentDescendant()) {
//                targetId++;
//            } else {
//                break;
//            }
//        }
//    }
    int currId = -1;
    for(int i = 0; i < mChildren.count(); i++) {
        const auto& absT = mChildren.at(i);
        if(abs == absT.get()) {
            currId = i;
            break;
        }
    }
    if(currId == -1) return;
    mChildren.move(currId, targetId);
    updateChildrenIds(qMin(currId, targetId), qMax(currId, targetId));

    mUpdateFuncs.fUpdateVisibleWidgetsContent();
    mUpdateFuncs.fUpdateParentHeight();
}
