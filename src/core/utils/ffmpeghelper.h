/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#ifndef FRICTION_FFMPEG_HELPER_H
#define FRICTION_FFMPEG_HELPER_H

#include "core_global.h"
#include "exceptions.h"

#include "include/core/SkImage.h"

#include <QString>
#include <QList>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/channel_layout.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#define AV_RuntimeThrow(errId, message) \
{ \
    char * const errMsg = new char[AV_ERROR_MAX_STRING_SIZE]; \
    av_make_error_string(errMsg, AV_ERROR_MAX_STRING_SIZE, errId); \
    try { \
            RuntimeThrow(errMsg); \
    } catch(...) { \
            delete[] errMsg; \
            RuntimeThrow(message); \
    } \
}

namespace Friction
{
    namespace Utils
    {
        class CORE_EXPORT FFmpegHelper
        {
        public:
            struct ChannelLayoutInfo
            {
                uint64_t mask;
                QString name;
            };

            typedef struct OutputStream
            {
                int64_t fNextPts;
                AVStream *fStream = nullptr;
                AVCodecContext *fCodec = nullptr;
                AVFrame *fDstFrame = nullptr;
                AVFrame *fSrcFrame = nullptr;
                struct SwsContext *fSwsCtx = nullptr;
                struct SwrContext *fSwrCtx = nullptr;
                int64_t fFrameDuration = 0;
            } OutputStream;

            static int setCodecLayout(AVCodecContext *c,
                                      uint64_t mask);
            static int setFrameLayout(AVFrame *frame,
                                      uint64_t mask);
            static bool compareLayout(const AVChannelLayout &frameLayout,
                                      uint64_t mask);

            static int getNbChannels(uint64_t layoutMask);

            static uint64_t getLayoutMask(const AVChannelLayout* layout);
            static QList<ChannelLayoutInfo> getSupportedLayouts(const AVCodec* codec);

            static bool isValidProfile(const AVCodec *codec,
                                       const int profile);

            static void openVideo(AVFormatContext * const oc,
                                  const AVCodec * const codec,
                                  OutputStream * const ost);
            static void openAudio(const AVCodec * const codec,
                                  OutputStream * const ost,
                                  const AVSampleFormat sampleFormat,
                                  const uint64_t channelLayout,
                                  const int sampleRate);

            static AVFrame* allocAudioFrame(enum AVSampleFormat sample_fmt,
                                            const uint64_t& channel_layout,
                                            const int sample_rate,
                                            const int nb_samples);
            static void encodeAudioFrame(AVFormatContext * const oc,
                                         FFmpegHelper::OutputStream * const ost,
                                         AVFrame * const frame,
                                         bool * const encodeAudio);

            static AVFrame* allocPicture(enum AVPixelFormat pix_fmt,
                                         const int width,
                                         const int height);
            static AVFrame* getVideoFrame(OutputStream * const ost,
                                          const sk_sp<SkImage> &image);
            static void writeVideoFrame(AVFormatContext * const oc,
                                        FFmpegHelper::OutputStream * const ost,
                                        const sk_sp<SkImage> &image,
                                        bool * const encodeVideo);

            static void flushStream(FFmpegHelper::OutputStream * const ost,
                                    AVFormatContext * const formatCtx);
            static void closeStream(FFmpegHelper::OutputStream * const ost);
        };
    }
}

#endif // FRICTION_FFMPEG_HELPER_H
