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

#ifndef VIDEOCACHEHANDLER_H
#define VIDEOCACHEHANDLER_H

#include "animationcachehandler.h"
#include "videostreamsdata.h"
#include "filecachehandler.h"
#include "CacheHandlers/hddcachablecachehandler.h"

#include <set>

class VideoFrameLoader;
class VideoFrameHandler;

class CORE_EXPORT VideoDataHandler : public FileDataCacheHandler {
    Q_OBJECT
public:
    VideoDataHandler() {}

    void clearCache();
    void afterSourceChanged();

    const HddCachableCacheHandler& getCacheHandler() const;

    void addFrameLoader(const int frameId, const stdsptr<VideoFrameLoader>& loader);
    VideoFrameLoader * getFrameLoader(const int frame) const;
    void removeFrameLoader(const int frame);
    void frameLoaderFinished(const int frame, const sk_sp<SkImage>& image);
    eTask* scheduleFrameHddCacheLoad(const int frame);
    ImageCacheContainer* getFrameAtFrame(const int relFrame) const;
    ImageCacheContainer* getFrameAtOrBeforeFrame(const int relFrame) const;
    int getFrameCount() const;
    void setFrameCount(const int count);
    qreal getFps() const;
    void setFps(const qreal fps);
    const QSize getDim();
    void setDim(const QSize dim);
signals:
    void frameCountUpdated(int);
private:
    int mFrameCount = 0;
    qreal mFrameFps = 0;
    int mFrameWidth = 0;
    int mFrameHeight = 0;
public:
    QList<VideoFrameHandler*> mFrameHandlers;
    QList<int> mFramesBeingLoaded;
    QList<stdsptr<VideoFrameLoader>> mFrameLoaders;
    HddCachableCacheHandler mFramesCache;
};

class CORE_EXPORT VideoFrameHandler : public AnimationFrameHandler {
    e_OBJECT
    friend class VideoFrameLoader;
protected:
    VideoFrameHandler(VideoDataHandler* const cacheHandler);
public:
    ImageCacheContainer* getFrameAtFrame(const int relFrame);
    ImageCacheContainer* getFrameAtOrBeforeFrame(const int relFrame);
    eTask *scheduleFrameLoad(const int frame);
    int getFrameCount() const;
    void reload();

    void afterSourceChanged();

    void frameLoaderFinished(const int frame, const sk_sp<SkImage>& image);
    void frameLoaderCanceled(const int frameId);
    void frameLoaderFailed(const int frameId);

    VideoDataHandler* getDataHandler() const;
    const HddCachableCacheHandler& getCacheHandler() const;
protected:
    VideoFrameLoader * getFrameLoader(const int frame);
    VideoFrameLoader * addFrameLoader(const int frameId);
    VideoFrameLoader * addFrameConverter(const int frameId, AVFrame * const frame);
    void removeFrameLoader(const int frame);

    void openVideoStream();
private:
    std::set<int> mNeededFrames;

    VideoDataHandler* const mDataHandler;
public:
    stdsptr<VideoStreamsData> mVideoStreamsData;
};
#include "CacheHandlers/soundcachehandler.h"
class CORE_EXPORT VideoFileHandler : public FileCacheHandler {
    e_OBJECT
protected:
    VideoFileHandler() {}

    void reload();
public:
    void replace();

    VideoDataHandler* getFrameHandler() const {
        return mDataHandler.get();
    }

    SoundDataHandler* getSoundHandler() const {
        return mSoundHandler.get();
    }
private:
    qsptr<VideoDataHandler> mDataHandler;
    qsptr<SoundDataHandler> mSoundHandler;
};

#endif // VIDEOCACHEHANDLER_H
