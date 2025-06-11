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

#ifndef IMAGECACHECONTAINER_H
#define IMAGECACHECONTAINER_H
#include "skia/skiaincludes.h"
#include "skia/skiahelpers.h"
#include "hddcachablerangecont.h"
#include "imagedatahandler.h"
class Scene;

class CORE_EXPORT ImageCacheContainer : public HddCachableRangeCont,
                            public ImageDataHandler {
    e_OBJECT
protected:
    ImageCacheContainer(const FrameRange& range,
                        HddCachableCacheHandler * const parent);
    ImageCacheContainer(const sk_sp<SkImage>& img,
                        const FrameRange &range,
                        HddCachableCacheHandler * const parent);
    stdsptr<eHddTask> createTmpFileDataSaver();
    stdsptr<eHddTask> createTmpFileDataLoader();
    int clearMemory();
public:
    int getByteCount();

    void setDataLoadedFromTmpFile(const sk_sp<SkImage> &img);
    void replaceImage(const sk_sp<SkImage> &img);
};


#include "CacheHandlers/tmploader.h"
#include "CacheHandlers/tmpsaver.h"

class CORE_EXPORT ImgSaver : public TmpSaver {
    e_OBJECT
public:
    typedef std::function<void(const qsptr<QTemporaryFile>&)> Func;
protected:
    ImgSaver(ImageCacheContainer* const target,
             const sk_sp<SkImage> &image) :
        TmpSaver(target), mImage(image) {}

    void write(eWriteStream& dst) {
        SkiaHelpers::writeImg(mImage, dst);
    }
private:
    const sk_sp<SkImage> mImage;
};

class CORE_EXPORT ImgLoader : public TmpLoader {
    e_OBJECT
public:
    typedef std::function<void(sk_sp<SkImage> img)> Func;

    const sk_sp<SkImage>& image() const { return mImage; }
protected:
    ImgLoader(const qsptr<QTemporaryFile> &file,
              ImageCacheContainer* const target,
              const Func& finishedFunc) :
        TmpLoader(file, target), mFinishedFunc(finishedFunc) {}

    void read(eReadStream& src) {
        mImage = SkiaHelpers::readImg(src);
    }
    void afterProcessing() {
        if(mFinishedFunc) mFinishedFunc(mImage);
    }
private:
    sk_sp<SkImage> mImage;
    const Func mFinishedFunc;
};

#endif // IMAGECACHECONTAINER_H
