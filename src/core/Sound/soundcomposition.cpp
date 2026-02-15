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

#include "soundcomposition.h"
#include "esound.h"
#include "canvas.h"
#include "CacheHandlers/soundcachecontainer.h"
#include "soundmerger.h"
#include "FileCacheHandlers/soundreaderformerger.h"

SoundComposition::SoundComposition(Canvas * const parent) :
    QIODevice(parent), mParent(parent) {
    connect(eSoundSettings::sInstance, &eSoundSettings::settingsChanged,
            this, [this]() {
        mSettings = eSoundSettings::sData();
        mSecondsCache.clear();
    });
}

void SoundComposition::start(const int startFrame) {
    mPos = qRound(startFrame*mSettings.fSampleRate/mParent->getFps());
    open(QIODevice::ReadOnly);
}

void SoundComposition::stop() {
    close();
    clearUseRange();
}

void SoundComposition::addSound(const qsptr<eSound>& sound) {
    auto& conn = mSounds.addObj(sound);
    conn << connect(sound.get(), &Property::prp_absFrameRangeChanged,
                    this, &SoundComposition::frameRangeChanged);
    frameRangeChanged(sound->prp_absInfluenceRange());
}

void SoundComposition::removeSound(const qsptr<eSound>& sound) {
    mSounds.removeObj(sound);
    frameRangeChanged(sound->prp_absInfluenceRange());
}

void SoundComposition::secondFinished(const int secondId,
                                      const stdsptr<Samples> &samples) {
    mProcessingSeconds.removeOne(secondId);
    if(!samples) return;
    const auto sCont = enve::make_shared<SoundCacheContainer>(
                samples, iValueRange{secondId, secondId}, &mSecondsCache);
    mSecondsCache.add(sCont);
}

void SoundComposition::setMinFrameUseRange(const int frame) {
    const qreal fps = mParent->getFps();
    const int sec = qFloor(frame/fps);
    mSecondsCache.setUseRange({sec, sec});
}

void SoundComposition::setMaxFrameUseRange(const int frame) {
    const qreal fps = mParent->getFps();
    const int sec = qFloor(frame/fps);
    mSecondsCache.setMaxUseRange(sec);
}

void SoundComposition::clearUseRange() {
    mSecondsCache.clearUseRange();
}

void SoundComposition::scheduleFrameRange(const FrameRange &range) {
    const qreal fps = mParent->getFps();
    const int minSec = qFloor((range.fMin + 1)/fps);
    const int maxSec = qFloor((range.fMax + 1)/fps);
    for(int i = minSec; i <= maxSec; i++) scheduleSecond(i);
}

SoundMerger *SoundComposition::scheduleFrame(const int frameId) {
    const qreal fps = mParent->getFps();
    return scheduleSecond(qFloor(frameId/fps));
}

SoundMerger *SoundComposition::scheduleSecond(const int secondId) {
    if(mSounds.isEmpty()) return nullptr;
    if(mProcessingSeconds.contains(secondId)) return nullptr;
    if(mSecondsCache.atFrame(secondId)) return nullptr;
    mProcessingSeconds.append(secondId);
    const int sampleRate = mSettings.fSampleRate;
    const SampleRange sampleRange = {secondId*sampleRate,
                                     (secondId + 1)*sampleRate - 1};
    const qreal fps = mParent->getFps();

    const auto task = enve::make_shared<SoundMerger>(secondId, sampleRange, this);
    for(const auto &sound : qAsConst(mSounds)) {
        if(!sound->isVisible()) continue;
        const auto enabledFrameRange = sound->prp_absInfluenceRange();
        const iValueRange enabledSecRange{qFloor(enabledFrameRange.fMin/fps),
                                          qFloor(enabledFrameRange.fMax/fps)};
        if(!enabledSecRange.inRange(secondId)) continue;
        const auto secs = sound->absSecondToRelSeconds(secondId);
        for(int i = secs.fMin; i <= secs.fMax; i++) {
            const auto samples = sound->getSamplesForSecond(i);
            if(samples) {
                task->addSoundToMerge({sound->getSampleShift(),
                                       sound->absSampleRange(),
                                       sound->getVolumeSnap(),
                                       sound->getStretch(),
                                       enve::make_shared<Samples>(samples)});
            } else {
                const auto reader = sound->getSecondReader(i);
                if(!reader) continue;
                reader->addMerger(task.get());
                reader->addDependent(task.get());
                reader->addSingleSound(sound.get(),
                                       sound->getSampleShift(),
                                       sound->absSampleRange(),
                                       sound->getVolumeSnap(),
                                       sound->getStretch());
            }
        }
    }
    task->queTask();
    return task.get();
}

void SoundComposition::frameRangeChanged(const FrameRange &range) {
    const qreal fps = mParent->getFps();
    secondRangeChanged({qFloor(range.fMin/fps), qCeil(range.fMax/fps)});
}

qint64 SoundComposition::readData(char *data, qint64 maxLen) {
    const int sampleRate = mSettings.fSampleRate;
    const int bytesPerSample = mSettings.bytesPerSample();
    const int nChannels = mSettings.channelCount();
    const int bytesPerSampleFrame = nChannels * bytesPerSample;

    qint64 total = 0;
    const SampleRange readSamples{static_cast<int>(mPos),
                                  static_cast<int>(mPos + maxLen/bytesPerSampleFrame)};
    while(maxLen > total) {
        const int secondId = static_cast<int>(mPos/sampleRate + (mPos >= 0 ? 0 : -1));
        const auto cont = mSecondsCache.atFrame<SoundCacheContainer>(secondId);
        if(!cont) break;
        const auto samples = cont->getSamples();
        const auto contSampleRange = samples->fSampleRange;
        const auto secondData = samples->fData;
        if(!secondData) break;
        const SampleRange samplesToRead = readSamples*contSampleRange;
        const SampleRange contRelRange = samplesToRead.shifted(-contSampleRange.fMin);
        const qint64 nSamples = contRelRange.span();
        const qint64 chunk = qMin(maxLen - total, nSamples*bytesPerSampleFrame);
        const auto src = secondData[0] + contRelRange.fMin*bytesPerSampleFrame;
        memcpy(data + total, src, static_cast<size_t>(chunk));
        mPos += nSamples;
        total += chunk;
    }

    return total;
}

qint64 SoundComposition::writeData(const char *data, qint64 len) {
    Q_UNUSED(data)
    Q_UNUSED(len)

    return 0;
}
