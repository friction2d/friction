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

#include "videocachehandler.h"
#include "Boxes/boxrendercontainer.h"
#include "Boxes/videobox.h"
#include "CacheHandlers/imagecachecontainer.h"

#include "GUI/edialogs.h"
#include "filesourcescache.h"

#include "videoframeloader.h"

VideoFrameHandler::VideoFrameHandler(VideoDataHandler * const cacheHandler) :
    mDataHandler(cacheHandler) {
    openVideoStream();
}

ImageCacheContainer* VideoFrameHandler::getFrameAtFrame(const int relFrame) {
    return mDataHandler->getFrameAtFrame(relFrame);
}

ImageCacheContainer* VideoFrameHandler::getFrameAtOrBeforeFrame(const int relFrame) {
    return mDataHandler->getFrameAtOrBeforeFrame(relFrame);
}

void VideoFrameHandler::frameLoaderFinished(const int frame,
                                            const sk_sp<SkImage>& image) {
    mDataHandler->frameLoaderFinished(frame, image);
    removeFrameLoader(frame);
}

void VideoFrameHandler::frameLoaderCanceled(const int frameId) {
    removeFrameLoader(frameId);
}

void VideoFrameHandler::frameLoaderFailed(const int frameId) {
    removeFrameLoader(frameId);
    mDataHandler->setFrameCount(frameId);
}

VideoDataHandler *VideoFrameHandler::getDataHandler() const {
    return mDataHandler;
}

const HddCachableCacheHandler &VideoFrameHandler::getCacheHandler() const {
    return mDataHandler->getCacheHandler();
}

VideoFrameLoader *VideoFrameHandler::getFrameLoader(const int frame) {
    return mDataHandler->getFrameLoader(frame);
}

VideoFrameLoader *VideoFrameHandler::addFrameLoader(const int frameId) {
    const auto loader = enve::make_shared<VideoFrameLoader>(
                    this, mVideoStreamsData, frameId);
    mDataHandler->addFrameLoader(frameId, loader);
    for(const auto& nFrame : mNeededFrames) {
        const auto nLoader = getFrameLoader(nFrame);
        if(nFrame < frameId) nLoader->addDependent(loader.get());
        else loader->addDependent(nLoader);
    }
    mNeededFrames.insert(frameId);

    return loader.get();
}

VideoFrameLoader *VideoFrameHandler::addFrameConverter(
        const int frameId,  AVFrame * const frame) {
    const auto loader = enve::make_shared<VideoFrameLoader>(
                    this, mVideoStreamsData, frameId, frame);
    mDataHandler->addFrameLoader(frameId, loader);
    return loader.get();
}

void VideoFrameHandler::removeFrameLoader(const int frame) {
    mDataHandler->removeFrameLoader(frame);
    mNeededFrames.erase(frame);
}

void VideoFrameHandler::openVideoStream()
{
    const auto filePath = mDataHandler->getFilePath();
    mVideoStreamsData = VideoStreamsData::sOpen(filePath);
    mDataHandler->setFrameCount(mVideoStreamsData->fFrameCount);
    mDataHandler->setFps(mVideoStreamsData->fFps);
    mDataHandler->setDim(QSize(mVideoStreamsData->fWidth,
                               mVideoStreamsData->fHeight));
}

eTask* VideoFrameHandler::scheduleFrameLoad(const int frame) {
    if(frame < 0 || frame >= getFrameCount())
        RuntimeThrow("Frame outside of range " + std::to_string(frame));
    const auto currLoader = getFrameLoader(frame);
    if(currLoader) return currLoader;
    if(mDataHandler->getFrameAtFrame(frame)) return nullptr;
    const auto loadTask = mDataHandler->scheduleFrameHddCacheLoad(frame);
    if(loadTask) return loadTask;
    const auto loader = addFrameLoader(frame);
    loader->queTask();
    return loader;
}

int VideoFrameHandler::getFrameCount() const {
    return mDataHandler->getFrameCount();
}

void VideoFrameHandler::reload() {
    mDataHandler->clearCache();
    openVideoStream();
}

void VideoFrameHandler::afterSourceChanged() {
    openVideoStream();
}

void VideoDataHandler::clearCache() {
    mFramesCache.clear();
    mFramesBeingLoaded.clear();
    const auto frameLoaders = mFrameLoaders;
    for(const auto& loader : frameLoaders)
        loader->cancel();
    mFrameLoaders.clear();
}

void VideoFileHandler::replace() {
    const QString importPath = eDialogs::openFile(
                "Replace Video Source " + path(), path(),
                "Video Files (" + FileExtensions::videoFilters() + ")");
    if(!importPath.isEmpty()) {
        const QFile file(importPath);
        if(!file.exists()) return;
        if(hasVideoExt(importPath)) {
            try {
                setPath(importPath);
            } catch(const std::exception& e) {
                gPrintExceptionCritical(e);
            }
        }
    }
}

void VideoDataHandler::afterSourceChanged() {
    for(const auto& handler : qAsConst(mFrameHandlers)) {
        handler->afterSourceChanged();
    }
}

const HddCachableCacheHandler &VideoDataHandler::getCacheHandler() const {
    return mFramesCache;
}

void VideoDataHandler::addFrameLoader(const int frameId,
                                      const stdsptr<VideoFrameLoader> &loader) {
    mFramesBeingLoaded << frameId;
    mFrameLoaders << loader;
}

VideoFrameLoader *VideoDataHandler::getFrameLoader(const int frame) const {
    const int id = mFramesBeingLoaded.indexOf(frame);
    if(id >= 0) return mFrameLoaders.at(id).get();
    return nullptr;
}

void VideoDataHandler::removeFrameLoader(const int frame) {
    const int id = mFramesBeingLoaded.indexOf(frame);
    if(id < 0 || id >= mFramesBeingLoaded.count()) return;
    mFramesBeingLoaded.removeAt(id);
    mFrameLoaders.removeAt(id);
}

void VideoDataHandler::frameLoaderFinished(const int frame,
                                           const sk_sp<SkImage> &image) {
    if(image) {
        mFramesCache.add(enve::make_shared<ImageCacheContainer>(
                             image, FrameRange{frame, frame}, &mFramesCache));
    } else {
        mFrameCount = frame;
        emit frameCountUpdated(mFrameCount);
    }
}

eTask *VideoDataHandler::scheduleFrameHddCacheLoad(const int frame) {
    const auto contAtFrame = mFramesCache.atFrame<ImageCacheContainer>(frame);
    if(contAtFrame) return contAtFrame->scheduleLoadFromTmpFile();
    return nullptr;
}

ImageCacheContainer* VideoDataHandler::getFrameAtFrame(const int relFrame) const {
    return mFramesCache.atFrame<ImageCacheContainer>(relFrame);
}

ImageCacheContainer* VideoDataHandler::getFrameAtOrBeforeFrame(const int relFrame) const {
    return mFramesCache.atOrBeforeFrame<ImageCacheContainer>(relFrame);
}

int VideoDataHandler::getFrameCount() const { return mFrameCount; }

void VideoDataHandler::setFrameCount(const int count) { mFrameCount = count; }

qreal VideoDataHandler::getFps() const
{
    return mFrameFps;
}

void VideoDataHandler::setFps(const qreal fps)
{
    mFrameFps = fps;
}

const QSize VideoDataHandler::getDim()
{
    return QSize(mFrameWidth, mFrameHeight);
}

void VideoDataHandler::setDim(const QSize dim)
{
    mFrameWidth = dim.width();
    mFrameHeight = dim.height();
}

bool hasSound(const char* path) {
    // get format from audio file
    auto format = avformat_alloc_context();
    if(avformat_open_input(&format, path, nullptr, nullptr) != 0) {
        avformat_close_input(&format);
        RuntimeThrow("Could not open file " + path);
    }
    if(avformat_find_stream_info(format, nullptr) < 0) {
        avformat_close_input(&format);
        RuntimeThrow("Could not retrieve stream info from file " + path);
    }

    for(uint i = 0; i < format->nb_streams; i++) {
        if(format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            avformat_close_input(&format);
            return true;
        }
    }

    avformat_close_input(&format);

    return false;
}

void VideoFileHandler::reload() {
    if(fileMissing()) {
        mDataHandler.reset();
        mSoundHandler.reset();
        return;
    }
    const QString& path = this->path();
    mDataHandler = VideoDataHandler::sGetCreateDataHandler<VideoDataHandler>(path);
    mDataHandler->reload();
    if(hasSound(path.toUtf8().data())) {
        mSoundHandler = SoundDataHandler::sGetCreateDataHandler<SoundDataHandler>(path);
        mSoundHandler->reload();
    } else {
        return mSoundHandler.reset();
    }
}
