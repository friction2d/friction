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

#include "undoredo.h"
#include "exceptions.h"

class UndoRedo_priv {
public:
    UndoRedo_priv(const int frame,
                  const QString& name,
                  const std::function<void()>& undo,
                  const std::function<void()>& redo) :
        fFrame(frame), fName(name), fUndo(undo), fRedo(redo) {}

    const int fFrame;
    const QString fName;
    const std::function<void()> fUndo;
    const std::function<void()> fRedo;
};

class UndoRedoSet : public UndoRedo_priv {
public:
    UndoRedoSet(const int frame, const QString& name) :
        UndoRedo_priv(frame, name,
                      [this]() { undo(); },
                      [this]() { redo(); }) {}

    void addUndoRedo(const stdsptr<UndoRedo_priv>& undoRedo)
    { mSet << undoRedo; }
    bool isEmpty()
    { return mSet.isEmpty(); }
private:
    void undo();
    void redo();

    QList<stdsptr<UndoRedo_priv>> mSet;
};

void UndoRedoSet::undo() {
    for(int i = mSet.length() - 1; i >= 0; i--)
        mSet.at(i)->fUndo();
}

void UndoRedoSet::redo() {
    for(const auto& undoRedo : qAsConst(mSet))
        undoRedo->fRedo();
}

UndoRedoStack::UndoRedoStack(const std::function<bool(int)> &changeFrameFunc) :
    mChangeFrameFunc(changeFrameFunc) {}

void UndoRedoStack::pushName(const QString &name) {
    if(mCurrentSetName.isEmpty()) {
        mCurrentSetName = name;
    }
}

bool UndoRedoStack::newCollection() {
    const bool add = mCurrentSet && !mCurrentSet->isEmpty();
    if(add) {
        mRedoStack.clear();
        emptySomeOfUndo();
        mUndoStack << mCurrentSet;
        checkUndoRedoChanged();
    }
    mCurrentSet = nullptr;
    mCurrentSetName = "";
    return add;
}

void UndoRedoStack::addToSet(const stdsptr<UndoRedo_priv>& undoRedo) {
    if(!mCurrentSet) {
        pushName(undoRedo->fName);
        mCurrentSet = std::make_shared<UndoRedoSet>(mCurrentAbsFrame,
                                                    mCurrentSetName);
    }
    mCurrentSet->addUndoRedo(undoRedo);
}

void UndoRedoStack::emptySomeOfUndo() {
    if(mUndoStack.length() > 50) {
        //            for(int i = 0; i < 50; i++) {
        mUndoStack.removeFirst();
        //            }
    }
}

void UndoRedoStack::addUndoRedo(const QString& name,
                                const std::function<void()>& undoFunc,
                                const std::function<void()>& redoFunc) {
    if(mUndoRedoBlocked) return;
    if(!undoFunc) RuntimeThrow("Missing undo function.");
    if(!redoFunc) RuntimeThrow("Missing redo function.");
    const auto undoRedo = std::make_shared<UndoRedo_priv>(mCurrentAbsFrame, name,
                                                          undoFunc, redoFunc);
    addToSet(undoRedo);
}

QString UndoRedoStack::undoText() const {
    if(mUndoStack.isEmpty()) return "Undo";
    return "Undo " + mUndoStack.last()->fName;
}

QString UndoRedoStack::redoText() const {
    if(mRedoStack.isEmpty()) return "Redo";
    return "Redo " + mRedoStack.last()->fName;
}

bool UndoRedoStack::canUndo() const {
    return !mUndoStack.isEmpty();
}

bool UndoRedoStack::canRedo() const {
    return !mRedoStack.isEmpty();
}

bool UndoRedoStack::redo() {
    if(!canRedo()) return false;
    stdsptr<UndoRedo_priv> toRedo = mRedoStack.last();
    // TODO: add setting for frame skip?
    if(mChangeFrameFunc(toRedo->fFrame)) return true;
    mRedoStack.removeLast();
    {
        const auto block = blockUndoRedo();
        toRedo->fRedo();
    }
    mUndoStack << toRedo;
    checkUndoRedoChanged();
    return true;
}

bool UndoRedoStack::undo() {
    if(!canUndo()) return false;
    stdsptr<UndoRedo_priv> toUndo = mUndoStack.last();
    // TODO: add setting for frame skip?
    if(mChangeFrameFunc(toUndo->fFrame)) return true;
    mUndoStack.removeLast();
    {
        const auto block = blockUndoRedo();
        toUndo->fUndo();
    }
    mRedoStack << toUndo;
    checkUndoRedoChanged();
    return true;
}

UndoRedoStack::StackBlock UndoRedoStack::blockUndoRedo() {
    return StackBlock(this);
}

void UndoRedoStack::clear() {
    mUndoStack.clear();
    mRedoStack.clear();
    checkUndoRedoChanged();
}

void UndoRedoStack::checkUndoRedoChanged() {
    checkCanUndoRedoChanged();
    checkUndoRedoTextChanged();
}

void UndoRedoStack::checkUndoRedoTextChanged() {
    checkUndoTextChanged();
    checkRedoTextChanged();
}

void UndoRedoStack::checkUndoTextChanged() {
    if(mUndoText == undoText()) return;
    mUndoText = undoText();
    emit undoTextChanged(mUndoText);
}

void UndoRedoStack::checkRedoTextChanged() {
    if(mRedoText == redoText()) return;
    mRedoText = redoText();
    emit redoTextChanged(mRedoText);
}

void UndoRedoStack::checkCanUndoRedoChanged() {
    checkCanUndoChanged();
    checkCanRedoChanged();
}

void UndoRedoStack::checkCanUndoChanged() {
    if(mCanUndo == canUndo()) return;
    mCanUndo = canUndo();
    emit canUndoChanged(mCanUndo);
}

void UndoRedoStack::checkCanRedoChanged() {
    if(mCanRedo == canRedo()) return;
    mCanRedo = canRedo();
    emit canRedoChanged(mCanRedo);
}
