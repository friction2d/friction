#include "usedrange.h"
#include "hddcachablecachehandler.h"
#include <QLoggingCategory>

UsedRange::UsedRange(HddCachableCacheHandler * const handler) :
    mHandler(handler), mRange(iValueRange::INVALID) {}

void UsedRange::addIfInRange(UsedRange::Cont * const cont) {
    if(mRange.overlaps(cont->getRange()))
        cont->incInUse();
}

void UsedRange::removeIfInRange(UsedRange::Cont * const cont) {
    if(mRange.overlaps(cont->getRange()))
        cont->decInUse();
}

void UsedRange::replaceRange(const iValueRange &range) {
    const auto newblocks = iValueRange::sDiff(range, mRange);
    for(const auto newblock : newblocks) {
        const auto its = mHandler->mConts.range(newblock);
        for(auto it = its.first; it != its.second; it++) {
            if(mRange.overlaps(it->first)) continue;
            it->second->incInUse();
        }
    }

    const auto unblocks = iValueRange::sDiff(mRange, range);
    for(const auto unblock : unblocks) {
        const auto its = mHandler->mConts.range(unblock);
        for(auto it = its.first; it != its.second; it++) {
            if(range.overlaps(it->first)) continue;
            it->second->decInUse();
        }
    }

    mRange = range;
}

void UsedRange::setRangeMin(const int min) {
    qCDebug(lcCacheHandler) << "UsedRange::setRangeMin:" << min
             << "(was" << mRange.fMin << "-" << mRange.fMax << ")";
    if(validRange()) incRangeMin(min - mRange.fMin);
    else replaceRange({min, min});
}

void UsedRange::setRangeMax(const int max) {
    qCDebug(lcCacheHandler) << "UsedRange::setRangeMax:" << max
             << "(was" << mRange.fMin << "-" << mRange.fMax << ")";
    if(validRange()) incRangeMax(max - mRange.fMax);
    else replaceRange({max, max});
}

void UsedRange::incRangeMax(const int inc) {
    if(!validRange() || inc == 0) return;
    if(inc > 0) {
        const auto its = mHandler->mConts.range(
            {mRange.fMax + 1, mRange.fMax + inc});
        for(auto it = its.first; it != its.second; it++) {
            if(mRange.overlaps(it->first)) continue;
            it->second->incInUse();
        }
        mRange.fMax += inc;
    } else {
        const int newMax = mRange.fMax + static_cast<int>(inc);
        const auto its = mHandler->mConts.range(
            {qMax(mRange.fMin, newMax + 1), mRange.fMax});
        mRange.fMax = newMax;
        for(auto it = its.first; it != its.second; it++) {
            if(mRange.overlaps(it->first)) continue;
            it->second->decInUse();
        }
    }
}

void UsedRange::incRangeMin(const int inc) {
    if(!validRange() || inc == 0) return;
    if(inc > 0) {
        const int newMin = mRange.fMin + static_cast<int>(inc);
        const auto its = mHandler->mConts.range(
            {mRange.fMin, qMin(mRange.fMax, newMin - 1)});
        mRange.fMin = newMin;
        for(auto it = its.first; it != its.second; it++) {
            if(mRange.overlaps(it->first)) continue;
            it->second->decInUse();
        }
    } else {
        const auto its = mHandler->mConts.range(
            {mRange.fMin + inc, mRange.fMin - 1});
        for(auto it = its.first; it != its.second; it++) {
            if(mRange.overlaps(it->first)) continue;
            it->second->incInUse();
        }
        mRange.fMin += inc;
    }
}

void UsedRange::clearRange() {
    qCDebug(lcCacheHandler) << "UsedRange::clearRange (was" << mRange.fMin << "-" << mRange.fMax << ")";
    replaceRange(iValueRange::INVALID);
}
