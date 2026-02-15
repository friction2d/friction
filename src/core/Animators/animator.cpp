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

#include "animator.h"

#include "complexanimator.h"
#include "key.h"
#include "qrealpoint.h"
#include "simplemath.h"
#include "svgexporthelpers.h"
#include "Private/esettings.h"

#include <QPainter>

Animator::Animator(const QString& name) : Property(name), anim_mKeys(this) {}

void Animator::anim_scaleTime(const int pivotAbsFrame, const qreal scale) {
    for(const auto &key : anim_mKeys) {
        key->scaleFrameAndUpdateParentAnimator(pivotAbsFrame, scale, false);
    }
}

void Animator::anim_shiftAllKeys(const int shift) {
    QList<Key*> keys;
    for(const auto& key : anim_mKeys) {
        keys << key;
    }
    for(const auto key : qAsConst(keys)) {
        const int targetFrame = key->getRelFrame() + shift;
        key->moveToRelFrameAction(targetFrame);
    }
}

bool Animator::anim_nextRelFrameWithKey(const int relFrame, int &nextRelFrame) {
    const auto key = anim_getNextKey(relFrame);
    if(!key) return false;
    nextRelFrame = key->getRelFrame();
    return true;
}

bool Animator::anim_prevRelFrameWithKey(const int relFrame, int &prevRelFrame) {
    const auto key = anim_getPrevKey(relFrame);
    if(!key) return false;
    prevRelFrame = key->getRelFrame();
    return true;
}

int Animator::anim_getPrevKeyRelFrame(const Key * const key) const {
    if(!key) return FrameRange::EMIN;
    Key * const prevKey = key->getPrevKey();
    if(!prevKey) return FrameRange::EMIN;
    return prevKey->getRelFrame();
}

int Animator::anim_getNextKeyRelFrame(const Key * const key) const {
    if(!key) return FrameRange::EMAX;
    Key * const nextKey = key->getNextKey();
    if(!nextKey) return FrameRange::EMAX;
    return nextKey->getRelFrame();
}

int Animator::anim_getPrevKeyRelFrame(const int relFrame) const {
    Key * const prevKey = anim_getPrevKey(relFrame);
    if(!prevKey) return FrameRange::EMIN;
    return prevKey->getRelFrame();
}

int Animator::anim_getNextKeyRelFrame(const int relFrame) const {
    Key * const nextKey = anim_getNextKey(relFrame);
    if(!nextKey) return FrameRange::EMAX;
    return nextKey->getRelFrame();
}

void Animator::prp_afterChangedAbsRange(const FrameRange &range, const bool clip) {
    if(range.inRange(anim_mCurrentAbsFrame))
        prp_afterChangedCurrent(UpdateReason::userChange);
    emit prp_absFrameRangeChanged(range, clip);
}

void Animator::anim_updateAfterChangedKey(Key * const key) {
    if(toComplexAnimator() || !key) return;
    const int relFrame = key->getRelFrame();
    int prevKeyRelFrame = anim_getPrevKeyRelFrame(key);
    if(prevKeyRelFrame != FrameRange::EMIN) prevKeyRelFrame++;
    int nextKeyRelFrame = anim_getNextKeyRelFrame(key);
    if(nextKeyRelFrame != FrameRange::EMAX) nextKeyRelFrame--;
    const FrameRange inflRange = {prevKeyRelFrame, nextKeyRelFrame};
    prp_afterChangedRelRange(inflRange + prp_getIdenticalRelRange(relFrame));
}

void Animator::anim_setAbsFrame(const int frame) {
    anim_mCurrentAbsFrame = frame;
    anim_updateRelFrame();
}

void Animator::anim_updateRelFrame() {
    anim_mCurrentRelFrame = anim_mCurrentAbsFrame - prp_getTotalFrameShift();
    anim_updateKeyOnCurrrentFrame();
}

void Animator::anim_setRecording(const bool rec) {
    if(rec == anim_mIsRecording) return;
    if(rec) {
        anim_setRecordingWithoutChangingKeys(rec);
        anim_saveCurrentValueAsKey();
    } else {
        anim_removeAllKeys();
        anim_setRecordingWithoutChangingKeys(rec);
    }
}

void Animator::anim_switchRecording() {
    anim_setRecording(!anim_mIsRecording);
}

bool Animator::anim_isDescendantRecording() const { return anim_mIsRecording; }

struct KeyPair {
    KeyPair(Key * const key1, Key * const key2) :
        fKey1(key1), fKey2(key2) {}

    Key * fKey1;
    Key * fKey2;
};

void Animator::anim_mergeKeysIfNeeded() {
    anim_mKeys.mergeAll();
}

bool Animator::anim_getClosestsKeyOccupiedRelFrame(const int frame,
                                                   int &closest) {
    int nextT;
    const bool hasNext = anim_nextRelFrameWithKey(frame, nextT);
    int prevT;
    const bool hasPrev = anim_prevRelFrameWithKey(frame, prevT);
    if(hasPrev && hasNext) {
        if(nextT - frame > frame - prevT) {
            closest = prevT;
        } else {
            closest = nextT;
        }
    } else if(hasNext) {
        closest = nextT;
    } else if(hasPrev) {
        closest = prevT;
    } else {
        return false;
    }
    return true;
}

bool Animator::anim_hasPrevKey(const Key * const key) {
    const int keyId = anim_getKeyIndex(key);
    if(keyId > 0) return true;
    return false;
}

bool Animator::anim_hasNextKey(const Key * const key) {
    const int keyId = anim_getKeyIndex(key);
    if(keyId < anim_mKeys.count() - 1) return true;
    return false;
}

int Animator::anim_getPrevKeyId(const int relFrame) const {
    return anim_getPrevAndNextKeyId(relFrame).first;
}

int Animator::anim_getNextKeyId(const int relFrame) const {
    return anim_getPrevAndNextKeyId(relFrame).second;
}

int Animator::anim_getKeyIndex(const Key * const key) const {
    int index;
    if(anim_mKeys.hasKey(key, &index)) return index;
    return -1;
}

void Animator::anim_addKeysWhereOtherHasKeys(const Animator * const other) {
    for(const auto& otherKey : other->anim_mKeys) {
        const int absFrame = otherKey->getAbsFrame();
        const int relFrame = prp_absFrameToRelFrame(absFrame);
        if(!anim_getKeyAtRelFrame(relFrame))
            anim_addKeyAtRelFrame(relFrame);
    }
}

void Animator::anim_readKeys(eReadStream& src) {
    int nKeys; src >> nKeys;
    if(nKeys < 0 || nKeys > 10000)
        RuntimeThrow("Invalid key count " + std::to_string(nKeys));
    for(int i = 0; i < nKeys; i++) {
        const auto key = anim_createKey();
        key->readKey(src);
        anim_appendKey(key);
    }
}

void Animator::anim_writeSelectedKeys(eWriteStream &dst) {
    const int nKeys = anim_mSelectedKeys.count();
    dst << nKeys;
    for(const auto& key : qAsConst(anim_mSelectedKeys)) key->writeKey(dst);
}

void Animator::anim_writeKeys(eWriteStream &dst) const {
    const int nKeys = anim_mKeys.count();
    dst << nKeys;
    for(const auto &key : anim_mKeys) key->writeKey(dst);
}

IdRange Animator::anim_frameRangeToKeyIdRange(const FrameRange &relRange) const {
    int min = anim_getPrevKeyId(relRange.fMin + 1);
    int max = anim_getNextKeyId(relRange.fMax - 1);
    if(min == -1) min = 0;
    if(max == -1) max = anim_mKeys.count() - 1;
    return {min, max};
}

void Animator::anim_coordinateKeysWith(Animator * const other) {
    anim_addKeysWhereOtherHasKeys(other);
    other->anim_addKeysWhereOtherHasKeys(this);
}

void Animator::anim_deleteCurrentKeyAction() {
    if(anim_mKeyOnCurrentFrame && !toComplexAnimator())
        anim_removeKeyAction(anim_mKeyOnCurrentFrame->ref<Key>());
}

void Animator::anim_appendKeyAction(const stdsptr<Key>& newKey)
{
    anim_appendKey(newKey);
    {
        prp_pushUndoRedoName(tr("Add Key"));
        UndoRedo ur;
        ur.fUndo = [this, newKey]() { anim_removeKey(newKey); };
        ur.fRedo = [this, newKey]() { anim_appendKey(newKey); };
        prp_addUndoRedo(ur);
    }
}

void Animator::anim_removeKeyAction(const stdsptr<Key> newKey)
{
    anim_removeKey(newKey);
    {
        prp_pushUndoRedoName(tr("Remove Key"));
        UndoRedo ur;
        ur.fUndo = [this, newKey]() { anim_appendKey(newKey); };
        ur.fRedo = [this, newKey]() { anim_removeKey(newKey); };
        prp_addUndoRedo(ur);
    }
}

void Animator::anim_appendKey(const stdsptr<Key>& newKey) {
    const bool isComplex = toComplexAnimator();
    if(!isComplex) anim_setRecordingValue(true);
    anim_mKeys.add(newKey);
    if(newKey->getRelFrame() == anim_mCurrentRelFrame)
        anim_setKeyOnCurrentFrame(newKey.get());
    if(!isComplex) anim_updateAfterChangedKey(newKey.get());
    emit anim_addedKey(newKey.get(), QPrivateSignal());
}

void Animator::anim_removeKey(const stdsptr<Key>& keyToRemove) {
    anim_removeKeyFromSelected(keyToRemove.get());
    removeKeyWithoutDeselecting(keyToRemove);
}

void Animator::anim_removeKeys(const FrameRange& relRange,
                               const bool action) {
    int i = relRange.adjusted(-1, 0).fMin;
    while(i < relRange.fMax) {
        const auto key = anim_getNextKey(i);
        if(!key) break;
        const int keyFrame = key->getRelFrame();
        const bool inRange = relRange.inRange(keyFrame);
        if(inRange) {
            if(action) anim_removeKeyAction(key->ref<Key>());
            else anim_removeKey(key->ref<Key>());
        }
        i = keyFrame;
    }
}

void Animator::removeKeyWithoutDeselecting(const stdsptr<Key>& keyToRemove) {
    Key * const keyPtr = keyToRemove.get();
    anim_updateAfterChangedKey(keyPtr);
    anim_mKeys.remove(keyToRemove);

    const int rFrame = keyPtr->getRelFrame();
    if(rFrame == anim_mCurrentRelFrame)
        anim_setKeyOnCurrentFrame(nullptr);
    emit anim_removedKey(keyPtr, QPrivateSignal());
}

void Animator::anim_moveKeyToRelFrame(Key * const key, const int newFrame) {
    const auto keySPtr = key->ref<Key>();
    removeKeyWithoutDeselecting(keySPtr);
    key->setRelFrame(newFrame);
    anim_appendKey(keySPtr);
}

void Animator::anim_updateKeyOnCurrrentFrame() {
    const auto key = anim_getKeyAtRelFrame(anim_mCurrentRelFrame);
    anim_setKeyOnCurrentFrame(key);
}

Key *Animator::anim_getKeyAtPos(const qreal relX,
                                const int minViewedFrame,
                                const qreal pixelsPerFrame,
                                const int keyRectSize) {
    const qreal timelineRelFrame = relX/pixelsPerFrame - 0.5;
    const qreal absX = relX + minViewedFrame*pixelsPerFrame;
    const qreal absFrame = timelineRelFrame + minViewedFrame;
    qreal keySize = keyRectSize;
    if(toComplexAnimator()) keySize *= 0.75;
    if(pixelsPerFrame > keySize) {
        const int relFrameInt = qRound(timelineRelFrame);
        const qreal distToFrame = (relFrameInt + 0.5)*pixelsPerFrame - relX;
        if(qAbs(2*distToFrame) > keySize) return nullptr;
    }
    //if(pressFrame < 0) pressFrame -= 1.;
    const qreal keyRectFramesSpan = 0.5*keySize/pixelsPerFrame;
    const int minPossibleKey = qFloor(absFrame - keyRectFramesSpan);
    const int maxPossibleKey = qCeil(absFrame + keyRectFramesSpan);
    Key* keyAtPos = nullptr;
    for(int i = maxPossibleKey; i >= minPossibleKey; i--) {
        const qreal distToFrame = qAbs((i + 0.5)*pixelsPerFrame - absX);
        if(2*distToFrame > keySize) continue;
        keyAtPos = anim_getKeyAtAbsFrame(i);
        if(keyAtPos) return keyAtPos;
    }
    return nullptr;
}

void Animator::anim_addAllKeysToComplexAnimator(ComplexAnimator *target) {
    for(const auto &key : anim_mKeys) {
        target->ca_addDescendantsKey(key);
    }
}

void Animator::anim_removeAllKeysFromComplexAnimator(ComplexAnimator *target) {
    for(const auto &key : anim_mKeys) {
        target->ca_removeDescendantsKey(key);
    }
}

bool Animator::anim_hasKeys() const {
    return !anim_mKeys.isEmpty();
}

void Animator::anim_setRecordingWithoutChangingKeys(const bool rec) {
    anim_setRecordingValue(rec);
}

void Animator::anim_setRecordingValue(const bool rec)
{
    if (rec == anim_mIsRecording) { return; }
    {
        prp_pushUndoRedoName(tr("Set Recording"));
        UndoRedo ur;
        const auto oldValue = anim_mIsRecording;
        const auto newValue = rec;
        ur.fUndo = [this, oldValue]() { anim_setRecordingValue(oldValue); };
        ur.fRedo = [this, newValue]() { anim_setRecordingValue(newValue); };
        prp_addUndoRedo(ur);
    }
    anim_mIsRecording = rec;
    emit anim_isRecordingChanged();
}

bool Animator::anim_isRecording() {
    return anim_mIsRecording;
}

void Animator::anim_removeAllKeys() {
    if(anim_mKeys.isEmpty()) return;
    const auto keys = anim_mKeys;
    for(const auto& key : keys)
        anim_removeKeyAction(key->ref<Key>());
}

void Animator::anim_setKeyOnCurrentFrame(Key* const key) {
    if(key == anim_mKeyOnCurrentFrame) return;
    anim_mKeyOnCurrentFrame = key;
    anim_afterKeyOnCurrentFrameChanged(key);
    emit anim_changedKeyOnCurrentFrame(key, QPrivateSignal());
}

void Animator::anim_getKeysInRect(const QRectF &selectionRect,
                                 const qreal pixelsPerFrame,
                                 QList<Key*> &keysList,
                                 const int keyRectSize) {
    //selectionRect.translate(-getFrameShift(), 0.);
    int selLeftFrame = qRound(selectionRect.left());
    if(0.5*(pixelsPerFrame + keyRectSize) <
       selectionRect.left() - selLeftFrame*pixelsPerFrame) {
        selLeftFrame++;
    }
    int selRightFrame = qRound(selectionRect.right());
    if(0.5*(pixelsPerFrame - keyRectSize) >
       selectionRect.right() - selRightFrame*pixelsPerFrame) {
        selRightFrame--;
    }
    for(int i = selLeftFrame; i <= selRightFrame; i++) {
        Key * const keyAtPos = anim_getKeyAtAbsFrame(i);
        if(keyAtPos) keysList.append(keyAtPos);
    }
}

std::pair<int, int> Animator::anim_getPrevAndNextKeyId(const int relFrame) const {
    return anim_mKeys.prevAndNextId(relFrame);
}

std::pair<int, int> Animator::anim_getPrevAndNextKeyIdF(const qreal relFrame) const {
    if(isInteger4Dec(relFrame))
        return anim_getPrevAndNextKeyId(qRound(relFrame));
    const int fFrame = qFloor(relFrame);
    const int next = anim_getNextKeyId(fFrame);
    const int cFrame = qCeil(relFrame);
    const int prev = anim_getPrevKeyId(cFrame);
    return {prev, next};
}

int Animator::anim_getCurrentAbsFrame() const {
    return anim_mCurrentAbsFrame;
}

int Animator::anim_getCurrentRelFrame() const {
    return anim_mCurrentRelFrame;
}

FrameRange Animator::prp_getIdenticalRelRange(const int relFrame) const {
    if(anim_mKeys.count() <= 1) return FrameRange::EMINMAX;
    const auto pn = anim_getPrevAndNextKeyId(relFrame);
    const int prevId = pn.first;
    const int nextId = pn.second;

    Key *prevKey = anim_getKeyAtIndex(prevId);
    Key *nextKey = anim_getKeyAtIndex(nextId);
    const bool adjKeys = nextId - prevId == 1;
    Key * const keyAtRelFrame = adjKeys ? nullptr :
                                          anim_getKeyAtIndex(pn.first + 1);
    Key *prevPrevKey = keyAtRelFrame ? keyAtRelFrame : nextKey;
    Key *prevNextKey = keyAtRelFrame ? keyAtRelFrame : prevKey;

    int fId = relFrame;
    int lId = relFrame;

    int idIt = prevId;
    while(true) {
        if(!prevKey) {
            fId = FrameRange::EMIN;
            break;
        }
        if(prevPrevKey) {
            if(prevKey->differsFromKey(prevPrevKey)) break;
        }
        fId = prevKey->getRelFrame();
        prevPrevKey = prevKey;
        prevKey = anim_getKeyAtIndex(--idIt);
    }

    idIt = nextId;
    while(true) {
        if(!nextKey) {
            lId = FrameRange::EMAX;
            break;
        }
        if(prevNextKey) {
            if(nextKey->differsFromKey(prevNextKey)) break;
        }
        lId = nextKey->getRelFrame();
        prevNextKey = nextKey;
        nextKey = anim_getKeyAtIndex(++idIt);
    }

    return {fId, lId};
}

FrameRange Animator::prp_nextNonUnaryIdenticalRelRange(const int relFrame) const {
    if(anim_mKeys.count() <= 1) return FrameRange::EMINMAX;
    const auto pn = anim_getPrevAndNextKeyId(relFrame);
    const int prevId = pn.first;
    const int nextId = pn.second;

    Key *nextKey = anim_getKeyAtIndex(nextId);
    const bool adjKeys = nextId - prevId == 1;
    Key * const keyAtRelFrame = adjKeys ? nullptr :
                                          anim_getKeyAtIndex(pn.first + 1);
    Key *prevKey = keyAtRelFrame ? keyAtRelFrame : anim_getKeyAtIndex(prevId);

    int i = relFrame;

    int idIt = nextId;
    while(true) {
        if(!nextKey || (prevKey && !nextKey->differsFromKey(prevKey))) {
            i = prevKey->getRelFrame();
            break;
        }
        prevKey = nextKey;
        nextKey = anim_getKeyAtIndex(++idIt);
    }

    return prp_getIdenticalRelRange(i);
}

void Animator::anim_saveCurrentValueAsKey() {
    anim_addKeyAtRelFrame(anim_getCurrentRelFrame());
}

enum class KeyFrameType {
    object, propertyGroup, property
};

void anim_drawKey(QPainter * const p,
                  Key * const key,
                  const qreal pixelsPerFrame,
                  const int startFrame,
                  const int rowHeight,
                  const QColor& color,
                  const QColor& selectedColor,
                  const qreal keyRadius,
                  const KeyFrameType type) {
    if(key->isSelected()) p->setBrush(selectedColor);
    else p->setBrush(color);

    if(key->isHovered()) p->setPen(QPen(Qt::black, 1.5));
    else p->setPen(QPen(Qt::black, 0.5));

    const int frameRelToStart = key->getRelFrame() - startFrame;
    const QPointF keyCenter((frameRelToStart + 0.5)*pixelsPerFrame,
                            0.5*rowHeight);
    switch(type) {
    case KeyFrameType::property: {
        const QPointF tl = keyCenter + QPointF(-keyRadius, -keyRadius);
        p->drawRect(QRectF(tl, QSizeF(2*keyRadius, 2*keyRadius)));
    } break;
    default: {
        p->drawEllipse(keyCenter, keyRadius, keyRadius);
    }
    }
}

void Animator::prp_drawTimelineControls(
        QPainter * const p, const qreal pixelsPerFrame,
        const FrameRange &absFrameRange, const int rowHeight) {
    p->translate(prp_getTotalFrameShift()*pixelsPerFrame, 0);
    const auto relRange = prp_absRangeToRelRange(absFrameRange);
    const auto idRange = anim_frameRangeToKeyIdRange(relRange);
    KeyFrameType type;
    if(toBoundingBox()) type = KeyFrameType::object;
    else if(toComplexAnimator()) type = KeyFrameType::propertyGroup;
    else type = KeyFrameType::property;
    const auto& sett = *eSettings::sInstance;
    QColor color;
    switch(type) {
    case KeyFrameType::object:
        color = sett.fObjectKeyframeColor;
        break;
    case KeyFrameType::propertyGroup:
        color = sett.fPropertyGroupKeyframeColor;
        break;
    case KeyFrameType::property:
        color = sett.fPropertyKeyframeColor;
        break;
    }
    qreal radMult;
    if(type == KeyFrameType::object) radMult = 0.3;
    else radMult = 0.21;
    const qreal keyRadius = rowHeight * radMult;
    for(int i = idRange.fMin; i <= idRange.fMax; i++) {
        if(i < 0 || i >= anim_mKeys.count()) continue;
        const auto& key = anim_mKeys.atId(i);
        anim_drawKey(p, key, pixelsPerFrame, absFrameRange.fMin, rowHeight,
                     color, sett.fSelectedKeyframeColor, keyRadius, type);
    }
}

#include "typemenu.h"
void Animator::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    if(menu->hasActionsForType<Animator>()) return;
    menu->addedActionsForType<Animator>();

    const PropertyMenu::PlainSelectedOp<Animator> aOp =
    [](Animator * animTarget) {
        animTarget->anim_saveCurrentValueAsKey();
    };
    menu->addPlainAction(QIcon::fromTheme("plus"), tr("Add Key(s)"), aOp)->setDisabled(anim_getKeyOnCurrentFrame());

    const PropertyMenu::PlainSelectedOp<Animator> dOp =
    [](Animator * animTarget) {
        animTarget->anim_deleteCurrentKeyAction();
    };
    menu->addPlainAction(QIcon::fromTheme("trash"), tr("Delete Key(s)"), dOp)->setEnabled(anim_getKeyOnCurrentFrame());

    menu->addSeparator();
    Property::prp_setupTreeViewMenu(menu);
}

void Animator::prp_afterFrameShiftChanged(const FrameRange &oldAbsRange,
                                          const FrameRange &newAbsRange) {
    anim_updateRelFrame();
    Property::prp_afterFrameShiftChanged(oldAbsRange, newAbsRange);
}

bool Animator::anim_hasSelectedKeys() const {
    return !anim_mSelectedKeys.isEmpty();
}

void Animator::anim_addKeyToSelected(Key * const key) {
    if(key->isSelected()) return;
    anim_mSelectedKeys << key;
    key->setSelected(true);
}

void Animator::anim_removeKeyFromSelected(Key * const key) {
    if(key->isSelected()) {
        key->setSelected(false);
        anim_mSelectedKeys.removeOne(key);
    }
}

void Animator::anim_deselectAllKeys() {
    for(const auto& key : qAsConst(anim_mSelectedKeys)) {
        key->setSelected(false);
    }
    anim_mSelectedKeys.clear();
}

void Animator::anim_selectAllKeys() {
    for(const auto& key : anim_mKeys) {
        anim_addKeyToSelected(key);
    }
}

void Animator::anim_incSelectedKeysFrame(const int dFrame) {
    for(const auto& key : qAsConst(anim_mSelectedKeys)) {
        const int newFrame = key->getRelFrame() + dFrame;
        anim_moveKeyToRelFrame(key, newFrame);
    }
}

void Animator::anim_scaleSelectedKeysFrame(const int absPivotFrame,
                                      const qreal scale) {
    for(const auto& key : qAsConst(anim_mSelectedKeys)) {
        key->scaleFrameAndUpdateParentAnimator(absPivotFrame, scale, true);
    }
}

void Animator::anim_cancelSelectedKeysTransform() {
    for(const auto& key : qAsConst(anim_mSelectedKeys)) {
        key->cancelFrameTransform();
    }
}

void Animator::anim_finishSelectedKeysTransform() {
    for(const auto& key : qAsConst(anim_mSelectedKeys)) {
        key->finishFrameTransform();
    }
    anim_mergeKeysIfNeeded();
}

void Animator::anim_startSelectedKeysTransform() {
    for(const auto& key : qAsConst(anim_mSelectedKeys)) {
        key->startFrameTransform();
    }
}

void Animator::anim_deleteSelectedKeys() {
    for(const auto& key : qAsConst(anim_mSelectedKeys)) {
        anim_removeKeyAction(key->ref<Key>());
    }
    anim_mSelectedKeys.clear();
}

int Animator::anim_getLowestAbsFrameForSelectedKey()
{
    int lowestKey = FrameRange::EMAX;
    for (const auto& key : qAsConst(anim_mSelectedKeys)) {
        int keyAbsFrame = key->getAbsFrame();
        if (keyAbsFrame < lowestKey) { lowestKey = keyAbsFrame; }
    }
    return lowestKey;
}

int Animator::anim_getHighestAbsFrameForSelectedKey()
{
    int highestKey = FrameRange::EMIN;
    for (const auto& key : qAsConst(anim_mSelectedKeys)) {
        int keyAbsFrame = key->getAbsFrame();
        if (keyAbsFrame > highestKey) { highestKey = keyAbsFrame; }
    }
    return highestKey;
}

void Animator::saveSVG(SvgExporter& exp,
                       QDomElement& parent,
                       const FrameRange& visRange,
                       const QString& attrName,
                       const ValueGetter& valueGetter,
                       const bool transform,
                       const QString& type,
                       const QString& interpolation,
                       const QList<Animator*> extInfl) const {
    Q_ASSERT(!transform || attrName == "transform");

    const auto thisIdRange = prp_getIdenticalRelRange(visRange.fMin);
    auto idRange = thisIdRange;
    for(const auto& infl : extInfl) {
        const auto extIdRange = infl->prp_getIdenticalRelRange(visRange.fMin);
        idRange = idRange*extIdRange;
    }
    const int span = exp.fAbsRange.span();
    if(idRange.inRange(visRange) || span == 1) {
        auto value = valueGetter(visRange.fMin);
        if(transform) {
            value = parent.attribute(attrName) + " " +
                    type + "(" + value + ")";
        }
        parent.setAttribute(attrName, value.trimmed());
    } else {
        const auto tagName = transform ? "animateTransform" : "animate";
        auto anim = exp.createElement(tagName);
        anim.setAttribute("calcMode", interpolation);
        anim.setAttribute("attributeName", attrName);
        if(!type.isEmpty()) anim.setAttribute("type", type);
        const qreal div = span - 1;
        const qreal dur = div/exp.fFps;
        anim.setAttribute("dur", QString::number(dur)  + 's');
        int i = visRange.fMin;
        QStringList values;
        QStringList keyTimes;
        while(true) {
            const auto value = valueGetter(i);
            values << value;
            const auto thisIRange = exp.fAbsRange*prp_getIdenticalAbsRange(i);
            FrameRange iRange = thisIRange;
            for(const auto& infl : extInfl) {
                const auto extIdRange = infl->prp_getIdenticalAbsRange(i);
                iRange = iRange*extIdRange;
            }
            const qreal minTime = (iRange.fMin - exp.fAbsRange.fMin)/div;
            keyTimes << QString::number(minTime);
            if(iRange.fMin != iRange.fMax) {
                values << value;
                const qreal maxTime = (iRange.fMax - exp.fAbsRange.fMin)/div;
                keyTimes << QString::number(maxTime);
            }
            if(iRange.fMax >= visRange.fMax) break;
            int newI = prp_nextDifferentRelFrame(i);
            for(const auto& infl : extInfl) {
                const auto extI = infl->prp_nextDifferentRelFrame(i);
                newI = qMin(extI, newI);
            }
            i = newI;
        }
        if(keyTimes.isEmpty()) return;
        if(keyTimes.last() != "1") {
            values << values.last();
            keyTimes << "1";
        }
        if(keyTimes.first() != "0") {
            values.prepend(values.first());
            keyTimes.prepend("0");
        }
        anim.setAttribute("values", values.join(';'));
        anim.setAttribute("keyTimes", keyTimes.join(';'));
        SvgExportHelpers::assignLoop(anim, exp.fLoop);
        parent.appendChild(anim);
    }
}

void Animator::saveSVG(SvgExporter& exp,
                       QDomElement& parent,
                       const FrameRange& visRange,
                       const QString& attrName,
                       const ValueGetter& valueGetter,
                       const QString& interpolation,
                       const QList<Animator*> extInfl) const {
    saveSVG(exp, parent, visRange, attrName, valueGetter,
            false, "", interpolation, extInfl);
}
