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

#include "audiostreamsdata.h"
#include "Sound/soundcomposition.h"

AudioStreamsData::AudioStreamsData() {
    connect(eSoundSettings::sInstance, &eSoundSettings::settingsChanged,
            this, [this]() {
        updateSwrContext();
    });
}

AudioStreamsData::~AudioStreamsData() {
    if(fOpened) close();
}

stdsptr<const AudioStreamsData> AudioStreamsData::sOpen(
        const QString &path, AVFormatContext * const formatContext) {
    const auto result = std::shared_ptr<AudioStreamsData>(
                new AudioStreamsData, AudioStreamsData::sDestroy);
    result->open(path, formatContext);
    return result;
}

void AudioStreamsData::updateSwrContext()
{
    if (mLocked) {
        mUpdateSwrPlanned = true;
        return;
    }
    mUpdateSwrPlanned = false;

    const auto audCodecPars = fAudioStream->codecpar;

    AVChannelLayout out_layout;
    uint64_t out_mask = eSoundSettings::sChannelLayout();
    if (out_mask == 0) out_mask = AV_CH_LAYOUT_STEREO;
    av_channel_layout_from_mask(&out_layout, out_mask);

    if (fSwrContext) { swr_free(&fSwrContext); }

    int ret = swr_alloc_set_opts2(&fSwrContext,
                                  &out_layout,
                                  eSoundSettings::sSampleFormat(),
                                  eSoundSettings::sSampleRate(),
                                  &audCodecPars->ch_layout,
                                  static_cast<AVSampleFormat>(audCodecPars->format),
                                  audCodecPars->sample_rate,
                                  0, nullptr);

    av_channel_layout_uninit(&out_layout);

    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        RuntimeThrow(QString("Resampler allocation failed: %1").arg(errbuf).toStdString().c_str());
    }

    ret = swr_init(fSwrContext);
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        RuntimeThrow(QString("Resampler init failed: %1").arg(errbuf).toStdString().c_str());
    }
}

stdsptr<AudioStreamsData> AudioStreamsData::sOpen(const QString &path) {
    const auto result = std::shared_ptr<AudioStreamsData>(
                new AudioStreamsData, AudioStreamsData::sDestroy);
    result->open(path);
    return result;
}

void AudioStreamsData::open(const QString &path,
                            AVFormatContext * const formatContext) {
    try {
        fPath = path;
        open(formatContext);
    } catch(...) {
        fPath.clear();
        RuntimeThrow("Failed to set audio file path to '" + path + "'.");
    }
}

void AudioStreamsData::open(const QString &path) {
    try {
        fPath = path;
        open();
    } catch(...) {
        fPath.clear();
        RuntimeThrow("Failed to set audio file path to '" + path + "'.");
    }
}

void AudioStreamsData::close() {
    fOpened = false;

    if(fDecodedFrame) av_frame_free(&fDecodedFrame);
    if(fPacket) av_packet_free(&fPacket);
    if(fSwrContext) swr_free(&fSwrContext);
    if(fCodecContext) {
        avcodec_close(fCodecContext);
        avcodec_free_context(&fCodecContext);
    }
    if(fFormatContext) avformat_close_input(&fFormatContext);

    fAudioStreamIndex = -1;
    fAudioStream = nullptr;
}

void AudioStreamsData::open() {
    if(fOpened) return;
    const auto stdString = fPath.toStdString();
    const char * const path = stdString.c_str();
    try {
        open(path);
    } catch(...) {
        close();
        RuntimeThrow("Failed to setup audio stream for '" + path + "'.");
    }
}

void AudioStreamsData::open(const char * const path) {
    auto formatContext = avformat_alloc_context();
    if(!formatContext) RuntimeThrow("Error allocating AVFormatContext");
    if(avformat_open_input(&formatContext, path, nullptr, nullptr) != 0) {
        RuntimeThrow("Could not open file");
    }
    if(avformat_find_stream_info(formatContext, nullptr) < 0) {
        RuntimeThrow("Could not retrieve stream info");
    }
    open(formatContext);
}

void AudioStreamsData::open(AVFormatContext * const formatContext) {
    fFormatContext = formatContext;
    // Find the index of the first audio stream
    fAudioStreamIndex = -1;
    const AVCodecParameters *audCodecPars = nullptr;
    const AVCodec *audCodec = nullptr;
    fAudioStream = nullptr;
    for(uint i = 0; i < fFormatContext->nb_streams; i++) {
        AVStream * const  iStream = fFormatContext->streams[i];
        const AVCodecParameters * const iCodecPars = iStream->codecpar;
        const AVMediaType &iMediaType = iCodecPars->codec_type;
        if(iMediaType == AVMEDIA_TYPE_AUDIO) {
            fAudioStreamIndex = static_cast<int>(i);
            audCodecPars = iCodecPars;
            audCodec = avcodec_find_decoder(audCodecPars->codec_id);
            fAudioStream = iStream;
            if(fAudioStream->duration == 0)
                RuntimeThrow("Unknown stream duration");
            const qreal mult = qreal(fAudioStream->time_base.num)/
                                     fAudioStream->time_base.den;
            fDurationSec = fAudioStream->duration*mult;
            break;
        }
    }
    if(fAudioStreamIndex == -1)
        RuntimeThrow("Could not retrieve audio stream");
    if(!audCodec) RuntimeThrow("Unsupported codec");

    fCodecContext = avcodec_alloc_context3(audCodec);
    if(!fCodecContext) RuntimeThrow("Error allocating AVCodecContext");
    if(avcodec_parameters_to_context(fCodecContext, audCodecPars) < 0) {
        RuntimeThrow("Failed to copy codec params to codec context");
    }

    if(avcodec_open2(fCodecContext, audCodec, nullptr) < 0) {
        RuntimeThrow("Failed to open codec");
    }

    updateSwrContext();

    fPacket = av_packet_alloc();
    if(!fPacket) RuntimeThrow("Error allocating AVPacket");
    fDecodedFrame = av_frame_alloc();
    if(!fDecodedFrame) RuntimeThrow("Error allocating AVFrame");

    fOpened = true;
}
