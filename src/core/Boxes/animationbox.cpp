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

#include "animationbox.h"

#include <QMenu>
#include <QInputDialog>

#include "Private/scene.h"
#include "FileCacheHandlers/animationcachehandler.h"
#include "imagebox.h"
#include "undoredo.h"
#include "Animators/qrealkey.h"
#include "RasterEffects/rastereffectcollection.h"
#include "frameremapping.h"
#include "typemenu.h"
#include "Private/Tasks/complextask.h"
#include "Private/Tasks/taskscheduler.h"
#include "GUI/dialogsinterface.h"
#include "svgexporter.h"

struct AnimationBoxRenderData : public ImageContainerRenderData {
    AnimationBoxRenderData(AnimationFrameHandler *cacheHandler,
                           BoundingBox *parentBox);

    void loadImageFromHandler();

    const qptr<AnimationFrameHandler> fSrcCacheHandler;
    int fAnimFrame;
};

AnimationBox::AnimationBox(const QString &name, const eBoxType type) :
    BoundingBox(name, type) {
    connect(this, &eBoxOrSound::parentChanged,
            this, &AnimationBox::updateAnimationRange);

    setDurationRectangle(enve::make_shared<FixedLenAnimationRect>(*this), true);

    mFrameRemapping = enve::make_shared<IntFrameRemapping>();
    ca_prependChild(mRasterEffectsAnimators.get(), mFrameRemapping);
    mFrameRemapping->disableAction();

    connect(mFrameRemapping.get(), &IntFrameRemapping::enabledChanged,
            this, &AnimationBox::updateAnimationRange);
}

FixedLenAnimationRect *AnimationBox::getAnimationDurationRect() const {
    return static_cast<FixedLenAnimationRect*>(getDurationRectangle());
}

void AnimationBox::updateAnimationRange() {
    const auto durRect = getAnimationDurationRect();
    if(mFrameRemapping->enabled()) {
        const int nFrames = durRect->getFrameDuration();
        durRect->setAnimationFrameDuration(0);
        durRect->setFramesDuration(nFrames);
    } else {
        int frameCount;
        if(mSrcFramesCache) frameCount = mSrcFramesCache->getFrameCount();
        else frameCount = 0;
        const int nFrames = qRound(frameCount*qAbs(mStretch));
        durRect->setAnimationFrameDuration(nFrames);
    }
}

void AnimationBox::animationDataChanged() {
    updateAnimationRange();
    if(mFrameRemapping->enabled()) {
        int frameCount;
        if(mSrcFramesCache) frameCount = mSrcFramesCache->getFrameCount();
        else frameCount = 1;
        mFrameRemapping->setFrameCount(frameCount);
    }
    prp_afterWholeInfluenceRangeChanged();
}

void AnimationBox::setStretch(const qreal stretch)
{
    {
        prp_pushUndoRedoName(tr("Stretch"));
        UndoRedo ur;
        const auto oldValue = mStretch;
        const auto newValue = stretch;
        ur.fUndo = [this, oldValue]() { setStretch(oldValue); };
        ur.fRedo = [this, newValue]() { setStretch(newValue); };
        prp_addUndoRedo(ur);
    }
    mStretch = stretch;
    prp_afterWholeInfluenceRangeChanged();
    updateAnimationRange();
}

bool AnimationBox::shouldScheduleUpdate() {
    if(!mSrcFramesCache) return false;
    return BoundingBox::shouldScheduleUpdate();
}

int AnimationBox::getAnimationFrameForRelFrame(const qreal relFrame) {
    const int lastFrameId = mSrcFramesCache->getFrameCount() - 1;
    const int animStartRelFrame =
                getAnimationDurationRect()->getMinAnimRelFrame();
    int pixId;
    if(mFrameRemapping->enabled()) {
        pixId = mFrameRemapping->frame(relFrame);
    } else {
        if(isZero6Dec(mStretch)) {
            pixId = lastFrameId;
        } else {
            pixId = qRound((relFrame - animStartRelFrame)/qAbs(mStretch));
            if(mStretch < 0) pixId = lastFrameId - pixId;
        }
    }

    if(pixId < 0) pixId = 0;
    else if(pixId > lastFrameId) pixId = lastFrameId;

    return pixId;
}

void AnimationBox::enableFrameRemappingAction() {
    const int frameCount = mSrcFramesCache->getFrameCount();
    const auto durRect = getAnimationDurationRect();
    const int animStartRelFrame = durRect->getMinAnimRelFrame();
    mFrameRemapping->enableAction(0, frameCount - 1, animStartRelFrame);
}

void AnimationBox::disableFrameRemappingAction() {
    mFrameRemapping->disableAction();
}

void AnimationBox::reload() {
    if(mSrcFramesCache) mSrcFramesCache->reload();
}

void AnimationBox::setAnimationFramesHandler(const qsptr<AnimationFrameHandler>& src) {
    mSrcFramesCache = src;
}

void AnimationBox::anim_setAbsFrame(const int frame) {
    BoundingBox::anim_setAbsFrame(frame);
    if(!mSrcFramesCache) return;
    planUpdate(UpdateReason::frameChange);
}

FrameRange AnimationBox::prp_getIdenticalRelRange(const int relFrame) const {
    if(isVisibleAndInDurationRect(relFrame) && !mFrameRemapping->enabled()) {
        const auto animDur = getAnimationDurationRect();
        const auto animRange = animDur->getAnimRelRange();
        if(animRange.inRange(relFrame))
            return {relFrame, relFrame};
        else if(relFrame > animRange.fMax) {
            const auto baseRange = BoundingBox::prp_getIdenticalRelRange(relFrame);
            const FrameRange durRect{animRange.fMax + 1,
                                     animDur->getRelFrameRange().fMax};
            return baseRange*durRect;
        } else if(relFrame < animRange.fMin) {
            const auto baseRange = BoundingBox::prp_getIdenticalRelRange(relFrame);
            const FrameRange durRect{animDur->getRelFrameRange().fMin,
                                     animRange.fMin - 1};
            return baseRange*durRect;
        }
    }
    return BoundingBox::prp_getIdenticalRelRange(relFrame);
}

class AnimationToPaint : public ComplexTask {
public:
    using Loader = std::function<void(int, int)>;
    AnimationToPaint(const int firstAbsFrame, const int firstRelFrame,
                     const int iMax, const int increment,
                     AnimationBox* const box,
                     AnimationFrameHandler* const src,
                     const Loader& loader) :
        ComplexTask(iMax, "Video to Paint"),
        mFirstAbsFrame(firstAbsFrame), mFirstRelFrame(firstRelFrame),
        mIncrement(increment), mBox(box), mSrc(src), mLoader(loader) {}

    void nextStep() {
        if(!mBox || !mSrc) return cancel();
        if(setValue(mI)) return;

        const int relFrame = mFirstRelFrame + mI;
        const int absFrame = mFirstAbsFrame + mI;
        const int animFrame = mBox->getAnimationFrameForRelFrame(relFrame);
        auto task = mSrc->scheduleFrameLoad(animFrame);
        if(task) addTask(task->ref<eTask>());
        else task = addEmptyTask();
        const QPointer<AnimationToPaint> ptr = this;
        task->addDependent({[ptr, animFrame, absFrame]() {
            if(ptr) ptr->mLoader(animFrame, absFrame);
        }, nullptr});
        mI += mIncrement;
    }
private:
    const int mFirstAbsFrame;
    const int mFirstRelFrame;
    const int mIncrement;

    const QPointer<AnimationBox> mBox;
    const QPointer<AnimationFrameHandler> mSrc;
    const Loader mLoader;

    int mI = 0;
};

void AnimationBox::setupCanvasMenu(PropertyMenu * const menu)
{
    if (menu->hasActionsForType<AnimationBox>()) { return; }
    menu->addedActionsForType<AnimationBox>();

    const auto widget = menu->getParentWidget();

    const PropertyMenu::PlainSelectedOp<AnimationBox> reloadOp =
    [](AnimationBox * box) { box->reload(); };
    menu->addPlainAction(QIcon::fromTheme("loop"), tr("Reload"), reloadOp);

    const PropertyMenu::PlainSelectedOp<AnimationBox> setSrcOp =
    [](AnimationBox * box) { box->changeSourceFile(); };
    menu->addPlainAction(QIcon::fromTheme("document-new"), tr("Set Source File"), setSrcOp);

    const PropertyMenu::CheckSelectedOp<AnimationBox> remapOp =
    [](AnimationBox * box, bool checked) {
        if (checked) { box->enableFrameRemappingAction(); }
        else { box->disableFrameRemappingAction(); }
    };
    menu->addCheckableAction(tr("Frame Remapping"),
                             mFrameRemapping->enabled(), remapOp);

    const PropertyMenu::PlainSelectedOp<AnimationBox> stretchOp =
    [widget](AnimationBox * box) {
        bool ok = false;
        const int stretch = QInputDialog::getInt(widget,
                                                 "Stretch " + box->prp_getName(),
                                                 "Stretch:",
                                                 qRound(box->getStretch()*100),
                                                 -1000, 1000, 1, &ok);
        if (!ok) { return; }
        box->setStretch(stretch*0.01);
    };
    menu->addPlainAction(QIcon::fromTheme("width"), tr("Stretch"), stretchOp);

    menu->addSeparator();
    BoundingBox::setupCanvasMenu(menu);
}

//void AnimationBox::createPaintObject(const int firstAbsFrame,
  //                                   const int lastAbsFrame,
    //                                 const int increment) {
    /*const qptr<ContainerBox> parent = getParentGroup();
    if(!parent) return;
    const auto paintObj = enve::make_shared<PaintBox>();
    copyBoundingBoxDataTo(paintObj.get());
    const auto surface = paintObj->getSurface();
    surface->anim_setAbsFrame(firstAbsFrame);
    surface->anim_setRecording(true);
    const auto src = mSrcFramesCache.get();
    const auto loader = [src, surface](const int animFrame, const int absFrame) {
        const auto cont = src->getFrameAtFrame(animFrame);
        if(!cont) return;
        surface->anim_setAbsFrame(absFrame);
        surface->loadPixmap(cont->getImage());
    };
    const auto adder = [paintObj, parent]() {
        if(!parent) return;
        parent->addContained(paintObj);
    };
    const int firstRelFrame = prp_absFrameToRelFrame(firstAbsFrame);
    const int iMax = lastAbsFrame - firstAbsFrame;

    const auto task = new AnimationToPaint(firstAbsFrame, firstRelFrame, iMax,
                                           increment, this, src, loader);
    const auto taskSPtr = QSharedPointer<AnimationToPaint>(
                              task, &QObject::deleteLater);
    task->nextStep();

    if(task->done()) {
        adder();
    } else {
        connect(task, &ComplexTask::finishedAll, paintObj.get(), adder);
        TaskScheduler::instance()->addComplexTask(taskSPtr);
    }*/
//}

void AnimationBox::setupRenderData(const qreal relFrame,
                                   const QMatrix& parentM,
                                   BoxRenderData * const data,
                                   Scene* const scene) {
    BoundingBox::setupRenderData(relFrame, parentM, data, scene);
    if(!mSrcFramesCache) return;
    const auto imgData = static_cast<AnimationBoxRenderData*>(data);
    const int animFrame = getAnimationFrameForRelFrame(relFrame);
    imgData->fAnimFrame = animFrame;
    const auto upd = mSrcFramesCache->scheduleFrameLoad(animFrame);
    if(upd) upd->addDependent(imgData);
    else {
        const auto cont = mSrcFramesCache->getFrameAtFrame(animFrame);
        imgData->setContainer(cont);
    }
}

stdsptr<BoxRenderData> AnimationBox::createRenderData() {
    return enve::make_shared<AnimationBoxRenderData>(mSrcFramesCache.get(), this);
}

void AnimationBox::saveSVG(SvgExporter& exp, DomEleTask* const task) const {
    if(!mSrcFramesCache) return BoundingBox::saveSVG(exp, task);
    auto& ele = task->initialize("use");
    const auto relRange = prp_absRangeToRelRange(exp.fAbsRange);
    const auto visRelRange = prp_absRangeToRelRange(task->visRange());
    const auto aTask = mSrcFramesCache->saveAnimationSVG(exp, ele, relRange,
                                                         visRelRange);
    if(aTask) aTask->addDependent(task);
}

AnimationBoxRenderData::AnimationBoxRenderData(
        AnimationFrameHandler *cacheHandler, BoundingBox *parentBox) :
    ImageContainerRenderData(parentBox),
    fSrcCacheHandler(cacheHandler) {}

void AnimationBoxRenderData::loadImageFromHandler() {
    if(!fSrcCacheHandler) return;
    const auto cont = fSrcCacheHandler->getFrameAtOrBeforeFrame(fAnimFrame);
    setContainer(cont);
}
