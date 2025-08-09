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

#ifndef ANIMATOR_H
#define ANIMATOR_H

class QIODevice;

#include "../Properties/property.h"
#include "key.h"
#include "overlappingkeylist.h"

#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"

#include <QDomElement>

class QPainter;
class TimelineMovable;
class SvgExporter;

class CORE_EXPORT Animator : public Property {
    Q_OBJECT
    e_DECLARE_TYPE(Animator)
    friend class OverlappingKeys;
protected:
    Animator(const QString &name);

    virtual void anim_afterKeyOnCurrentFrameChanged(Key* const key)
    { Q_UNUSED(key) }
public:
    virtual stdsptr<Key> anim_createKey() = 0;
    virtual void anim_addKeyAtRelFrame(const int relFrame) = 0;

    virtual void anim_scaleTime(const int pivotAbsFrame,
                                const qreal scale);

    virtual bool anim_prevRelFrameWithKey(const int relFrame,
                                          int &prevRelFrame);
    virtual bool anim_nextRelFrameWithKey(const int relFrame,
                                          int &nextRelFrame);
    virtual Key *anim_getKeyAtPos(const qreal relX,
                                  const int minViewedFrame,
                                  const qreal pixelsPerFrame,
                                  const int keyRectSize);
    virtual void anim_setAbsFrame(const int frame);
    virtual bool anim_isDescendantRecording() const;

    virtual TimelineMovable *anim_getTimelineMovable(
                const int relX, const int minViewedFrame,
                const qreal pixelsPerFrame) {
        Q_UNUSED(relX)
        Q_UNUSED(minViewedFrame)
        Q_UNUSED(pixelsPerFrame)
        return nullptr;
    }

    virtual void anim_removeAllKeys();
    virtual void anim_shiftAllKeys(const int shift);
    virtual void anim_getKeysInRect(const QRectF &selectionRect,
                                    const qreal pixelsPerFrame,
                                    QList<Key*>& keysList,
                                    const int keyRectSize);
    virtual void anim_setRecording(const bool rec);

    void prp_drawTimelineControls(
            QPainter * const p, const qreal pixelsPerFrame,
            const FrameRange &absFrameRange, const int rowHeight);

    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    void prp_afterFrameShiftChanged(const FrameRange& oldAbsRange,
                                    const FrameRange& newAbsRange);
    void prp_afterChangedAbsRange(const FrameRange &range,
                                  const bool clip = true);
    FrameRange prp_getIdenticalRelRange(const int relFrame) const;
    FrameRange prp_nextNonUnaryIdenticalRelRange(const int relFrame) const;
public:
    void anim_saveCurrentValueAsKey();
    void anim_appendKey(const stdsptr<Key> &newKey);
    void anim_removeKey(const stdsptr<Key>& keyToRemove);
    void anim_removeKeys(const FrameRange& relRange, const bool action);
    void anim_removeAllKeysFromComplexAnimator(ComplexAnimator *target);
    void anim_mergeKeysIfNeeded();
    void anim_updateAfterChangedKey(Key * const key);

    bool anim_hasKeys() const;
    bool anim_isRecording();

    void anim_updateRelFrame();
    void anim_switchRecording();

    bool anim_getClosestsKeyOccupiedRelFrame(const int frame,
                                             int &closest);
    template <class T = Key>
    T* anim_getKeyOnCurrentFrame() const;
    template <class T = Key>
    T *anim_getKeyAtRelFrame(const int frame) const;
    template <class T = Key>
    T *anim_getKeyAtAbsFrame(const int frame) const;
    template <class T = Key>
    T *anim_getNextKey(const Key * const key) const;
    template <class T = Key>
    T *anim_getPrevKey(const Key * const key) const;
    template <class T = Key>
    std::pair<T*, T*> anim_getPrevAndNextKey(const int relFrame) const;
    template <class T = Key>
    T* anim_getPrevKey(const int relFrame) const;
    template <class T = Key>
    T* anim_getNextKey(const int relFrame) const;

    int anim_getPrevKeyId(const int relFrame) const;
    int anim_getNextKeyId(const int relFrame) const;

    int anim_getNextKeyRelFrame(const Key * const key) const;
    int anim_getPrevKeyRelFrame(const Key * const key) const;

    bool anim_hasPrevKey(const Key * const key);
    bool anim_hasNextKey(const Key * const key);

    void anim_addAllKeysToComplexAnimator(ComplexAnimator *target);

    void anim_setRecordingWithoutChangingKeys(const bool rec);

    std::pair<int, int> anim_getPrevAndNextKeyId(const int relFrame) const;
    std::pair<int, int> anim_getPrevAndNextKeyIdF(const qreal relFrame) const;

    void anim_setRecordingValue(const bool rec);

    int anim_getCurrentRelFrame() const;
    int anim_getCurrentAbsFrame() const;

    void anim_moveKeyToRelFrame(Key * const key, const int newFrame);

    int anim_getPrevKeyRelFrame(const int relFrame) const;
    int anim_getNextKeyRelFrame(const int relFrame) const;

    bool anim_hasSelectedKeys() const;

    void anim_addKeyToSelected(Key * const key);
    void anim_removeKeyFromSelected(Key * const key);

    void anim_writeSelectedKeys(eWriteStream &dst);

    void anim_deselectAllKeys();
    void anim_selectAllKeys();

    void anim_incSelectedKeysFrame(const int dFrame);

    void anim_scaleSelectedKeysFrame(const int absPivotFrame,
                                     const qreal scale);

    void anim_cancelSelectedKeysTransform();
    void anim_finishSelectedKeysTransform();
    void anim_startSelectedKeysTransform();

    void anim_deleteSelectedKeys();
    void anim_deleteCurrentKeyAction();

    int anim_getLowestAbsFrameForSelectedKey();
    int anim_getHighestAbsFrameForSelectedKey();

    const QList<Key*>& anim_getSelectedKeys() const {
        return anim_mSelectedKeys;
    }

    const OverlappingKeyList& anim_getKeys() const {
        return anim_mKeys;
    }

    template <class T = Key>
    T* anim_getKeyAtIndex(const int id) const;
    int anim_getKeyIndex(const Key * const key) const;

    void anim_coordinateKeysWith(Animator * const other);
    void anim_addKeysWhereOtherHasKeys(const Animator * const other);

    void anim_appendKeyAction(const stdsptr<Key> &newKey);
    void anim_removeKeyAction(const stdsptr<Key> newKey);

    using ValueGetter = std::function<QString(const int relFrame)>;
    void saveSVG(SvgExporter& exp,
                 QDomElement& parent,
                 const FrameRange& visRange,
                 const QString& attrName,
                 const ValueGetter& valueGetter,
                 const QString& interpolation = "linear",
                 QList<Animator*> const extInfl = QList<Animator*>()) const;
    void saveSVG(SvgExporter& exp,
                 QDomElement& parent,
                 const FrameRange& visRange,
                 const QString& attrName,
                 const ValueGetter& valueGetter,
                 const bool transform,
                 const QString& type,
                 const QString& interpolation = "linear",
                 QList<Animator*> const extInfl = QList<Animator*>()) const;
protected:
    void anim_readKeys(eReadStream &src);
    void anim_writeKeys(eWriteStream& dst) const;

    IdRange anim_frameRangeToKeyIdRange(const FrameRange& relRange) const;
signals:
    void anim_isRecordingChanged();
    void anim_changedKeyOnCurrentFrame(Key* key, QPrivateSignal);
    void anim_removedKey(Key* key, QPrivateSignal);
    void anim_addedKey(Key* key, QPrivateSignal);
private:
    void removeKeyWithoutDeselecting(const stdsptr<Key> &keyToRemove);
    void anim_updateKeyOnCurrrentFrame();
    void anim_setKeyOnCurrentFrame(Key * const key);

    bool anim_mIsRecording = false;

    int anim_mCurrentAbsFrame = 0;
    int anim_mCurrentRelFrame = 0;
    stdptr<Key> anim_mKeyOnCurrentFrame;
    QList<Key*> anim_mSelectedKeys;
    OverlappingKeyList anim_mKeys;
};

template <class T>
T *Animator::anim_getKeyOnCurrentFrame() const {
    return static_cast<T*>(anim_mKeyOnCurrentFrame.data());
}

template <class T>
T* Animator::anim_getKeyAtIndex(const int id) const {
    if(id < 0 || id >= anim_mKeys.count()) return nullptr;
    return anim_mKeys.atId<T>(id);
}

template <class T>
T *Animator::anim_getKeyAtRelFrame(const int frame) const {
    return anim_mKeys.atRelFrame<T>(frame);
}

template <class T>
T *Animator::anim_getKeyAtAbsFrame(const int frame) const {
    return anim_getKeyAtRelFrame<T>(prp_absFrameToRelFrame(frame));
}

template <class T>
T* Animator::anim_getPrevKey(const Key * const key) const {
    const int keyId = anim_getKeyIndex(key);
    if(keyId <= 0) return nullptr;
    return anim_mKeys.atId<T>(keyId - 1);
}

template <class T>
T* Animator::anim_getNextKey(const Key * const key) const {
    const int keyId = anim_getKeyIndex(key);
    if(keyId == anim_mKeys.count() - 1) return nullptr;
    return anim_mKeys.atId<T>(keyId + 1);
}

template <class T>
std::pair<T*, T*> Animator::anim_getPrevAndNextKey(const int relFrame) const {
    const auto prevNext = anim_getPrevAndNextKeyId(relFrame);
    const int prevId = prevNext.first;
    T * const prevKey = anim_getKeyAtIndex<T>(prevId);
    const int nextId = prevNext.second;
    T * const nextKey = anim_getKeyAtIndex<T>(nextId);
    return {prevKey, nextKey};
}

template <class T>
T *Animator::anim_getPrevKey(const int relFrame) const {
    return anim_getKeyAtIndex<T>(anim_getPrevKeyId(relFrame));
}

template <class T>
T *Animator::anim_getNextKey(const int relFrame) const {
    return anim_getKeyAtIndex<T>(anim_getNextKeyId(relFrame));
}


#endif // ANIMATOR_H
