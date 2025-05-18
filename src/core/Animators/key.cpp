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

#include "key.h"
#include "qrealpoint.h"
#include "animator.h"
#include "pointhelpers.h"
#include "Private/scene.h"

Key::Key(Animator * const parentAnimator) :
    Key(0, parentAnimator) {}

Key::Key(const int frame, Animator * const parentAnimator) :
    mParentAnimator(parentAnimator) {
    mRelFrame = frame;
}

bool Key::isSelected() const { return mIsSelected; }

void Key::writeKey(eWriteStream& dst) {
    dst << mRelFrame;
}

void Key::readKey(eReadStream& src) {
    src >> mRelFrame;
}

bool Key::hasPrevKey() const {
    if(!mParentAnimator) return false;
    return mParentAnimator->anim_hasPrevKey(this);
}

bool Key::hasNextKey() const {
    if(!mParentAnimator) return false;
    return mParentAnimator->anim_hasNextKey(this);
}

void Key::incFrameAndUpdateParentAnimator(const int inc) {
    setRelFrameAndUpdateParentAnimator(mRelFrame + inc);
}

void Key::setRelFrameAndUpdateParentAnimator(const int relFrame) {
    if(!mParentAnimator) return;
    mParentAnimator->anim_moveKeyToRelFrame(this, relFrame);
}

void Key::addToSelection(QList<Animator*> &toSelect) {
    if(isSelected()) return;
    if(!mParentAnimator->anim_hasSelectedKeys()) {
        toSelect << mParentAnimator;
    }
    mParentAnimator->anim_addKeyToSelected(this);
}

void Key::removeFromSelection(QList<Animator*> &toRemove) {
    if(isSelected()) {
        mParentAnimator->anim_removeKeyFromSelected(this);
        if(!mParentAnimator->anim_hasSelectedKeys()) {
            toRemove << mParentAnimator;
        }
    }
}

void Key::startFrameTransform() {
    mSavedRelFrame = mRelFrame;
}

void Key::cancelFrameTransform() {
    mParentAnimator->anim_moveKeyToRelFrame(this, mSavedRelFrame);
}

void Key::scaleFrameAndUpdateParentAnimator(
        const int relativeToFrame,
        const qreal scaleFactor,
        const bool useSavedFrame) {
    int thisRelFrame = useSavedFrame ? mSavedRelFrame : mRelFrame;
    int relativeToRelFrame =
            mParentAnimator->prp_absFrameToRelFrame(relativeToFrame);
    int newFrame = qRound(relativeToRelFrame +
                          (thisRelFrame - relativeToRelFrame)*scaleFactor);
    if(newFrame == mRelFrame) return;
    setRelFrameAndUpdateParentAnimator(newFrame);
}

void Key::setSelected(const bool bT) {
    mIsSelected = bT;
}

void Key::finishFrameTransform() {
    UndoRedo ur;
    const int oldFrame = mSavedRelFrame;
    const int newFrame = mRelFrame;
    ur.fUndo = [this, oldFrame]() {
        moveToRelFrame(oldFrame);
    };
    ur.fRedo = [this, newFrame]() {
        moveToRelFrame(newFrame);
    };
    addUndoRedo(ur);
}

int Key::relFrameToAbsFrame(const int relFrame) const {
    if(!mParentAnimator) return relFrame;
    return mParentAnimator->prp_relFrameToAbsFrame(relFrame);
}

int Key::absFrameToRelFrame(const int absFrame) const {
    if(!mParentAnimator) return absFrame;
    return mParentAnimator->prp_absFrameToRelFrame(absFrame);
}

qreal Key::relFrameToAbsFrameF(const qreal relFrame) const {
    if(!mParentAnimator) return relFrame;
    return mParentAnimator->prp_relFrameToAbsFrameF(relFrame);
}

qreal Key::absFrameToRelFrameF(const qreal absFrame) const {
    if(!mParentAnimator) return absFrame;
    return mParentAnimator->prp_absFrameToRelFrameF(absFrame);
}

void Key::moveToRelFrame(const int frame) {
    if(!mParentAnimator) setRelFrame(frame);
    else mParentAnimator->anim_moveKeyToRelFrame(this, frame);
}

void Key::moveToRelFrameAction(const int frame) {
    startFrameTransform();
    moveToRelFrame(frame);
    finishFrameTransform();
}

void Key::addUndoRedo(const UndoRedo &undoRedo) {
    if(!mParentAnimator) return;
    const auto parentScene = mParentAnimator->getParentScene();
    if(!parentScene) return;
    stdptr<Key> thisPtr = this;
    auto undo = undoRedo.fUndo;
    auto redo = undoRedo.fRedo;
    undo = [thisPtr, undo]() { if(thisPtr) undo(); };
    redo = [thisPtr, redo]() { if(thisPtr) redo(); };
    parentScene->addUndoRedo("KeyFrame Change", undo, redo);
}

int Key::getAbsFrame() const {
    return relFrameToAbsFrame(mRelFrame);
}

int Key::getRelFrame() const {
    return mRelFrame;
}

void Key::setRelFrame(const int frame) {
    mRelFrame = frame;
}

void Key::setAbsFrame(const int frame) {
    setRelFrame(absFrameToRelFrame(frame));
}

Key *Key::getNextKey() const {
    return mParentAnimator->anim_getNextKey(this);
}

Key *Key::getPrevKey() const {
    return mParentAnimator->anim_getPrevKey(this);
}

void Key::afterKeyChanged() {
    mParentAnimator->anim_updateAfterChangedKey(this);
}
