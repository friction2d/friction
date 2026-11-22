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

#include "soundreader.h"
#include "audiostreamsdata.h"
#include "CacheHandlers/soundcachehandler.h"
#include "CacheHandlers/soundcachecontainer.h"
#include "Sound/soundcomposition.h"
#include "utils/ffmpeghelper.h"

void SoundReader::beforeProcessing(const Hardware) {
    mOpenedAudio->lock();
}

void SoundReader::afterProcessing() {
    mOpenedAudio->unlock();
    mCacheHandler->secondReaderFinished(mSecondId, mSamples);
}

void SoundReader::afterCanceled() {
    mCacheHandler->secondReaderCanceled(mSecondId);
}

void seek(const int tryN, const int secondId,
          AVFormatContext * const formatContext,
          const int audioStreamIndex, AVStream * const audioStream,
          AVCodecContext * const codecContext) {
    const int64_t tsms = qFloor((secondId - 1 - tryN) * 1000);
    const int64_t tm = av_rescale(tsms, audioStream->time_base.den,
                                  audioStream->time_base.num)/1000;
    if(tm <= 0)
        avformat_seek_file(formatContext, audioStreamIndex,
                           INT64_MIN, 0, 0, 0);
    else {
        const int64_t tsms0 = qFloor((secondId - 1 - tryN) * 1000);
        const int64_t tm0 = av_rescale(tsms0, audioStream->time_base.den,
                                       audioStream->time_base.num)/1000;
        if(avformat_seek_file(formatContext, audioStreamIndex, tm0,
                              tm, tm, AVSEEK_FLAG_FRAME) < 0) {
            qDebug() << "Failed to seek to " << secondId;
            avformat_seek_file(formatContext, audioStreamIndex,
                               INT64_MIN, 0, INT64_MAX, 0);
        }
    }
    avcodec_flush_buffers(codecContext);
}

void SoundReader::readFrame() {
    if(!mOpenedAudio->fOpened)
        RuntimeThrow("Cannot read frame from closed AudioStream");
    const int dstSampleRate = mSettings.fSampleRate;
    const AVSampleFormat dstSampleFormat = mSettings.fSampleFormat;
    const uint64_t dstChLayout = mSettings.fChannelLayout;
    const uint dstSampleSize = static_cast<uint>(mSettings.bytesPerSample());
    const int dstChCount = Friction::Utils::FFmpegHelper::getNbChannels(dstChLayout);
    const bool dstPlanar = mSettings.planarFormat();

    const auto formatContext = mOpenedAudio->fFormatContext;
    const auto audioStreamIndex = mOpenedAudio->fAudioStreamIndex;
    const auto audioStream = mOpenedAudio->fAudioStream;
    const auto codecPars = audioStream->codecpar;
    const int srcSampleRate = codecPars->sample_rate;
    const qreal dstSamplesPerSrc = dstSampleRate/qreal(srcSampleRate);
    const auto packet = mOpenedAudio->fPacket;
    const auto decodedFrame = mOpenedAudio->fDecodedFrame;
    const auto codecContext = mOpenedAudio->fCodecContext;
    const auto swrContext = mOpenedAudio->fSwrContext;

    const int firstSample = mSecondId*dstSampleRate;

    int seekTry = 0;
    if(mOpenedAudio->fLastDstSample >= firstSample ||
       firstSample - mOpenedAudio->fLastDstSample > dstSampleRate) {
        seek(seekTry++, mSecondId, formatContext,
             audioStreamIndex, audioStream, codecContext);
    }

    bool firstFrame = true;
    int currentDstSample = 0;
    uchar ** audioData = nullptr;
    if(dstPlanar) {
        audioData = new uchar*[static_cast<ulong>(dstChCount)];
        for(int i = 0; i < dstChCount; i++) {
            audioData[i] = nullptr;
        }
    } else {
        audioData = new uchar*[1];
        audioData[0] = nullptr;
    }
    SampleRange audioDataRange{mSampleRange.fMin, mSampleRange.fMin - 1};
    int nSamples = 0;
    while(true) {
        mOpenedAudio->fLastDstSample = -10*dstSampleRate;
        const int readRet = av_read_frame(formatContext, packet);
        if(readRet < 0) break;
        if(packet->stream_index == audioStreamIndex) {
            const int sendRet = avcodec_send_packet(codecContext, packet);
            if(sendRet < 0) RuntimeThrow("Sending packet to the decoder failed");

            const int recRet = avcodec_receive_frame(codecContext, decodedFrame);
            av_packet_unref(packet);

            if(recRet == AVERROR_EOF)
                break;
            else if(recRet == AVERROR(EAGAIN)) {
                continue;
            } else if(recRet < 0)
                RuntimeThrow("Did not receive frame from the decoder");
        } else {
            av_packet_unref(packet);
            continue;
        }

        // calculate PTS:
        if(firstFrame) {
            int64_t pts = decodedFrame->best_effort_timestamp;
            pts = av_rescale_q(pts, audioStream->time_base, {1, AV_TIME_BASE});
            currentDstSample = static_cast<int>(pts*dstSampleRate/1000000);
            if(currentDstSample > firstSample) {
                if(seekTry > 3) {
                    qDebug() << "sample" << QString::number(currentDstSample) +
                                " instead of " + QString::number(firstSample);
                } else {
                    av_frame_unref(decodedFrame);
                    seek(seekTry++, mSecondId, formatContext,
                         audioStreamIndex, audioStream, codecContext);
                    continue;
                }
            }
        }

        if(currentDstSample + decodedFrame->nb_samples >= mSecondId*dstSampleRate) {
            // resample frames
            uchar** buffer = nullptr;
            const int bufferSamples = qCeil(decodedFrame->nb_samples*dstSamplesPerSrc);
            int linesize;
            const int res = av_samples_alloc_array_and_samples(
                        &buffer, &linesize, dstChCount,
                        bufferSamples, dstSampleFormat, 0);
            if(res < 0) RuntimeThrow("Resampling output buffer alloc failed");

            const int nDstSamples =
                    swr_convert(swrContext, buffer, bufferSamples,
                                const_cast<const uint8_t**>(decodedFrame->data),
                                decodedFrame->nb_samples);
            if(nDstSamples < 0) RuntimeThrow("Resampling failed");
            // append resampled frames to data
            const SampleRange frameSampleRange{currentDstSample, currentDstSample + nDstSamples - 1};
            const SampleRange neededSampleRange = mSampleRange*frameSampleRange;
            const int firstRelSample = neededSampleRange.fMin - frameSampleRange.fMin;
            const int nSamplesInRange = neededSampleRange.span();
            if(nSamplesInRange > 0) {
                const int newNSamples = nSamples + nSamplesInRange;
                if(dstPlanar) {
                    const ulong newAudioDataSize = static_cast<ulong>(newNSamples) * dstSampleSize;
                    for(int i = 0; i < dstChCount; i++) {
                        void * const audioDataMem = realloc(audioData[i], newAudioDataSize);
                        audioData[i] = static_cast<uchar*>(audioDataMem);
                        const uint srcDispl = uint(firstRelSample) * dstSampleSize;
                        const auto src = buffer[i] + srcDispl;
                        const uint dstDispl = uint(nSamples) * dstSampleSize;
                        uchar * const dst = audioData[i] + dstDispl;
                        memcpy(dst, src, static_cast<ulong>(nSamplesInRange) * dstSampleSize);
                    }
                } else {
                    const ulong newAudioDataSize = static_cast<ulong>(newNSamples * dstChCount) * dstSampleSize;
                    void * const audioDataMem = realloc(audioData[0], newAudioDataSize);
                    audioData[0] = static_cast<uchar*>(audioDataMem);
                    const uint srcDispl = uint(firstRelSample*dstChCount) * dstSampleSize;
                    const auto src = buffer[0] + srcDispl;
                    const uint dstDispl = uint(nSamples*dstChCount) * dstSampleSize;
                    uchar * const dst = audioData[0] + dstDispl;
                    memcpy(dst, src, static_cast<ulong>(nSamplesInRange * dstChCount) * dstSampleSize);
                }
                nSamples = newNSamples;
                if(firstFrame) audioDataRange = neededSampleRange;
                else audioDataRange += neededSampleRange;
            }

            if(buffer) av_freep(&buffer[0]);
            av_freep(&buffer);
            firstFrame = false;

            currentDstSample += nDstSamples;
            mOpenedAudio->fLastDstSample = currentDstSample - 1;
        }

        if(currentDstSample >= mSampleRange.fMax) break;
        av_frame_unref(decodedFrame);
    }
    av_frame_unref(decodedFrame);
    mSamples = enve::make_shared<Samples>(audioData, audioDataRange,
                                          dstSampleRate,
                                          dstSampleFormat, dstChLayout);
}
