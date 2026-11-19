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

#ifndef SWT_ABSTRACTION_H
#define SWT_ABSTRACTION_H

#include "smartPointers/ememory.h"

class SingleWidgetTarget;

enum class SWT_BoxRule : short;
enum class SWT_Target : short;
struct SWT_RulesCollection;

class SWT_Abstraction;
template <typename T> using stdfunc = std::function<T>;
typedef stdfunc<void(SWT_Abstraction*, int)> SetAbsFunc;

struct CORE_EXPORT UpdateFuncs {
    stdfunc<void(const SWT_BoxRule)> fContentUpdateIfIsCurrentRule;
    stdfunc<void(SingleWidgetTarget*, const SWT_Target)>
        fContentUpdateIfIsCurrentTarget;
    stdfunc<void()> fContentUpdateIfSearchNotEmpty;
    stdfunc<void()> fUpdateParentHeight;
    stdfunc<void()> fUpdateVisibleWidgetsContent;
};

class CORE_EXPORT SWT_Abstraction : public StdSelfRef {
public:
    SWT_Abstraction(SingleWidgetTarget * const target,
                    const UpdateFuncs& updateFuncs,
                    const int visiblePartId);

    bool setAbstractions(const int minY, const int maxY,
                         int &currY, int currX,
                         const int swtHeight,
                         const SetAbsFunc& setAbsFunc,
                         const SWT_RulesCollection &rules,
                         const bool parentSatisfiesRule,
                         const bool parentMainTarget);

    int updateHeight(const SWT_RulesCollection &rules,
                     const bool parentSatisfiesRule,
                     const bool parentMainTarget,
                     const int swtHeight);
    int getHeight() const { return mHeight; }

    void setContentVisible(const bool visible);

    SingleWidgetTarget *getTarget() const;

    void addChild(SingleWidgetTarget * const target);
    void addChildAt(SingleWidgetTarget * const target, const int id);
    void addChildAbstraction(const stdsptr<SWT_Abstraction> &abs);
    void addChildAbstractionAt(const stdsptr<SWT_Abstraction> &abs, const int id);

    void removeChild(const SingleWidgetTarget * const target);
    void removeChild(const stdsptr<SWT_Abstraction> &abs);

    void switchContentVisible();

    bool contentVisible();

    int getParentVisiblePartWidgetId()
    { return mVisiblePartWidgetId; }

    void scheduleContentUpdate(const SWT_BoxRule rule);
    void scheduleSearchContentUpdate();

    bool isMainTarget() { return mIsMainTarget; }

    void setIsMainTarget(const bool bT) {
        mIsMainTarget = bT;
    }

    SWT_Abstraction *getChildAbsFor(const SingleWidgetTarget * const target);

    void moveChildTo(SingleWidgetTarget * const target, const int id);
    void afterContentVisibilityChanged();

    SWT_Abstraction* getParent() const { return mParent; }

    int getIdInParent() const { return mIdInParent; }

    int childrenCount() const { return mChildren.count(); }

    SWT_Abstraction* getChild(int index) const;

    void removeAlongWithAllChildren_k();
protected:
    void setParent(SWT_Abstraction * const parent);
    void setIdInParent(const int id);
private:
    void updateChildrenIds(const int minId, const int maxId) const;

    bool mIsMainTarget = false;
    bool mContentVisible = false;
    int mHeight = 0;
    const int mVisiblePartWidgetId;
    const UpdateFuncs mUpdateFuncs;
    SingleWidgetTarget * const mTarget_k;

    QList<stdsptr<SWT_Abstraction>> mChildren;
    int mIdInParent = -1;
    stdptr<SWT_Abstraction> mParent;
};

#endif // SWT_ABSTRACTION_H
