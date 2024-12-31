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

#include "videostreamsdata.h"
#include "Private/esettings.h"

stdsptr<VideoStreamsData> VideoStreamsData::sOpen(const QString &path) {
    const auto result = std::shared_ptr<VideoStreamsData>(
                new VideoStreamsData, VideoStreamsData::sDestroy);
    result->open(path);
    return result;
}


void VideoStreamsData::open(const QString &path) {
    try {
        fPath = path;
        open();
    } catch(...) {
        fPath.clear();
        RuntimeThrow("Failed to set video file path to '" + path + "'.");
    }
}

void VideoStreamsData::close() {
    fOpened = false;

    if(fDecodedFrame) av_frame_free(&fDecodedFrame);
    if(fPacket) av_packet_free(&fPacket);
    if(fSwsContext) sws_freeContext(fSwsContext);
    fSwsContext = nullptr;
    if(fCodecContext) {
        avcodec_close(fCodecContext);
        avcodec_free_context(&fCodecContext);
    }
    if(fFormatContext) avformat_close_input(&fFormatContext);

    fVideoStreamIndex = -1;
    fVideoStream = nullptr;
}

void VideoStreamsData::open() {
    const auto stdString = fPath.toStdString();
    const char * const path = stdString.c_str();
    try {
        open(path);
    } catch(...) {
        close();
        RuntimeThrow("Failed to setup video stream for '" + path + "'.");
    }
}

void VideoStreamsData::open(const char * const path)
{
    fFormatContext = avformat_alloc_context();
    if (!fFormatContext) {
        RuntimeThrow(QObject::tr("Error allocating AVFormatContext"));
    }

    if (avformat_open_input(&fFormatContext,
                            path,
                            nullptr,
                            nullptr) != 0) {
        RuntimeThrow(QObject::tr("Could not open file"));
    }
    if (avformat_find_stream_info(fFormatContext,
                                  nullptr) < 0) {
        RuntimeThrow(QObject::tr("Could not retrieve stream info"));
    }

    // Find the index of the first audio stream
    fVideoStreamIndex = -1;
    const AVCodecParameters *vidCodecPars = nullptr;
    const AVCodec *vidCodec = nullptr;
    fVideoStream = nullptr;
    bool hasAudio = false;
    for (uint i = 0; i < fFormatContext->nb_streams; i++) {
        const AVStream * const  iStream = fFormatContext->streams[i];
        const AVCodecParameters * const iCodecPars = iStream->codecpar;
        const AVMediaType &iMediaType = iCodecPars->codec_type;
        if (iMediaType == AVMEDIA_TYPE_VIDEO) {
            fVideoStreamIndex = static_cast<int>(i);
            vidCodecPars = iCodecPars;
            vidCodec = avcodec_find_decoder(vidCodecPars->codec_id);
            fVideoStream = fFormatContext->streams[fVideoStreamIndex];
            fTimeBaseDen = fVideoStream->avg_frame_rate.den;
            fTimeBaseNum = fVideoStream->avg_frame_rate.num;

            if (fTimeBaseDen == 0) {
                RuntimeThrow(QObject::tr("Invalid video frame rate denominator (0)"));
            }
            fFps = static_cast<qreal>(fTimeBaseNum)/fTimeBaseDen;
            if (fVideoStream->nb_frames > 0) {
                fFrameCount = static_cast<int>(fVideoStream->nb_frames);
            } else {
                const int64_t duration = fFormatContext->duration +
                        (fFormatContext->duration <= INT64_MAX - 5000 ? 5000 : 0);
                fFrameCount = qFloor(duration*fFps/AV_TIME_BASE);
            }
            break;
        } else if (iMediaType == AVMEDIA_TYPE_AUDIO) { hasAudio = true; }
    }
    if (fVideoStreamIndex == -1) {
        RuntimeThrow(QObject::tr("Could not retrieve video stream"));
    }
    if (!vidCodec) {
        RuntimeThrow(QObject::tr("Unsupported codec"));
    }

    fCodecContext = avcodec_alloc_context3(vidCodec);
    if (!fCodecContext) {
        RuntimeThrow(QObject::tr("Error allocating AVCodecContext"));
    }

    int maxThreads = eSettings::sCpuThreadsCapped() - 1;
    if (maxThreads > 0) {
        fCodecContext->thread_count = maxThreads > 16 ? 16 : maxThreads;
    }

    if (avcodec_parameters_to_context(fCodecContext,
                                      vidCodecPars) < 0) {
        RuntimeThrow(QObject::tr("Failed to copy codec params to codec context"));
    }

    if (avcodec_open2(fCodecContext,
                      vidCodec,
                      nullptr) < 0) {
        RuntimeThrow(QObject::tr("Failed to open codec"));
    }
    fWidth = fCodecContext->width;
    fHeight = fCodecContext->height;
    fSwsContext = sws_getContext(fCodecContext->width,
                                 fCodecContext->height,
                                 fCodecContext->pix_fmt,
                                 fCodecContext->width,
                                 fCodecContext->height,
                                 AV_PIX_FMT_RGBA, SWS_BICUBIC,
                                 nullptr, nullptr, nullptr);

    fPacket = av_packet_alloc();
    if (!fPacket) {
        RuntimeThrow(QObject::tr("Error allocating AVPacket"));
    }
    fDecodedFrame = av_frame_alloc();
    if (!fDecodedFrame) {
        RuntimeThrow(QObject::tr("Error allocating AVFrame"));
    }

    fOpened = true;

    if (hasAudio) { fAudioData = AudioStreamsData::sOpen(fPath); }
}
