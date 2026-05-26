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

#ifndef SOUNDCACHEHANDLER_H
#define SOUNDCACHEHANDLER_H
#include "FileCacheHandlers/filecachehandler.h"
#include "CacheHandlers/hddcachablecachehandler.h"
#include "CacheHandlers/soundcachecontainer.h"
#include "FileCacheHandlers/audiostreamsdata.h"
#include "FileCacheHandlers/soundreaderformerger.h"

class CORE_EXPORT SoundDataHandler : public FileDataCacheHandler {
    typedef stdsptr<SoundCacheContainer> stdptrSCC;
    e_OBJECT
public:
    SoundDataHandler();

    void clearCache() {
        mSecondsCache.clear();
    }
    void afterSourceChanged();

    const HddCachableCacheHandler& getCacheHandler() const {
        return mSecondsCache;
    }

    void addSecondReader(const int secondId,
                         const stdsptr<SoundReaderForMerger>& reader) {
        mSecondsBeingRead << secondId;
        mSecondReaders << reader;
    }

    stdsptr<Samples> getSamplesForSecond(const int secondId) {
        const auto cont = mSecondsCache.atFrame
                <SoundCacheContainer>(secondId);
        if(!cont) return nullptr;
        return cont->getSamples();
    }

    SoundReaderForMerger * getSecondReader(const int second) {
        const int id = mSecondsBeingRead.indexOf(second);
        if(id >= 0) return mSecondReaders.at(id).get();
        return nullptr;
    }

    void removeSecondReader(const int second) {
        const int id = mSecondsBeingRead.indexOf(second);
        mSecondsBeingRead.removeAt(id);
        mSecondReaders.removeAt(id);
    }

    void secondReaderFinished(const int secondId,
                             const stdsptr<Samples>& samples) {
        mSecondsCache.add(enve::make_shared<SoundCacheContainer>(
                              samples, iValueRange{secondId, secondId},
                              &mSecondsCache));
    }
private:
    QList<int> mSecondsBeingRead;
    QList<stdsptr<SoundReaderForMerger>> mSecondReaders;
    HddCachableCacheHandler mSecondsCache;
};

class CORE_EXPORT SoundHandler : public StdSelfRef {
    typedef stdsptr<SoundCacheContainer> stdptrSCC;
    e_OBJECT
public:
    SoundHandler(SoundDataHandler* const dataHandler) :
        mDataHandler(dataHandler) {
        openAudioStream();
    }

    void afterSourceChanged() {
        openAudioStream();
    }

    stdsptr<Samples> getSamplesForSecond(const int secondId);

    void secondReaderFinished(const int secondId,
                              const stdsptr<Samples>& samples);
    void secondReaderCanceled(const int secondId) {
        removeSecondReader(secondId);
    }

    std::function<void(int)> mOnSecondCached;

    SoundReaderForMerger * getSecondReader(const int second) {
        return mDataHandler->getSecondReader(second);
    }

    SoundReaderForMerger * addSecondReader(const int secondId);

    int durationSecCeil() const {
        return qCeil(durationSec());
    }

    qreal durationSec() const {
        if(!mAudioStreamsData) return 0;
        return mAudioStreamsData->fDurationSec;
    }

    SoundDataHandler* getDataHandler() const {
        return mDataHandler;
    }
    const HddCachableCacheHandler& getCacheHandler() const {
        return mDataHandler->getCacheHandler();
    }

    const QString& getFilePath() const {
        return mDataHandler->getFilePath();
    }
protected:
    void removeSecondReader(const int second) {
        mDataHandler->removeSecondReader(second);
    }
private:
    void openAudioStream() {
        const auto filePath = mDataHandler->getFilePath();
        mAudioStreamsData = AudioStreamsData::sOpen(filePath);
    }

    SoundDataHandler* const mDataHandler;
    stdsptr<AudioStreamsData> mAudioStreamsData;

};

class CORE_EXPORT SoundFileHandler : public FileCacheHandler {
    e_OBJECT
protected:
    SoundFileHandler() {}

    void reload() {
        mDataHandler.reset();
        if(fileMissing()) return;
        mDataHandler = SoundDataHandler::sGetCreateDataHandler<SoundDataHandler>(path());
    }
public:
    void replace();
private:
    qsptr<SoundDataHandler> mDataHandler;
};

#endif // SOUNDCACHEHANDLER_H
