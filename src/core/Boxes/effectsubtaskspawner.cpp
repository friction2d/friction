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

#include "effectsubtaskspawner.h"
#include "boxrenderdata.h"
#include "Private/Tasks/taskscheduler.h"
#include "skia/skiaincludes.h"
#include "RasterEffects/rastereffect.h"
#include "RasterEffects/rastereffectcaller.h"
#include "Private/Tasks/taskexecutor.h"

#include <atomic>

class EffectSubTaskSpawner_priv {
public:
    EffectSubTaskSpawner_priv(const stdsptr<RasterEffectCaller>& effect,
                              const stdsptr<BoxRenderData>& data) :
        mUseDst(effect->srcDstSeparation()),
        mEffectCaller(effect), mData(data) {}

    void initialize();
private:
    void decRemaining_k();
    void spawn();
    void splitSpawn(CpuRenderData& data,
                    const SkIRect& rect,
                    const int nSplits);

    const bool mUseDst;
    std::atomic<int> mRemaining{0};

    const stdsptr<RasterEffectCaller> mEffectCaller;
    const stdsptr<BoxRenderData> mData;
    SkBitmap mSrcBitmap;
    SkBitmap mDstBitmap;

    sk_sp<SkImage> mSrcRasterImg;
};

void EffectSubTaskSpawner_priv::initialize() {
    SkPixmap pixmap;
    const auto& srcImg = mData->fRenderedImage;
    mSrcRasterImg = srcImg->makeRasterImage();
    mSrcRasterImg->peekPixels(&pixmap);
    mSrcBitmap.installPixels(pixmap);
    if(mUseDst) mDstBitmap.allocPixels(mSrcBitmap.info());
    spawn();
}

void EffectSubTaskSpawner_priv::splitSpawn(CpuRenderData& data,
                                           const SkIRect& rect,
                                           const int nSplits) {
    if(nSplits == 0) return;
    if(nSplits == 1) {
        data.fTexTile = rect;
        const auto decRemaining = [this]() { decRemaining_k(); };
        const auto subTask = enve::make_shared<eCustomCpuTask>(nullptr,
            [this, data]() {
                SkBitmap dstBitmap;
                if(mUseDst) {
                    mDstBitmap.extractSubset(&dstBitmap, data.fTexTile);
                } else {
                    mSrcBitmap.extractSubset(&dstBitmap, data.fTexTile);
                }
                CpuRenderTools tools{mSrcBitmap, dstBitmap};
                mEffectCaller->processCpu(tools, data);
            }, decRemaining, decRemaining);
        CpuTaskExecutor::sAddTask(subTask);
        return;
    }

    const int splits1 = nSplits/2;
    const int splits2 = nSplits - splits1;
    const bool splitWidth = rect.width() > rect.height();
    if(splitWidth) {
        const int width1 = rect.width()*splits1/nSplits;
        const auto rect1 = SkIRect::MakeXYWH(rect.x(), rect.y(),
                                             width1, rect.height());
        splitSpawn(data, rect1, splits1);

        //const int width2 = rect.width() - width1;
        const auto rect2 = SkIRect::MakeLTRB(rect1.right(), rect.top(),
                                             rect.right(), rect.bottom());
        splitSpawn(data, rect2, splits2);
    } else {
        const int height1 = rect.height()*splits1/nSplits;
        const auto rect1 = SkIRect::MakeXYWH(rect.x(), rect.y(),
                                             rect.width(), height1);
        splitSpawn(data, rect1, splits1);

        //const int height2 = rect.height() - height1;
        const auto rect2 = SkIRect::MakeLTRB(rect.left(), rect1.bottom(),
                                             rect.right(), rect.bottom());
        splitSpawn(data, rect2, splits2);
    }
}

void EffectSubTaskSpawner_priv::spawn() {
    const int width = mSrcBitmap.width();
    const int height = mSrcBitmap.height();
    const int area = width*height;
    const int nAllThreads = QThread::idealThreadCount();
    const int nThreads = qMax(1, mEffectCaller->cpuThreads(nAllThreads, area));
    mRemaining = nThreads;

    auto& srcImage = mData->fRenderedImage;
    const int srcWidth = srcImage->width();
    const int srcHeight = srcImage->height();
    CpuRenderData data;
    data.fPos = mData->fGlobalRect.topLeft();
    data.fWidth = static_cast<uint>(srcWidth);
    data.fHeight = static_cast<uint>(srcHeight);

    splitSpawn(data, srcImage->bounds(), nThreads);
}

void EffectSubTaskSpawner_priv::decRemaining_k() {
    if(--mRemaining > 0) return;
    if(mData->getState() != eTaskState::canceled) {
        if(mUseDst) {
            mData->fRenderedImage = SkiaHelpers::transferDataToSkImage(
                                        mDstBitmap);
        } else {
            mData->fRenderedImage = mSrcRasterImg;
        }
        if(mData->nextStep()) {
            mData->queTask();
        } else {
            mData->finishedProcessing();
        }
    }
    delete this;
}


void EffectSubTaskSpawner::sSpawn(const stdsptr<RasterEffectCaller> &effect,
                                  const stdsptr<BoxRenderData> &data) {
    const auto spawner = new EffectSubTaskSpawner_priv(effect, data);
    spawner->initialize();
}
