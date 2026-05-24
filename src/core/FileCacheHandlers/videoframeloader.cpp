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

#include "videoframeloader.h"
#include "videocachehandler.h"
#include "Private/Tasks/taskscheduler.h"
#include "Private/Tasks/taskexecutor.h"

#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcVideoLoader, "friction.video", QtWarningMsg)

VideoFrameLoader::VideoFrameLoader(VideoFrameHandler * const cacheHandler,
                                   const stdsptr<VideoStreamsData> &openedVideo,
                                   const int frameId) :
    mCacheHandler(cacheHandler), mOpenedVideo(openedVideo),
    mFrameId(frameId) {}

VideoFrameLoader::VideoFrameLoader(VideoFrameHandler * const cacheHandler,
                                   const stdsptr<VideoStreamsData> &openedVideo,
                                   const int frameId, AVFrame * const frame) :
    VideoFrameLoader(cacheHandler, openedVideo, frameId) {
    setFrameToConvert(frame, openedVideo->fCodecContext);
}

VideoFrameLoader::~VideoFrameLoader() {
    for(auto& excess : mExcessFrames) {
        av_frame_unref(excess.second);
        av_frame_free(&excess.second);
    }
    cleanUp();
}

void VideoFrameLoader::convertFrame() {
    const auto info = SkiaHelpers::getPremulRGBAInfo(
                mFrameToConvert->width, mFrameToConvert->height);
    SkBitmap bitmap;
    bitmap.allocPixels(info);

    SkPixmap pixmap;
    bitmap.peekPixels(&pixmap);

    void * const addr = pixmap.writable_addr();
    uint8_t * const dstSk[] = { static_cast<uint8_t*>(addr) };
    int linesizesSk[4];

    av_image_fill_linesizes(linesizesSk, AV_PIX_FMT_RGBA,
                            mFrameToConvert->width);

    sws_scale(mSwsContext, mFrameToConvert->data, mFrameToConvert->linesize,
              0, mFrameToConvert->height, dstSk, linesizesSk);

    mLoadedFrame = SkiaHelpers::transferDataToSkImage(bitmap);

    cleanUp();
}

int frameId(AVFrame * const decodedFrame,
            AVStream * const videoStream,
            const qreal fps) {
    int64_t pts = decodedFrame->best_effort_timestamp;
    pts = av_rescale_q(pts, videoStream->time_base, {1, AV_TIME_BASE});
    const qreal frameApprox = pts/1000000.*fps;
    const int frameRound = qRound(frameApprox);
    if(frameRound - frameApprox > 0.4) return frameRound - 1;
    return frameRound;
}

void seek(const int tryN, const int frameId, const qreal fps,
          AVFormatContext * const formatContext,
          const int videoStreamIndex, AVStream * const videoStream,
          AVCodecContext * const codecContext) {
    const int64_t tsms = qMax(0, qFloor((frameId - tryN) * 1000 / fps));
    const int64_t tm = av_rescale(tsms, videoStream->time_base.den,
                                  videoStream->time_base.num)/1000;
    if(tm <= 0)
        avformat_seek_file(formatContext, videoStreamIndex,
                           INT64_MIN, 0, 0, 0);
    else {
        const int64_t tsms0 = qMax(0, qFloor((frameId - fps - tryN) * 1000 / fps));
        const int64_t tm0 = av_rescale(tsms0, videoStream->time_base.den,
                                       videoStream->time_base.num)/1000;
        if(avformat_seek_file(formatContext, videoStreamIndex, tm0,
                              tm, tm, AVSEEK_FLAG_FRAME) < 0) {
            qCDebug(lcVideoLoader) << "Failed to seek to " << frameId;
            avformat_seek_file(formatContext, videoStreamIndex,
                               INT64_MIN, 0, INT64_MAX, 0);
        }
    }
    avcodec_flush_buffers(codecContext);
}

void VideoFrameLoader::readFrame() {
    if(!mOpenedVideo->fOpened)
        RuntimeThrow("Cannot read frame from closed VideoStream");
    const auto formatContext = mOpenedVideo->fFormatContext;
    const auto videoStreamIndex = mOpenedVideo->fVideoStreamIndex;
    const auto videoStream = mOpenedVideo->fVideoStream;
    const auto packet = mOpenedVideo->fPacket;
    const auto codecContext = mOpenedVideo->fCodecContext;
    auto& decodedFrame = mOpenedVideo->fDecodedFrame;
    //const auto swsContext = mOpenedVideo->fSwsContext;
    const qreal fps = mOpenedVideo->fFps;

    int seekTry = 0;
    if(mOpenedVideo->fLastFrame >= mFrameId ||
       mFrameId - mOpenedVideo->fLastFrame > fps) {
        seek(seekTry++, mFrameId, fps, formatContext,
             videoStreamIndex, videoStream, codecContext);
    }

    while(true) {
        const int lastFrameTmp = mOpenedVideo->fLastFrame;
        mOpenedVideo->fLastFrame = -qFloor(10*fps); // Just in case error occurs
        const int readRet = av_read_frame(formatContext, packet);
        if(readRet < 0) NoBreakRuntimeThrow("Error retrieving AVPacket");
        if(packet->stream_index == videoStreamIndex) {
            const int sendRet = avcodec_send_packet(codecContext, packet);
            if(sendRet < 0) RuntimeThrow("Sending packet to the decoder failed");

            const int recRet = avcodec_receive_frame(codecContext, decodedFrame);
            av_packet_unref(packet);

            if(recRet == AVERROR_EOF)
                break;
            else if(recRet == AVERROR(EAGAIN))
                continue;
            else if(recRet < 0)
                RuntimeThrow("Did not receive frame from the decoder");
        } else {
            av_packet_unref(packet);
            continue;
        }

        const int currFrame = frameId(decodedFrame, videoStream, fps);
        const bool usePrevious = mFrameId > lastFrameTmp &&
                                 currFrame > mFrameId &&
                                 !mExcessFrames.isEmpty();
        mOpenedVideo->fLastFrame = currFrame;
        const bool reseek = currFrame > mFrameId && seekTry <= 3;
        if(usePrevious) {
            int minPositiveDist = INT_MAX;
            int excessId = -1;
            for(int i = 0; i < mExcessFrames.count(); i++) {
                const auto& excess = mExcessFrames.at(i);
                const int dist = mFrameId - excess.first;
                if(dist < 0 || dist > minPositiveDist) continue;
                minPositiveDist = dist;
                excessId = i;
            }
            AVFrame* frame;
            if(excessId == -1) {
                frame = decodedFrame;
                decodedFrame = av_frame_alloc();
            } else {
                frame = mExcessFrames.takeAt(excessId).second;
                av_frame_unref(decodedFrame);
            }
            setFrameToConvert(frame, codecContext);
            break;
        } else if(currFrame == mFrameId || (!reseek && currFrame > mFrameId)) {
            if(currFrame > mFrameId)
                qCDebug(lcVideoLoader) << "frame " + QString::number(currFrame) +
                            " instead of " + QString::number(mFrameId);            
            setFrameToConvert(decodedFrame, codecContext);
            decodedFrame = av_frame_alloc();
            break;
        } else if(qAbs(mFrameId - currFrame) < 20) {
            mExcessFrames.append({currFrame, decodedFrame});
            decodedFrame = av_frame_alloc();
        } else av_frame_unref(decodedFrame);

        if(reseek) seek(seekTry++, mFrameId, fps, formatContext,
                        videoStreamIndex, videoStream, codecContext);
    }
}

void VideoFrameLoader::afterProcessing() {
    if(!mCacheHandler) return;
    mCacheHandler->frameLoaderFinished(mFrameId, mLoadedFrame);
    for(auto& excess : mExcessFrames) {
        if(mCacheHandler->getFrameAtFrame(excess.first)) {
            av_frame_unref(excess.second);
            av_frame_free(&excess.second);
            continue;
        }
        const auto currFL = mCacheHandler->getFrameLoader(excess.first);
        if(currFL) {
            if(currFL->getState() >= eTaskState::processing) {
                av_frame_unref(excess.second);
                av_frame_free(&excess.second);
                continue;
            }
            currFL->setFrameToConvert(excess.second, mOpenedVideo->fCodecContext);
        } else {
            const auto newFL = mCacheHandler->addFrameConverter(
                        excess.first, excess.second);
            newFL->queTask();
        }
    }
    mExcessFrames.clear();
}

void VideoFrameLoader::afterCanceled() {
    if(!mCacheHandler) return;
    mCacheHandler->frameLoaderCanceled(mFrameId);
}

bool VideoFrameLoader::handleException() {
    if(!mCacheHandler) return false;
    takeException();
    if(mFrameId <= 0) {
        finishedProcessing();
        return false;
    }
    const auto moved = mCacheHandler->addFrameLoader(mFrameId - 1);
    moveDependent(moved);
    mCacheHandler->frameLoaderFailed(mFrameId);
    moved->queTask();
    return true;
}

void VideoFrameLoader::queTaskNow() {
    if(mFrameToConvert) {
        TaskScheduler::instance()->queCpuTask(ref<eTask>());
    } else {
        TaskScheduler::instance()->queHddTask(ref<eTask>());
    }
}

void VideoFrameLoader::setFrameToConvert(
        AVFrame * const frame,
        AVCodecContext * const codecContext) {
    cleanUp();
    mFrameToConvert = frame;
    mSwsContext = sws_getContext(codecContext->width,
                                 codecContext->height,
                                 codecContext->pix_fmt,
                                 codecContext->width,
                                 codecContext->height,
                                 AV_PIX_FMT_RGBA, SWS_BICUBIC,
                                 nullptr, nullptr, nullptr);
}

void VideoFrameLoader::process() {
    if(mFrameToConvert) {
        convertFrame();
    } else {
        readFrame();
    }
}

bool VideoFrameLoader::nextStep() {
    if(mFrameToConvert) {
        CpuTaskExecutor::sAddTask(ref<eTask>());
        return true;
    }
    return false;
}

void VideoFrameLoader::cleanUp() {
    if(mFrameToConvert) {
        av_frame_unref(mFrameToConvert);
        av_frame_free(&mFrameToConvert);
        mFrameToConvert = nullptr;
    }
    if(mSwsContext) {
        sws_freeContext(mSwsContext);
        mSwsContext = nullptr;
    }
}
