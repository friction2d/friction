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

#include "esoundobjectbase.h"

#include "esoundlink.h"
#include "fileshandler.h"
#include "Timeline/fixedlenanimationrect.h"

eSoundObjectBase::eSoundObjectBase(const qsptr<FixedLenAnimationRect>& durRect) {
    connect(this, &eBoxOrSound::prp_ancestorChanged, this, [this]() {
        if(!getParentScene()) return;
        updateDurationRectLength();
    });
    setDurationRectangle(durRect, true);
    durRect->setSoundCacheHandler(getCacheHandler());

    ca_addChild(mVolumeAnimator);
}

#include <QInputDialog>
#include "typemenu.h"
void eSoundObjectBase::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    if(menu->hasActionsForType<eSoundObjectBase>()) return;
    menu->addedActionsForType<eSoundObjectBase>();
    const PropertyMenu::CheckSelectedOp<eSoundObjectBase> enableOp =
            [](eSoundObjectBase * sound, bool enable) {
        sound->setVisible(enable);
    };
    menu->addCheckableAction("Enabled", isVisible(), enableOp);
}

SoundReaderForMerger *eSoundObjectBase::getSecondReader(const int relSecondId) {
    if(!mCacheHandler) return nullptr;
    const int maxSec = mCacheHandler->durationSecCeil() - 1;
    if(relSecondId < 0 || relSecondId > maxSec) return nullptr;
    const auto reader = mCacheHandler->getSecondReader(relSecondId);
    if(!reader) return mCacheHandler->addSecondReader(relSecondId);
    return reader;
}

stdsptr<Samples> eSoundObjectBase::getSamplesForSecond(const int relSecondId) {
    if(!mCacheHandler) return nullptr;
    return mCacheHandler->getSamplesForSecond(relSecondId);
}

qreal eSoundObjectBase::durationSeconds() const {
    return mCacheHandler ? mCacheHandler->durationSec() : 0;
}

qsptr<eSound> eSoundObjectBase::createLink() {
    return enve::make_shared<eSoundLink>(this);
}

const HddCachableCacheHandler *eSoundObjectBase::getCacheHandler() const {
    if(!mCacheHandler) return nullptr;
    return &mCacheHandler->getCacheHandler();
}

void eSoundObjectBase::setStretch(const qreal stretch)
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
    updateDurationRectLength();
    prp_afterWholeInfluenceRangeChanged();
}

QrealSnapshot eSoundObjectBase::getVolumeSnap() const {
    return mVolumeAnimator->makeSnapshot(
                eSoundSettings::sSampleRate()/getCanvasFPS(), 0.01);
}

void eSoundObjectBase::setSoundDataHandler(SoundDataHandler* const newDataHandler) {
    mLastRequestedSec = -1;
    if(mCacheFlashTimer) mCacheFlashTimer->stop();

    if(newDataHandler) mCacheHandler = enve::make_shared<SoundHandler>(newDataHandler);
    else mCacheHandler.reset();
    const auto durRect = getDurationRectangle();
    durRect->setSoundCacheHandler(getCacheHandler());
    updateDurationRectLength();

    if(mCacheHandler) {
        QPointer<eSoundObjectBase> self(this);
        mCacheHandler->mOnSecondCached = [self](int secondId) {
            if(self) self->onCachedSecond(secondId);
        };
        startBackgroundCaching();
    }
}

void eSoundObjectBase::startBackgroundCaching() {
    if(!mCacheHandler || mCacheHandler->durationSecCeil() <= 0) return;

    // Find the first uncached second to start from
    const int total = mCacheHandler->durationSecCeil();
    int startSec = -1;
    for(int sec = 0; sec < total; ++sec) {
        if(!getSamplesForSecond(sec)) { startSec = sec; break; }
    }
    if(startSec < 0) return; // already fully cached

    mLastRequestedSec = startSec;
    mCacheHandler->addSecondReader(startSec);

    if(!mCacheFlashTimer) {
        mCacheFlashTimer = new QTimer(this);
        mCacheFlashTimer->setInterval(500);
        connect(mCacheFlashTimer, &QTimer::timeout, this, [this]() {
            prp_afterWholeInfluenceRangeChanged();
        });
    }
    mCacheFlashTimer->start();
}

void eSoundObjectBase::onCachedSecond(const int secondId) {
    if(!mCacheHandler) return;
    const int next = secondId + 1;
    const int total = mCacheHandler->durationSecCeil();
    if(next < total && !getSamplesForSecond(next)) {
        mLastRequestedSec = next;
        mCacheHandler->addSecondReader(next);
    } else if(next >= total) {
        if(mCacheFlashTimer) mCacheFlashTimer->stop();
    }
    prp_afterWholeInfluenceRangeChanged();
}
