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

#include "overlappingkeys.h"

#include "animator.h"
#include "complexkey.h"
#include "complexanimator.h"

OverlappingKeys::OverlappingKeys(const stdsptr<Key> &key,
                                 Animator * const animator) :
    mAnimator(animator) {
    mKeys << key;
}

int OverlappingKeys::getFrame() const {
    const auto key = getKey();
    if(key) return key->getRelFrame();
    return 0;
}

bool OverlappingKeys::hasKey(const Key * const key) const {
    for(const auto& iKey : mKeys) {
        if(iKey.get() == key) return true;
    }
    return false;
}

bool OverlappingKeys::hasMultiple() const {
    return mKeys.count() > 1;
}

Key *OverlappingKeys::getKey() const {
    if(mKeys.isEmpty()) return nullptr;
    if(mKeys.count() == 1) return mKeys.last().get();
    for(const auto& iKey : mKeys) {
        if(iKey->isDescendantSelected()) return iKey.get();
    }
    return mKeys.last().get();
}

bool OverlappingKeys::isEmpty() const {
    return mKeys.isEmpty();
}

void OverlappingKeys::removeKey(const stdsptr<Key> &key) {
    for(int i = 0; i < mKeys.count(); i++) {
        const auto& iKey = mKeys.at(i);
        if(iKey.get() == key.get()) {
            mKeys.removeAt(i);
            break;
        }
    }
}

void OverlappingKeys::addKey(const stdsptr<Key> &key) {
    mKeys.append(key);
}

void OverlappingKeys::merge() {
    if(mKeys.count() < 2) return;
    Key * target = nullptr;
    for(const auto& iKey : qAsConst(mKeys)) {
        if(iKey->isDescendantSelected()) {
            target = iKey.get();
            break;
        }
    }
    if(!target) target = mKeys.last().get();

    if(enve_cast<ComplexAnimator*>(mAnimator)) {
        const auto cTarget = static_cast<ComplexKey*>(target);
        for(int i = 0; i < mKeys.count(); i++) {
            const auto& iKey = mKeys.at(i);
            if(iKey.get() == target) continue;
            const auto cKey = static_cast<ComplexKey*>(iKey.get());
            cKey->moveAllKeysTo(cTarget);
            mAnimator->anim_removeKeyAction(iKey);
            i--;
        }
    } else {
        for(int i = 0; i < mKeys.count(); i++) {
            const auto& iKey = mKeys.at(i);
            if(iKey.get() == target) continue;
            mAnimator->anim_removeKeyAction(iKey);
            i--;
        }
    }
}
