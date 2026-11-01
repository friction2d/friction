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

#ifndef KEY_H
#define KEY_H
#include "../smartPointers/ememory.h"
#include "../Properties/property.h"

#include <QtCore>
class QPainter;
class KeyCloner;
class Animator;
struct UndoRedo;
class KeysClipboard;

class CORE_EXPORT Key : public StdSelfRef {
    friend class Animator;
    friend class KeysClipboard;
    friend class ComplexKey;
public:
    Key(Animator * const parentAnimator);
    Key(const int frame, Animator * const parentAnimator);

//    QrealPoint *mousePress(qreal frameT, qreal valueT,
//                    qreal pixelsPerFrame, qreal pixelsPerValue);

    virtual void startFrameTransform();
    virtual void cancelFrameTransform();
    virtual void finishFrameTransform();

    virtual void startValueTransform() {}
    virtual void cancelValueTransform() {}
    virtual void finishValueTransform() {}

    virtual bool differsFromKey(Key* key) const = 0;
    virtual void writeKey(eWriteStream &dst);
    virtual void readKey(eReadStream &src);

    virtual void scaleFrameAndUpdateParentAnimator(
            const int relativeToFrame,
            const qreal scaleFactor,
            const bool useSavedFrame);

    virtual bool isSelected() const;
    virtual void addToSelection(QList<Animator *> &toSelect);
    virtual void removeFromSelection(QList<Animator *> &toRemove);
    virtual bool isDescendantSelected() const { return isSelected(); }

    void invalidateSceneFramesCache();

    int getAbsFrame() const;

    bool hasPrevKey() const;
    bool hasNextKey() const;
    void incFrameAndUpdateParentAnimator(const int inc);
    void setRelFrameAndUpdateParentAnimator(const int relFrame);
    template <class T = Animator>
    T* getParentAnimator() const {
        return static_cast<T*>(mParentAnimator.data());
    }

    void setSelected(const bool bT);

    bool isHovered() const {
        return mHovered;
    }

    void setHovered(const bool bT) {
        mHovered = bT;
    }

    int getRelFrame() const;

    Key* getNextKey() const;
    Key* getPrevKey() const;

    template <class T>
    T* getNextKey() const {
        return static_cast<T*>(getNextKey());
    }

    template <class T = Key>
    T* getPrevKey() const {
        return static_cast<T*>(getPrevKey());
    }

    bool differesFromNextKey() const {
        return differsFromKey(getNextKey());
    }

    bool differesFromPrevKey() const {
        return differsFromKey(getPrevKey());
    }

    void afterKeyChanged();

    int getPrevKeyRelFrame() const {
        auto prevKey = getPrevKey();
        if(!prevKey) return mRelFrame;
        return prevKey->getRelFrame();
    }

    int getNextKeyRelFrame() const {
        auto nextKey = getNextKey();
        if(!nextKey) return mRelFrame;
        return getNextKey()->getRelFrame();
    }

    int relFrameToAbsFrame(const int relFrame) const;
    int absFrameToRelFrame(const int absFrame) const;
    qreal relFrameToAbsFrameF(const qreal relFrame) const;
    qreal absFrameToRelFrameF(const qreal absFrame) const;

    void moveToRelFrame(const int frame);
    void moveToRelFrameAction(const int frame);
protected:
    virtual void setRelFrame(const int frame);
    void addUndoRedo(const UndoRedo& undoRedo);
    void setAbsFrame(const int frame);

    int mRelFrame;
    int mSavedRelFrame;

    const QPointer<Animator> mParentAnimator;
private:
    bool mIsSelected = false;
    bool mHovered = false;
};

#endif // KEY_H
