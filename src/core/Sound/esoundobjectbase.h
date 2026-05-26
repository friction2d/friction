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

#ifndef ESOUNDOBJECTBASE_H
#define ESOUNDOBJECTBASE_H

#include "esound.h"

#include "CacheHandlers/soundcachehandler.h"
#include "FileCacheHandlers/filehandlerobjref.h"

#include <QTimer>
#include <QPointer>

class FixedLenAnimationRect;

class CORE_EXPORT eSoundObjectBase : public eSound {
protected:
    eSoundObjectBase(const qsptr<FixedLenAnimationRect> &durRect);

    virtual void updateDurationRectLength() = 0;
public:
    void prp_setupTreeViewMenu(PropertyMenu * const menu) override;

    SoundReaderForMerger * getSecondReader(const int relSecondId) final;
    stdsptr<Samples> getSamplesForSecond(const int relSecondId) final;

    qsptr<eSound> createLink() final;

    qreal durationSeconds() const final;
    qreal getStretch() const final { return mStretch; }
    QrealSnapshot getVolumeSnap() const final;

    void setStretch(const qreal stretch);
    void setSoundDataHandler(SoundDataHandler * const newDataHandler);

    int lastRequestedSecond() const override { return mLastRequestedSec; }
protected:
    const SoundHandler* cacheHandler() const
    { return mCacheHandler.get(); }
private:
    const HddCachableCacheHandler* getCacheHandler() const;

    void startBackgroundCaching();
    void onCachedSecond(int secondId);

    qreal mStretch = 1;
    stdsptr<SoundHandler> mCacheHandler;
    QTimer* mCacheFlashTimer = nullptr;
    int mLastRequestedSec = -1;

    qsptr<QrealAnimator> mVolumeAnimator =
            enve::make_shared<QrealAnimator>(100, 0, 200, 1, "volume");
};

#endif // ESOUNDOBJECTBASE_H
