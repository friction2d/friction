/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Friction contributors
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

#include "eboxorsound.h"

#include "canvas.h"
#include "Timeline/durationrectangle.h"
#include "Properties/emimedata.h"
#include "Sound/esound.h"

eBoxOrSound::eBoxOrSound(const QString &name) :
    StaticComplexAnimator(name) {
    ca_setDisabledWhenEmpty(false);
    connect(this, &Property::prp_nameChanged, this,
            &SingleWidgetTarget::SWT_scheduleSearchContentUpdate);
}

void eBoxOrSound::setParentGroup(ContainerBox * const parent) {
    if(parent == mParentGroup) return;
    emit aboutToChangeAncestor();

    prp_afterWholeInfluenceRangeChanged();
    auto& conn = mParentGroup.assign(parent);
    if(mParentGroup) {
        anim_setAbsFrame(mParentGroup->anim_getCurrentAbsFrame());
        conn << connect(mParentGroup, &eBoxOrSound::aboutToChangeAncestor,
                        this, &eBoxOrSound::aboutToChangeAncestor);
    }

    setParent(mParentGroup);
    emit parentChanged(parent);
}

void eBoxOrSound::removeFromParent_k() {
    if(!mParentGroup) return;
    mParentGroup->removeContained_k(ref<eBoxOrSound>());
}

bool eBoxOrSound::isAncestor(const BoundingBox * const box) const {
    if(!mParentGroup) return false;
    if(mParentGroup == box) return true;
    if(enve_cast<const ContainerBox*>(box))
        return mParentGroup->isAncestor(box);
    return false;
}

bool eBoxOrSound::isFrameInDurationRect(const int relFrame) const {
    if(!mDurationRectangle) return true;
    return relFrame <= mDurationRectangle->getMaxRelFrame() &&
            relFrame >= mDurationRectangle->getMinRelFrame();
}

bool eBoxOrSound::isFrameFInDurationRect(const qreal relFrame) const {
    if(!mDurationRectangle) return true;
    return qCeil(relFrame) <= mDurationRectangle->getMaxRelFrame() &&
           qFloor(relFrame) >= mDurationRectangle->getMinRelFrame();
}

void eBoxOrSound::shiftAll(const int shift) {
    if(hasDurationRectangle()) mDurationRectangle->changeFramePosBy(shift);
    else anim_shiftAllKeys(shift);
}

QMimeData *eBoxOrSound::SWT_createMimeData() {
    return new eMimeData(QList<eBoxOrSound*>() << this);
}

FrameRange eBoxOrSound::prp_relInfluenceRange() const {
    if(mDurationRectangle) return mDurationRectangle->getRelFrameRange();
    return ComplexAnimator::prp_relInfluenceRange();
}

FrameRange eBoxOrSound::prp_getIdenticalRelRange(const int relFrame) const {
    if(mVisible) {
        const auto cRange = ComplexAnimator::prp_getIdenticalRelRange(relFrame);
        if(mDurationRectangle) {
            const auto dRange = mDurationRectangle->getRelFrameRange();
            if(relFrame > dRange.fMax) {
                return mDurationRectangle->getRelFrameRangeToTheRight();
            } else if(relFrame < dRange.fMin) {
                return mDurationRectangle->getRelFrameRangeToTheLeft();
            } else return cRange*dRange;
        }
        return cRange;
    }
    return {FrameRange::EMIN, FrameRange::EMAX};
}

int eBoxOrSound::prp_getRelFrameShift() const {
    if(!mDurationRectangle) return 0;
    return mDurationRectangle->getRelShift();
}

void eBoxOrSound::prp_afterChangedAbsRange(const FrameRange &range,
                                           const bool clip) {
    const auto croppedRange = clip ? prp_absInfluenceRange()*range : range;
    StaticComplexAnimator::prp_afterChangedAbsRange(croppedRange);
}

void eBoxOrSound::prp_writeProperty_impl(eWriteStream& dst) const {
    StaticComplexAnimator::prp_writeProperty_impl(dst);
    dst << mVisible;
    dst << mLocked;

    const bool hasDurRect = mDurationRectangle;
    dst << hasDurRect;
    if(hasDurRect) mDurationRectangle->writeDurationRectangle(dst);

    dst << prp_getName();
}

void eBoxOrSound::prp_readProperty_impl(eReadStream& src) {
    StaticComplexAnimator::prp_readProperty_impl(src);
    src >> mVisible;
    src >> mLocked;

    bool hasDurRect;
    src >> hasDurRect;
    if(hasDurRect) {
        if(!mDurationRectangle) createDurationRectangle();
        mDurationRectangle->readDurationRectangle(src);
    }
    if(src.evFileVersion() >= 10) {
        QString name; src >> name;
        prp_setName(name);
    }
}

void eBoxOrSound::writeBoxOrSoundXEV(const std::shared_ptr<XevZipFileSaver>& xevFileSaver,
                                     const RuntimeIdToWriteId& objListIdConv,
                                     const QString& path) const {
    QDomDocument doc;
    const auto exp = enve::make_shared<XevExporter>(
                         doc, xevFileSaver, objListIdConv, path);
    auto obj = prp_writeNamedPropertyXEV("Object", *exp);
    if(mDurationRectangle) mDurationRectangle->writeDurationRectangleXEV(obj);

    doc.appendChild(obj);
    auto& fileSaver = xevFileSaver->fileSaver();
    fileSaver.processText(path + "properties.xml",
                          [&](QTextStream& stream) {
        stream << doc.toString();
    });
}

void eBoxOrSound::readBoxOrSoundXEV(XevReadBoxesHandler& boxReadHandler,
                                    ZipFileLoader& fileLoader, const QString& path,
                                    const RuntimeIdToWriteId& objListIdConv) {
    QDomDocument doc;
    fileLoader.process(path + "properties.xml",
                       [&](QIODevice* const src) {
        doc.setContent(src);
    });
    const auto obj = doc.firstChildElement("Object");
    const bool hasDurRect = obj.hasAttribute("visRange");
    if(hasDurRect) {
        if(!mDurationRectangle) createDurationRectangle();
        mDurationRectangle->readDurationRectangleXEV(obj);
    }
    const XevImporter imp(boxReadHandler, fileLoader, objListIdConv, path);
    prp_readPropertyXEV(obj, imp);
}

TimelineMovable *eBoxOrSound::anim_getTimelineMovable(
        const int relX, const int minViewedFrame,
        const qreal pixelsPerFrame) {
    if(!mDurationRectangle) return nullptr;
    return mDurationRectangle->getMovableAt(relX, pixelsPerFrame,
                                            minViewedFrame);
}

void eBoxOrSound::drawDurationRectangle(
        QPainter * const p, const qreal pixelsPerFrame,
        const FrameRange &absFrameRange, const int rowHeight) const {
    if(mDurationRectangle) {
        p->save();
        const int width = qCeil(absFrameRange.span()*pixelsPerFrame);
        const QRect drawRect(0, 0, width, rowHeight);
        const auto pScene = getParentScene();
        const qreal fps = pScene ? pScene->getFps() : 1;
        mDurationRectangle->draw(p, drawRect, fps,
                                 pixelsPerFrame, absFrameRange);
        p->restore();
    }
}

void eBoxOrSound::prp_drawTimelineControls(
        QPainter * const p, const qreal pixelsPerFrame,
        const FrameRange &absFrameRange, const int rowHeight) {
    drawDurationRectangle(p, pixelsPerFrame, absFrameRange, rowHeight);
    ComplexAnimator::prp_drawTimelineControls(
                p, pixelsPerFrame, absFrameRange, rowHeight);
}

void eBoxOrSound::setDurationRectangle(
        const qsptr<DurationRectangle>& durationRect,
        const bool lock) {
    Q_ASSERT(!mDurationRectangleLocked);
    if(mDurationRectangle == durationRect) return;
    if(mDurationRectangleLocked) return;
    if(lock) mDurationRectangleLocked = true;
    const FrameRange oldRange = mDurationRectangle ?
                mDurationRectangle->getAbsFrameRange() :
                FrameRange{FrameRange::EMIN, FrameRange::EMAX};
    const FrameRange newRange = durationRect ?
                durationRect->getAbsFrameRange() :
                FrameRange{FrameRange::EMIN, FrameRange::EMAX};
    const auto oldDurRect = mDurationRectangle.sptr();
    auto& conn = mDurationRectangle.assign(durationRect);
    prp_afterFrameShiftChanged(oldRange, newRange);

    {
        UndoRedo ur;
        ur.fUndo = [this, oldDurRect]() {
            setDurationRectangle(oldDurRect);
        };
        ur.fRedo = [this, durationRect]() {
            setDurationRectangle(durationRect);
        };
        prp_addUndoRedo(ur);
    }

    if(!durationRect) return anim_shiftAllKeys(oldDurRect->getRelShift());
    if(durationRect->getRelShift() != 0)
        anim_shiftAllKeys(-durationRect->getRelShift());

    conn << connect(durationRect.data(), &DurationRectangle::shiftChanged,
            this, [this](const int oldShift, const int newShift) {
        const auto newRange = prp_absInfluenceRange();
        const auto oldRange = newRange.shifted(oldShift - newShift);
        prp_afterFrameShiftChanged(oldRange, newRange);
    });

    conn << connect(durationRect.data(), &DurationRectangle::minRelFrameChanged,
            this, [this](const int oldMin, const int newMin) {
        const int min = qMin(newMin, oldMin);
        const int max = qMax(newMin, oldMin);
        prp_afterChangedRelRange(FrameRange{min, max}.adjusted(-1, 1), false);
    });
    conn << connect(durationRect.data(), &DurationRectangle::maxRelFrameChanged,
            this, [this](const int oldMax, const int newMax) {
        const int min = qMin(newMax, oldMax);
        const int max = qMax(newMax, oldMax);
        prp_afterChangedRelRange(FrameRange{min, max}.adjusted(-1, 1), false);
    });
}

bool eBoxOrSound::durationRectangleLocked() const {
    return mDurationRectangleLocked;
}

bool eBoxOrSound::isVisibleAndInVisibleDurationRect() const {
    return isFrameInDurationRect(anim_getCurrentRelFrame()) && mVisible;
}

bool eBoxOrSound::isVisibleAndInDurationRect(
        const int relFrame) const {
    return isFrameInDurationRect(relFrame) && mVisible;
}

bool eBoxOrSound::isFrameFVisibleAndInDurationRect(
        const qreal relFrame) const {
    return isFrameFInDurationRect(relFrame) && mVisible;
}

bool eBoxOrSound::hasDurationRectangle() const {
    return mDurationRectangle;
}

void eBoxOrSound::startDurationRectPosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->startPosTransform();
    }
}

void eBoxOrSound::finishDurationRectPosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->finishPosTransform();
    }
}

void eBoxOrSound::cancelDurationRectPosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->cancelPosTransform();
    }
}

void eBoxOrSound::moveDurationRect(const int dFrame) {
    if(hasDurationRectangle()) {
        mDurationRectangle->changeFramePosBy(dFrame);
    }
}

void eBoxOrSound::startMinFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->startMinFramePosTransform();
    }
}

void eBoxOrSound::finishMinFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->finishMinFramePosTransform();
    }
}

void eBoxOrSound::cancelMinFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->cancelMinFramePosTransform();
    }
}

void eBoxOrSound::moveMinFrame(const int dFrame) {
    if(hasDurationRectangle()) {
        mDurationRectangle->moveMinFrame(dFrame);
    }
}

void eBoxOrSound::startMaxFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->startMaxFramePosTransform();
    }
}

void eBoxOrSound::finishMaxFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->finishMaxFramePosTransform();
    }
}

void eBoxOrSound::cancelMaxFramePosTransform() {
    if(hasDurationRectangle()) {
        mDurationRectangle->cancelMaxFramePosTransform();
    }
}

void eBoxOrSound::moveMaxFrame(const int dFrame) {
    if(hasDurationRectangle()) {
        mDurationRectangle->moveMaxFrame(dFrame);
    }
}

DurationRectangle *eBoxOrSound::getDurationRectangle() const {
    return mDurationRectangle.get();
}

void eBoxOrSound::createDurationRectangle() {
    const auto durRect = enve::make_shared<DurationRectangle>(*this);
//    durRect->setMinFrame(0);
//    if(mParentScene) durRect->setFramesDuration(mParentScene->getFrameCount());
    durRect->setMinRelFrame(anim_getCurrentRelFrame() - 5);
    durRect->setFramesDuration(10);
    setDurationRectangle(durRect);
}

void eBoxOrSound::setSelected(const bool select) {
    if(mSelected == select) return;
    mSelected = select;
    SWT_scheduleContentUpdate(SWT_BoxRule::selected);
    emit selectionChanged(select);
}

void eBoxOrSound::setGraphSelected(const bool select)
{
    if (mGraphSelected == select) { return; }
    mGraphSelected = select;
}

void eBoxOrSound::select() {
    setSelected(true);
}

void eBoxOrSound::deselect() {
    setSelected(false);
}

void eBoxOrSound::selectionChangeTriggered(const bool shiftPressed) {
    const auto pScene = getParentScene();
    if(!pScene) return;
    const auto bb = enve_cast<BoundingBox*>(this);
    if(!bb) return;
    if(shiftPressed) {
        if(mSelected) {
            pScene->removeBoxFromSelection(bb);
        } else {
            pScene->addBoxToSelection(bb);
        }
    } else {
        pScene->clearBoxesSelection();
        pScene->addBoxToSelection(bb);
    }
}

void eBoxOrSound::setVisible(const bool visible) {
    if(mVisible == visible) return;
    if(!isLink()) {
        if(enve_cast<eSound*>(this)) {
            prp_pushUndoRedoName(visible ? "Mute" : "Unmute");
        } else prp_pushUndoRedoName(visible ? "Hide" : "Show");
        UndoRedo ur;
        const auto oldValue = mVisible;
        const auto newValue = visible;
        ur.fUndo = [this, oldValue]() {
            setVisible(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setVisible(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mVisible = visible;

    if(hasDurationRectangle() && enve_cast<BoundingBox*>(this)) {
        const auto updateRange = prp_absInfluenceRange().adjusted(-1, 1);
        prp_afterChangedAbsRange(updateRange, false);
    } else prp_afterWholeInfluenceRangeChanged();

    SWT_scheduleContentUpdate(SWT_BoxRule::visible);
    SWT_scheduleContentUpdate(SWT_BoxRule::hidden);

    emit visibilityChanged(visible);
}

void eBoxOrSound::switchVisible() {
    setVisible(!mVisible);
}

void eBoxOrSound::switchLocked() {
    setLocked(!mLocked);
}

void eBoxOrSound::hide() {
    setVisible(false);
}

void eBoxOrSound::show() {
    setVisible(true);
}

bool eBoxOrSound::isVisible() const {
    return mVisible;
}

bool eBoxOrSound::isVisibleAndUnlocked() const {
    return isVisible() && !mLocked;
}

bool eBoxOrSound::isLocked() const {
    return mLocked;
}

void eBoxOrSound::lock() {
    setLocked(true);
}

void eBoxOrSound::unlock() {
    setLocked(false);
}

void eBoxOrSound::setLocked(const bool locked) {
    if(locked == mLocked) return;
    const auto pScene = getParentScene();
    if(pScene && mSelected) {
        if(const auto bb = enve_cast<BoundingBox*>(this)) {
            pScene->removeBoxFromSelection(bb);
        }
    }
    mLocked = locked;
    SWT_scheduleContentUpdate(SWT_BoxRule::locked);
    SWT_scheduleContentUpdate(SWT_BoxRule::unlocked);
    emit lockedChanged(locked);
}

void eBoxOrSound::moveUp() {
    mParentGroup->decreaseContainedZInList(this);
}

void eBoxOrSound::moveDown() {
    mParentGroup->increaseContainedZInList(this);
}

void eBoxOrSound::bringToFront() {
    mParentGroup->bringContainedToFrontList(this);
}

void eBoxOrSound::bringToEnd() {
    mParentGroup->bringContainedToEndList(this);
}

void eBoxOrSound::rename(const QString &newName) {
    if(newName == prp_getName()) return;
    const auto fixedName = Property::prp_sFixName(newName);
    const auto parentScene = getParentScene();
    if(parentScene) {
        const QString uniqueName = parentScene->
                makeNameUniqueForDescendants(fixedName, this);
        return prp_setNameAction(uniqueName);
    }
    prp_setNameAction(fixedName);
}
