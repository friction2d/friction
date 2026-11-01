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

#include "soundcachehandler.h"
#include "FileCacheHandlers/soundreader.h"
#include "filesourcescache.h"
#include "appsupport.h"

SoundDataHandler::SoundDataHandler() {
    connect(eSoundSettings::sInstance, &eSoundSettings::settingsChanged,
            this, [this]() {
        mSecondsCache.clear();
    });
}

stdsptr<Samples> SoundHandler::getSamplesForSecond(const int secondId) {
    return mDataHandler->getSamplesForSecond(secondId);
}

void SoundHandler::secondReaderFinished(
        const int secondId,
        const stdsptr<Samples>& samples) {
    if(samples) mDataHandler->secondReaderFinished(secondId, samples);
    removeSecondReader(secondId);
}

SoundReaderForMerger *SoundHandler::addSecondReader(const int secondId) {
    const int sampleRate = eSoundSettings::sSampleRate();
    const SampleRange range = {secondId*sampleRate, (secondId + 1)*sampleRate - 1};
    const auto reader = enve::make_shared<SoundReaderForMerger>(
                this, mAudioStreamsData, secondId, range);
    mDataHandler->addSecondReader(secondId, reader);
    reader->queTask();
    return reader.get();
}

void SoundDataHandler::afterSourceChanged() {}

void SoundFileHandler::replace()
{
    const auto importPath = AppSupport::getOpenFile(nullptr,
                                                    tr("Change Source"),
                                                    path(),
                                                    tr("Audio Files (%1)").arg(FileExtensions::soundFilters()));
    if (!importPath.isEmpty()) { setPath(importPath); }
}
