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

#ifndef HddCACHABLECACHEHANDLER_H
#define HddCACHABLECACHEHANDLER_H

#include "hddcachablerangecont.h"
#include "rangemap.h"
#include "usepointer.h"
#include "usedrange.h"

#include <QPainter>
#include <QLoggingCategory>
#include <stdexcept>

Q_DECLARE_LOGGING_CATEGORY(lcCacheHandler)

class CORE_EXPORT HddCachableCacheHandler {
    friend class UsedRange;
public:
    typedef HddCachableRangeCont Cont;

    HddCachableCacheHandler() : mUsedRange(this) {}

    void drawCacheOnTimeline(QPainter * const p,
                             const QRectF &drawRect,
                             const int startFrame,
                             const int endFrame,
                             const qreal unit = 1,
                             const int maxX = INT_MAX/2) const;

    int firstEmptyFrameAtOrAfter(const int frame) const {
        return mConts.firstEmptyRangeLowerBound(frame).fMin;
    }

    void add(const stdsptr<Cont>& cont) {
        const auto range = cont->getRange();
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::add range=[" << range.fMin << "," << range.fMax << "] size_before=" << mConts.count();
        const auto ret = mConts.insert({range, cont});
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::add inserted=" << ret.second << "size_after=" << mConts.count();
        if(ret.second) mUsedRange.addIfInRange(cont.get());
    }

    void setNoClear(const bool noClear) { mNoClear = noClear; }

    void clear() {
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::clear size=" << mConts.count();
        if(mNoClear) throw std::runtime_error("HddCachableCacheHandler::clear called with mNoClear=true");
        mConts.clear();
    }

    void remove(const stdsptr<Cont>& cont) {
        const auto range = cont->getRange();
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::remove(cont) range=[" << range.fMin << "," << range.fMax << "]";
        mConts.erase(range);
    }

    void remove(const iValueRange& range) {
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::remove(range) range=[" << range.fMin << "," << range.fMax << "] size_before=" << mConts.count() << " mNoClear=" << mNoClear;
        mConts.erase(range);
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::remove(range) size_after=" << mConts.count();
    }

    template <class T = Cont>
    T * atFrame(const int relFrame) const {
        const auto it = mConts.atFrame(relFrame);
        if(it == mConts.end()) {
            qCDebug(lcCacheHandler) << "HddCachableCacheHandler::atFrame(" << relFrame << ") MISS size=" << mConts.count();
            return nullptr;
        }
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::atFrame(" << relFrame << ") HIT range=[" << it->first.fMin << "," << it->first.fMax << "]";
        return static_cast<T*>(it->second.get());
    }

    template <class T = Cont>
    T * atOrBeforeFrame(const int relFrame) const {
        const auto it = mConts.atOrBeforeFrame(relFrame);
        if(it == mConts.end()) return nullptr;
        return static_cast<T*>(it->second.get());
    }

    void setUseRange(const iValueRange& range) {
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::setUseRange()";
        mUsedRange.replaceRange(range);
    }

    void setMaxUseRange(const int max) {
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::setMaxUseRange()";
        mUsedRange.setRangeMax(max);
    }

    void setMinUseRange(const int min) {
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::setMinUseRange()";
        mUsedRange.setRangeMin(min);
    }

    void clearUseRange() {
        qCDebug(lcCacheHandler) << "HddCachableCacheHandler::clearUseRang()";
        if(mNoClear) throw std::runtime_error("HddCachableCacheHandler::clearUseRange called with mNoClear=true");
        mUsedRange.clearRange();
    }

    auto begin() const { return mConts.begin(); }
    auto end() const { return mConts.begin(); }
private:
    RangeMap<stdsptr<Cont>> mConts;
    UsedRange mUsedRange;
    bool mNoClear = false;
};

#endif // HddCACHABLECACHEHANDLER_H
