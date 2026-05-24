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

#ifndef RANGEMAP_H
#define RANGEMAP_H
#include "exceptions.h"
#include "framerange.h"

template <class T>
class RangeMap : public std::map<iValueRange, T> {
    typedef typename std::map<iValueRange, T>::iterator iterator;
    typedef typename std::map<iValueRange, T>::const_iterator const_iterator;
public:
    //! @brief Returns iterator range { first, end }
    std::pair<iterator, iterator> range(const iValueRange &range) {
        if(!range.isValid()) return { this->end(), this->end() };
        return {atOrAfterFrame(range.fMin), afterFrame(range.fMax)};
    }

    //! @brief Returns iterator range { first, end }
    std::pair<const_iterator, const_iterator> range(const iValueRange &range) const {
        if(!range.isValid()) return { this->end(), this->end() };
        return {atOrAfterFrame(range.fMin), afterFrame(range.fMax)};
    }

    int count() const { return this->size(); }

    iValueRange firstEmptyRangeUpperBound(const int keyMin) const {
        return firstEmptyRangeLowerBound(keyMin + 1);
    }

    iValueRange firstEmptyRangeLowerBound(const int keyMin) const {
        int currKeyMin = keyMin;
        while(true) {
            const auto it = atFrame(currKeyMin);
            if(it == this->end()) {
                const auto after = afterFrame(currKeyMin);
                if(after == this->end()) return {currKeyMin, iValueRange::EMAX};
                const int lastEmpty = after->first.adjusted(-1, 0).fMin;
                return {currKeyMin, lastEmpty};
            }
            currKeyMin = it->first.fMax + 1;
        }
    }

    QList<iValueRange> getMissingRanges(const iValueRange &range) const {
        QList<iValueRange> result;

        int currKeyMin = range.fMin;
        while(currKeyMin < range.fMax) {
            const auto empty = firstEmptyRangeLowerBound(currKeyMin);
            result << empty;
            currKeyMin = empty.fMax + 1;
        }

        return result;
    }

    bool isEmpty() const { return this->empty(); }

    iterator atFrame(const int frame) {
        const auto it = atOrAfterFrame(frame);
        if(it.first.inRange(frame)) return it;
        else return this->end();
    }

    const_iterator atFrame(const int frame) const {
        const auto it = atOrAfterFrame(frame);
        if(it != this->end() && it->first.inRange(frame)) return it;
        // atOrAfterFrame uses lower_bound({frame,frame}) which skips entries with fMin < frame.
        // Fall back to atOrBeforeFrame to catch entries whose range contains frame.
        const auto before = atOrBeforeFrame(frame);
        if(before != this->end() && before->first.inRange(frame)) return before;
        return this->end();
    }

    iterator atOrBeforeFrame(const int frame) {
        auto nextIt = afterFrame(frame);
        if(nextIt == this->begin()) return this->end();
        return --nextIt;
    }

    const_iterator atOrBeforeFrame(const int frame) const {
        auto nextIt = afterFrame(frame);
        if(nextIt == this->begin()) return this->end();
        return --nextIt;
    }

    iterator atOrAfterFrame(const int frame) {
        return this->lower_bound({frame, frame});
    }

    const_iterator atOrAfterFrame(const int frame) const {
        return this->lower_bound({frame, frame});
    }

    iterator beforeFrame(const int frame) {
        auto nextIt = atOrAfterFrame(frame);
        if(nextIt == this->begin()) return this->end();
        return --nextIt;
    }

    const_iterator beforeFrame(const int frame) const {
        auto nextIt = atOrAfterFrame(frame);
        if(nextIt == this->begin()) return this->end();
        return --nextIt;
    }

    iterator afterFrame(const int frame) {
        return this->upper_bound({frame, frame});
    }

    const_iterator afterFrame(const int frame) const {
        return this->upper_bound({frame, frame});
    }
};

#endif // RANGEMAP_H
