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

#include "soundmerger.h"
#include "utils/ffmpeghelper.h"

template <typename T>
void mergePlanarDataUnsigned(T const * const * const src,
                             const SampleRange& srcRange,
                             T ** const dst,
                             const SampleRange& dstRange,
                             const int nSamples,
                             QrealSnapshot::Iterator volIt,
                             const int nChannels) {
    int dstId = dstRange.fMin;
    int srcId = srcRange.fMin;
    const qreal min = std::numeric_limits<T>::min();
    const qreal max = std::numeric_limits<T>::max();
    const qreal shift = max/2;
    if(volIt.staticValue()) {
        const qreal vol = volIt.getValueAndProgress(1);
        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nChannels; j++) {
                auto& dstP = dst[j][dstId];
                dstP = T(qBound(min, round(dstP + (src[j][srcId] - shift)*vol + shift), max));
            }
            dstId++; srcId++;
        }
    } else {
        for(int i = 0; i < nSamples; i++) {
            const qreal vol = volIt.getValueAndProgress(1);
            for(int j = 0; j < nChannels; j++) {
                auto& dstP = dst[j][dstId];
                dstP = T(qBound(min, round(dstP + (src[j][srcId] - shift)*vol + shift), max));
            }
            dstId++; srcId++;
        }
    }
}

template <typename T>
void mergeInterleavedDataUnsigned(const T* const src,
                                  const SampleRange& srcRange,
                                  T * const dst,
                                  const SampleRange& dstRange,
                                  const int nSamples,
                                  QrealSnapshot::Iterator volIt,
                                  const int nChannels) {
    int dstId = dstRange.fMin*nChannels;
    int srcId = srcRange.fMin*nChannels;
    const qreal min = std::numeric_limits<T>::min();
    const qreal max = std::numeric_limits<T>::max();
    const qreal shift = max/2;
    if(volIt.staticValue()) {
        const qreal vol = volIt.getValueAndProgress(1);
        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nChannels; j++) {
                auto& dstP = dst[dstId++];
                dstP = T(qBound(min, round(dstP + (src[srcId++] - shift)*vol + shift), max));
            }
        }
    } else {
        for(int i = 0; i < nSamples; i++) {
            const qreal vol = volIt.getValueAndProgress(1);
            for(int j = 0; j < nChannels; j++) {
                auto& dstP = dst[dstId++];
                dstP = T(qBound(min, round(dstP + (src[srcId++] - shift)*vol + shift), max));
            }
        }
    }
}

template <typename T>
void mergePlanarDataSigned(T const * const * const src,
                           const SampleRange& srcRange,
                           T ** const dst,
                           const SampleRange& dstRange,
                           const int nSamples,
                           QrealSnapshot::Iterator volIt,
                           const int nChannels) {
    int dstId = dstRange.fMin;
    int srcId = srcRange.fMin;
    const qreal min = (qreal)std::numeric_limits<T>::min();
    const qreal max = (qreal)std::numeric_limits<T>::max();
    if(volIt.staticValue()) {
        const qreal vol = volIt.getValueAndProgress(1);
        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nChannels; j++) {
                dst[j][dstId] = T(qBound(min, round(dst[j][dstId] + src[j][srcId]*vol), max));
            }
            dstId++; srcId++;
        }
    } else {
        for(int i = 0; i < nSamples; i++) {
            const qreal vol = volIt.getValueAndProgress(1);
            for(int j = 0; j < nChannels; j++) {
                dst[j][dstId] = T(qBound(min, round(dst[j][dstId] + src[j][srcId]*vol), max));
            }
            dstId++; srcId++;
        }
    }
}

template <typename T>
void mergeInterleavedDataSigned(const T* const src,
                                const SampleRange& srcRange,
                                T * const dst,
                                const SampleRange& dstRange,
                                const int nSamples,
                                QrealSnapshot::Iterator volIt,
                                const int nChannels) {
    int dstId = dstRange.fMin*nChannels;
    int srcId = srcRange.fMin*nChannels;
    const qreal min = (qreal)std::numeric_limits<T>::min();
    const qreal max = (qreal)std::numeric_limits<T>::max();
    if(volIt.staticValue()) {
        const qreal vol = volIt.getValueAndProgress(1);
        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nChannels; j++) {
                auto& dstP = dst[dstId++];
                dstP = T(qBound(min, round(dstP + src[srcId++]*vol), max));
            }
        }
    } else {
        for(int i = 0; i < nSamples; i++) {
            const qreal vol = volIt.getValueAndProgress(1);
            for(int j = 0; j < nChannels; j++) {
                auto& dstP = dst[dstId++];
                dstP = T(qBound(min, round(dstP + src[srcId++]*vol), max));
            }
        }
    }
}

void mergePlanarData(float const * const * const src,
                     const SampleRange& srcRange,
                     float ** const dst,
                     const SampleRange& dstRange,
                     const int nSamples,
                     QrealSnapshot::Iterator volIt,
                     const int nChannels) {
    int dstId = dstRange.fMin;
    int srcId = srcRange.fMin;
    if(volIt.staticValue()) {
        const float vol = static_cast<float>(volIt.getValueAndProgress(1));
        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nChannels; j++) {
                dst[j][dstId] += src[j][srcId]*vol;
            }
            dstId++; srcId++;
        }
    } else {
        for(int i = 0; i < nSamples; i++) {
            const float vol = static_cast<float>(volIt.getValueAndProgress(1));
            for(int j = 0; j < nChannels; j++) {
                dst[j][dstId] += src[j][srcId]*vol;
            }
            dstId++; srcId++;
        }
    }
}

void mergeInterleavedData(const float* const src,
                          const SampleRange& srcRange,
                          float * const dst,
                          const SampleRange& dstRange,
                          const int nSamples,
                          QrealSnapshot::Iterator volIt,
                          const int nChannels) {
    int dstId = dstRange.fMin*nChannels;
    int srcId = srcRange.fMin*nChannels;
    if(volIt.staticValue()) {
        const float vol = static_cast<float>(volIt.getValueAndProgress(1));
        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nChannels; j++) {
                dst[dstId++] += src[srcId++]*vol;
            }
        }
    } else {
        for(int i = 0; i < nSamples; i++) {
            const float vol = static_cast<float>(volIt.getValueAndProgress(1));
            for(int j = 0; j < nChannels; j++) {
                dst[dstId++] += src[srcId++]*vol;
            }
        }
    }
}


void mergePlanarData(qreal const * const * const src,
                     const SampleRange& srcRange,
                     qreal ** const dst,
                     const SampleRange& dstRange,
                     const int nSamples,
                     QrealSnapshot::Iterator volIt,
                     const int nChannels) {
    int dstId = dstRange.fMin;
    int srcId = srcRange.fMin;
    if(volIt.staticValue()) {
        const qreal vol = volIt.getValueAndProgress(1);
        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nChannels; j++) {
                dst[j][dstId] += src[j][srcId]*vol;
            }
            dstId++; srcId++;
        }
    } else {
        for(int i = 0; i < nSamples; i++) {
            const qreal vol = volIt.getValueAndProgress(1);
            for(int j = 0; j < nChannels; j++) {
                dst[j][dstId] += src[j][srcId]*vol;
            }
            dstId++; srcId++;
        }
    }
}

void mergeInterleavedData(const qreal* const src,
                          const SampleRange& srcRange,
                          qreal * const dst,
                          const SampleRange& dstRange,
                          const int nSamples,
                          QrealSnapshot::Iterator volIt,
                          const int nChannels) {
    int dstId = dstRange.fMin*nChannels;
    int srcId = srcRange.fMin*nChannels;
    if(volIt.staticValue()) {
        const qreal vol = volIt.getValueAndProgress(1);
        for(int i = 0; i < nSamples; i++) {
            for(int j = 0; j < nChannels; j++) {
                dst[dstId++] += src[srcId++]*vol;
            }
        }
    } else {
        for(int i = 0; i < nSamples; i++) {
            const qreal vol = volIt.getValueAndProgress(1);
            for(int j = 0; j < nChannels; j++) {
                dst[dstId++] += src[srcId++]*vol;
            }
        }
    }
}

void mergeData(uchar const * const * const src,
               const SampleRange& srcRange,
               uchar ** const dst,
               const SampleRange& dstRange,
               int nSamples,
               QrealSnapshot::Iterator volIt,
               const AVSampleFormat format,
               const int nChannels) {
    nSamples = qMin(qMin(nSamples, dstRange.span()), srcRange.span());
    if(format == AV_SAMPLE_FMT_FLT) {
        mergeInterleavedData(reinterpret_cast<const float*>(src[0]), srcRange,
                             reinterpret_cast<float*>(dst[0]), dstRange,
                             nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_FLTP) {
        mergePlanarData(reinterpret_cast<float const * const *>(src), srcRange,
                        reinterpret_cast<float**>(dst), dstRange,
                        nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_DBL) {
        mergeInterleavedData(reinterpret_cast<const qreal*>(src[0]), srcRange,
                             reinterpret_cast<qreal*>(dst[0]), dstRange,
                             nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_DBLP) {
        mergePlanarData(reinterpret_cast<qreal const * const *>(src), srcRange,
                        reinterpret_cast<qreal**>(dst), dstRange,
                        nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_U8) {
        mergeInterleavedDataUnsigned(reinterpret_cast<const quint8*>(src[0]), srcRange,
                                     reinterpret_cast<quint8*>(dst[0]), dstRange,
                                     nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_U8P) {
        mergePlanarDataUnsigned(reinterpret_cast<quint8 const * const *>(src), srcRange,
                                reinterpret_cast<quint8**>(dst), dstRange,
                                nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_S16) {
        mergeInterleavedDataSigned(reinterpret_cast<const qint16*>(src[0]), srcRange,
                                   reinterpret_cast<qint16*>(dst[0]), dstRange,
                                   nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_S16P) {
        mergePlanarDataSigned(reinterpret_cast<qint16 const * const *>(src), srcRange,
                              reinterpret_cast<qint16**>(dst), dstRange,
                              nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_S32) {
        mergeInterleavedDataSigned(reinterpret_cast<const qint32*>(src[0]), srcRange,
                                   reinterpret_cast<qint32*>(dst[0]), dstRange,
                                   nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_S32P) {
        mergePlanarDataSigned(reinterpret_cast<qint32 const * const *>(src), srcRange,
                              reinterpret_cast<qint32**>(dst), dstRange,
                              nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_S64) {
        mergeInterleavedDataSigned(reinterpret_cast<const qint64*>(src[0]), srcRange,
                                   reinterpret_cast<qint64*>(dst[0]), dstRange,
                                   nSamples, volIt, nChannels);
    } else if(format == AV_SAMPLE_FMT_S64P) {
        mergePlanarDataSigned(reinterpret_cast<qint64 const * const *>(src), srcRange,
                              reinterpret_cast<qint64**>(dst), dstRange,
                              nSamples, volIt, nChannels);
    } else RuntimeThrow("Unsupported format " + av_get_sample_fmt_name(format));
}

void SoundMerger::process() {
    const int nChannels = mSettings.channelCount();
    mSamples = enve::make_shared<Samples>(mSampleRange,
                                          mSettings.fSampleRate,
                                          mSettings.fSampleFormat,
                                          mSettings.fChannelLayout);
    mSamples->zeroAll();
    const auto dst = mSamples->fData;
    for(const auto& sound : mSounds) {
        const auto srcSamples = sound.fSamples;
        const qreal stretch = sound.fStretch;

        const SampleRange smplsRelRange = srcSamples->fSampleRange;
        const SampleRange smplsSpeedRelRange{qRound(smplsRelRange.fMin*stretch),
                                             qRound(smplsRelRange.fMax*stretch)};
        const SampleRange smplsAbsRange = smplsSpeedRelRange.shifted(sound.fSampleShift);
        const SampleRange srcAbsRange = smplsAbsRange*sound.fSSAbsRange;
        const SampleRange srcNeededAbsRange = srcAbsRange*mSampleRange;
        const int absToRel = -qRound(srcSamples->fSampleRange.fMin*stretch) - sound.fSampleShift;
        const SampleRange srcNeededRelRange = srcNeededAbsRange.shifted(absToRel);

        const SampleRange dstAbsRange = mSampleRange;
        const SampleRange dstNeededAbsRange = dstAbsRange*srcNeededAbsRange;
        const SampleRange dstRelRange = dstNeededAbsRange.shifted(-mSampleRange.fMin);

        if(!dstRelRange.isValid()) continue;
        if(!srcNeededRelRange.isValid()) continue;
        const int firstVolSample = dstNeededAbsRange.fMin - sound.fSampleShift;
        QrealSnapshot::Iterator volIt(firstVolSample, 1000, &sound.fVolume);
        if(isOne4Dec(stretch)) {
            const int nSamples = qMin(srcNeededRelRange.span(), dstRelRange.span());

            const auto src = srcSamples->fData;
            mergeData(src, srcNeededRelRange, dst, dstRelRange,
                      nSamples, volIt, mSettings.fSampleFormat, nChannels);
        } else {
            const int srcSampleRate = mSettings.fSampleRate;
            const int dstSampleRate = qRound(mSettings.fSampleRate*stretch);

            const AVSampleFormat sampleFormat = mSettings.fSampleFormat;
            const uint64_t chLayout = mSettings.fChannelLayout;
            const int chCount = Friction::Utils::FFmpegHelper::getNbChannels(chLayout);

            struct SwrContext * swrContext = swr_alloc();
            av_opt_set_int(swrContext, "in_channel_count", chCount, 0);
            av_opt_set_int(swrContext, "out_channel_count", chCount, 0);
            av_opt_set_int(swrContext, "in_channel_layout", chLayout, 0);
            av_opt_set_int(swrContext, "out_channel_layout", chLayout, 0);
            av_opt_set_int(swrContext, "in_sample_rate", srcSampleRate, 0);
            av_opt_set_int(swrContext, "out_sample_rate", dstSampleRate, 0);
            av_opt_set_sample_fmt(swrContext, "in_sample_fmt", sampleFormat, 0);
            av_opt_set_sample_fmt(swrContext, "out_sample_fmt", sampleFormat,  0);
            swr_init(swrContext);
            if(!swr_is_initialized(swrContext))
                RuntimeThrow("Resampler has not been properly initialized");
            uint8_t** buffer = nullptr;
            const int res = av_samples_alloc_array_and_samples(
                        &buffer, nullptr, chCount, dstSampleRate,
                        sampleFormat, 0);
            if(res < 0) RuntimeThrow("Resampling output buffer alloc failed");
            const int nSamples =
                    swr_convert(swrContext, buffer, dstSampleRate,
                                const_cast<const uint8_t**>(srcSamples->fData),
                                srcSampleRate);
            swr_free(&swrContext);
            if(nSamples < 0) RuntimeThrow("Resampling failed");
            mergeData(buffer, srcNeededRelRange, dst, dstRelRange,
                      nSamples, volIt, mSettings.fSampleFormat, nChannels);
            if(buffer) av_freep(&buffer[0]);
            av_freep(&buffer);
        }
    }
}
