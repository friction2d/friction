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

#include "keyfocustarget.h"
KeyFocusTarget *KeyFocusTarget::KFT_mCurrentTarget = nullptr;
KeyFocusTarget *KeyFocusTarget::KFT_mLastTarget = nullptr;
QList<KeyFocusTarget*> KeyFocusTarget::KFT_mAllTargets;

KeyFocusTarget::KeyFocusTarget() {
    KFT_mAllTargets << this;
}

KeyFocusTarget::~KeyFocusTarget() {
    KFT_mAllTargets.removeOne(this);
    if(KFT_mLastTarget == this)
        KFT_mLastTarget = nullptr;
    if(KFT_mCurrentTarget == this) {
        KFT_mCurrentTarget = nullptr;
        KFT_sSetRandomTarget();
    }
}

KeyFocusTarget * KeyFocusTarget::KFT_getCurrentTarget() {
    return KeyFocusTarget::KFT_mCurrentTarget;
}

bool KeyFocusTarget::KFT_handleKeyEvent(QKeyEvent *e) {
    const auto func = e->type() == QEvent::KeyRelease ?
                &KeyFocusTarget::KFT_keyReleaseEvent :
                &KeyFocusTarget::KFT_keyPressEvent;

    if(KFT_mCurrentTarget) {
        if((KFT_mCurrentTarget->*func)(e)) {
            KFT_mCurrentTarget->KFT_setFocus();
            return true;
        }
    }
    for(const auto target : qAsConst(KFT_mAllTargets)) {
        if(target == KFT_mCurrentTarget) continue;
        if((target->*func)(e)) {
            target->KFT_setFocus();
            return true;
        }
    }

    return false;
}

void KeyFocusTarget::KFT_sTab() {
    const auto lastTarget = KFT_mCurrentTarget ? KFT_mCurrentTarget :
                                                 KFT_mLastTarget;
    int index = lastTarget ? KFT_mAllTargets.indexOf(lastTarget) + 1 : 0;
    for(int i = 0; i < KFT_mAllTargets.count(); i++) {
        if(index >= KFT_mAllTargets.count()) index = 0;
        const auto target = KFT_mAllTargets.at(index++);
        if(target && target->visible()) {
            target->KFT_setFocus();
            break;
        }
    }
}

void KeyFocusTarget::KFT_sSetRandomTarget() {
    if(KFT_mCurrentTarget) return KFT_mCurrentTarget->KFT_setFocus();
    if(KFT_mLastTarget && KFT_mLastTarget->visible()) KFT_sSetLastTarget();
    else KFT_sTab();
}

void KeyFocusTarget::KFT_sSetLastTarget() {
    if(KFT_mLastTarget) KFT_mLastTarget->KFT_setFocus();
}

void KeyFocusTarget::KFT_setCurrentTarget(KeyFocusTarget *target) {
    if(target == KFT_mCurrentTarget) return;
    if(KFT_mCurrentTarget) KFT_mCurrentTarget->KFT_clearFocus();
    KFT_mCurrentTarget = target;
    if(target) KFT_mLastTarget = target;
}
