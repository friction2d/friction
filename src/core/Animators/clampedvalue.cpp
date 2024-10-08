/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Friction contributors
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

#include "clampedvalue.h"

ClampedValue::ClampedValue(const qreal value) :
    mValue(value) {}

void ClampedValue::saveValue() {
    mSavedValue = mValue;
}

void ClampedValue::restoreSavedValue() {
    mValue = mSavedValue;
}

qreal ClampedValue::getRawSavedValue() const {
    return mSavedValue;
}

qreal ClampedValue::getRawValue() const {
    if (mValue != mValue) { return 0.; }
    return mValue;
}

qreal ClampedValue::getClampedSavedValue() const {
    return clamp(mSavedValue, mMin, mMax);
}

qreal ClampedValue::getClampedValue() const {
    return clamp(mValue, mMin, mMax);
}

void ClampedValue::setValue(const qreal value) {
    mValue = value;
}

void ClampedValue::clampValue() {
    mValue = getClampedValue();
}

void ClampedValue::setRange(const qreal min, const qreal max) {
    mMin = min;
    mMax = max;
}

void ClampedValue::setMax(const qreal max) {
    mMax = max;
}

void ClampedValue::setMin(const qreal min) {
    mMin = min;
}
