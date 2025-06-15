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

#include "esound.h"

#include "soundcomposition.h"


eSound::eSound() : eBoxOrSound("sound") {
    connect(this, &eBoxOrSound::aboutToChangeAncestor, this, [this]() {
        const auto pScene = getParentScene();
        if(!pScene) return;
        pScene->getSoundComposition()->removeSound(ref<eSound>());
    });
    connect(this, &eBoxOrSound::prp_ancestorChanged, this, [this]() {
        const auto pScene = getParentScene();
        if(!pScene) return;
        pScene->getSoundComposition()->addSound(ref<eSound>());
    });
}

SampleRange eSound::relSampleRange() const {
    const qreal fps = getCanvasFPS();
    const auto relFrameRange = prp_relInfluenceRange();
    const auto qRelFrameRange = qValueRange{qreal(relFrameRange.fMin),
                                            qreal(relFrameRange.fMax)};
    const auto qSampleRange = qRelFrameRange*(eSoundSettings::sSampleRate()/fps);
    return {qFloor(qSampleRange.fMin), qCeil(qSampleRange.fMax)};
}

SampleRange eSound::absSampleRange() const {
    const qreal fps = getCanvasFPS();
    const auto absFrameRange = prp_relRangeToAbsRange(prp_relInfluenceRange());
    const auto qAbsFrameRange = qValueRange{qreal(absFrameRange.fMin),
                                            qreal(absFrameRange.fMax)};
    const auto qSampleRange = qAbsFrameRange*(eSoundSettings::sSampleRate()/fps);
    return {qFloor(qSampleRange.fMin), qCeil(qSampleRange.fMax)};
}

int eSound::getSampleShift() const{
    const qreal fps = getCanvasFPS();
    return qRound(prp_getTotalFrameShift()*(eSoundSettings::sSampleRate()/fps));
}

qreal eSound::getCanvasFPS() const {
    const auto pScene = getParentScene();
    if(!pScene) return 1;
    return pScene->fps();
}

iValueRange eSound::absSecondToRelSeconds(const int absSecond) {
    if(getStretch() < 0) {
        const auto absStretch = absSecondToRelSecondsAbsStretch(absSecond);
        const int secs = durationSecondsCeil();
        return {secs - absStretch.fMax, secs - absStretch.fMin};
    }
    return absSecondToRelSecondsAbsStretch(absSecond);
}

iValueRange eSound::absSecondToRelSecondsAbsStretch(const int absSecond) {
    const qreal fps = getCanvasFPS();
    const qreal stretch = qAbs(getStretch());
    const qreal speed = isZero6Dec(stretch) ? TEN_MIL : 1/stretch;
    const qreal qFirstSecond = prp_absFrameToRelFrameF(absSecond*fps)*speed/fps;
    if(isInteger4Dec(qFirstSecond)) {
        const int round = qRound(qFirstSecond);
        if(isOne4Dec(stretch) || stretch > 1) {
            return {round, round};
        }
        const qreal qLast = qFirstSecond + speed - 1;
        if(isInteger4Dec(qLast)) {
            const int roundLast = qRound(qLast);
            return {round, roundLast};
        }
        const int ceilLast = qMax(round, qCeil(qLast));
        return {round, ceilLast};
    }
    const int firstSecond = qFloor(qFirstSecond);
    const int lastSecond = qCeil(qFirstSecond + speed - 1);
    return {firstSecond, lastSecond};
}
