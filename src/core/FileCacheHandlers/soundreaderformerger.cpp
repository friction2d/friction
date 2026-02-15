#include "soundreaderformerger.h"

#include "Sound/soundmerger.h"

SoundReaderForMerger::SoundReaderForMerger(
        SoundHandler* const cacheHandler,
        const stdsptr<AudioStreamsData>& openedAudio,
        const int secondId, const SampleRange& sampleRange) :
    SoundReader(cacheHandler, openedAudio, secondId, sampleRange) {}

void SoundReaderForMerger::afterProcessing() {
    for(const auto& merger : qAsConst(mMergers)) {
        if(!merger) continue;
        for(const auto& ss : qAsConst(mSSAbsRanges)) {
            merger->addSoundToMerge({ss.fSampleShift, ss.fSamplesRange,
                                     ss.fVolume, ss.fSpeed,
                                     enve::make_shared<Samples>(getSamples())});
        }
    }
    SoundReader::afterProcessing();
}

void SoundReaderForMerger::addSingleSound(void* const soundPtr,
                                          const int sampleShift,
                                          const SampleRange& absRange,
                                          const QrealSnapshot& volume,
                                          const qreal speed) {
    const bool c = mSoundPtrs.contains(soundPtr);
    if(c) return;
    mSoundPtrs.append(soundPtr);
    mSSAbsRanges.append({sampleShift, absRange, volume, speed});
}

void SoundReaderForMerger::addMerger(SoundMerger* const merger) {
    mMergers << merger;
}
