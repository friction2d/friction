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

#include "complexkey.h"
#include "complexanimator.h"

ComplexKey::ComplexKey(const int absFrame,
                       ComplexAnimator * const parent) :
    Key(parent) {
    setAbsFrame(absFrame);
}

void ComplexKey::addChildKey(Key * const key) {
    mKeys << key;
}

void ComplexKey::removeChildKey(Key * const key) {
    mKeys.removeOne(key);
}

bool ComplexKey::isEmpty() const {
    return mKeys.isEmpty();
}

void ComplexKey::moveAllKeysTo(ComplexKey * const target) {
    target->mKeys << mKeys;
    mKeys.clear();
}

bool ComplexKey::isDescendantSelected() const {
    if(isSelected()) return true;
    for(const auto& key : mKeys) {
        if(key->isDescendantSelected()) return true;
    }
    return false;
}

bool ComplexKey::isSelected() const {
    for(const auto& key : mKeys) {
        if(key->isSelected()) continue;
        return false;
    }

    return true;
}

void ComplexKey::addToSelection(QList<Animator*> &selectedAnimators) {
    for(const auto& key : qAsConst(mKeys)) key->addToSelection(selectedAnimators);
}

bool ComplexKey::hasKey(Key *key) const {
    for(const auto& keyT : mKeys) {
        if(key == keyT) return true;
    }
    return false;
}

bool ComplexKey::differsFromKey(Key *otherKey) const {
    const auto otherComplexKey = static_cast<ComplexKey*>(otherKey);
    if(getChildKeysCount() == otherComplexKey->getChildKeysCount()) {
        for(const auto& key : mKeys) {
            if(otherComplexKey->hasSameKey(key)) continue;
            return true;
        }
        return false;
    }
    return true;
}

void ComplexKey::removeFromSelection(QList<Animator *> &selectedAnimators) {
    for(const auto& key : qAsConst(mKeys))
        key->removeFromSelection(selectedAnimators);
}

int ComplexKey::getChildKeysCount() const {
    return mKeys.count();
}

bool ComplexKey::hasSameKey(Key *otherKey) const {
    for(const auto& key : mKeys) {
        if(key->getParentAnimator() == otherKey->getParentAnimator()) {
            if(key->differsFromKey(otherKey)) return false;
            return true;
        }
    }
    return false;
}
