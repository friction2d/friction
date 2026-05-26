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
#include "../canvas.h"
#include "soundcomposition.h"
#include "CacheHandlers/samples.h"
#include "Timeline/durationrectangle.h"

#include <QPainter>
#include <QDateTime>
#include <cstring>

extern "C" {
#include <libavutil/samplefmt.h>
}

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
    return pScene->getFps();
}

iValueRange eSound::absSecondToRelSeconds(const int absSecond) {
    if(getStretch() < 0) {
        const auto absStretch = absSecondToRelSecondsAbsStretch(absSecond);
        const int secs = durationSecondsCeil();
        return {secs - absStretch.fMax, secs - absStretch.fMin};
    }
    return absSecondToRelSecondsAbsStretch(absSecond);
}

static float sampleToFloat(const Samples& s, const int idx) {
    const uchar* ptr;
    if (s.fPlanar) {
        ptr = s.fData[0] + static_cast<ulong>(idx) * s.fSampleSize;
    } else {
        ptr = s.fData[0] + static_cast<ulong>(idx) * s.fNChannels * s.fSampleSize;
    }
    switch (s.fFormat) {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        return (*ptr - 128) / 128.0f;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P: {
        int16_t v; memcpy(&v, ptr, 2);
        return v / 32768.0f;
    }
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P: {
        int32_t v; memcpy(&v, ptr, 4);
        return v / 2147483648.0f;
    }
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP: {
        float v; memcpy(&v, ptr, 4);
        return v;
    }
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_DBLP: {
        double v; memcpy(&v, ptr, 8);
        return float(v);
    }
    default:
        return 0.0f;
    }
}

void eSound::prp_drawTimelineControls(
        QPainter * const p, const qreal pixelsPerFrame,
        const FrameRange &absFrameRange, const int rowHeight) {
    drawDurationRectangle(p, pixelsPerFrame, absFrameRange, rowHeight);
    drawWaveform(p, pixelsPerFrame, absFrameRange, rowHeight);
    ComplexAnimator::prp_drawTimelineControls(p, pixelsPerFrame, absFrameRange, rowHeight);
}

void eSound::drawWaveform(
        QPainter * const p, const qreal pixelsPerFrame,
        const FrameRange &absFrameRange, const int rowHeight) {
    const auto durRect = getDurationRectangle();
    if (!durRect) return;

    const qreal fps = getCanvasFPS();
    if (fps <= 0) return;

    const int clampedMin = qMax(absFrameRange.fMin, durRect->getMinAbsFrame());
    const int clampedMax = qMin(absFrameRange.fMax, durRect->getMaxAbsFrame());
    if (clampedMin > clampedMax) return;

    const int firstRelDrawFrame = clampedMin - absFrameRange.fMin;
    const int lastRelDrawFrame  = clampedMax - absFrameRange.fMin;
    const int drawFrameSpan = lastRelDrawFrame - firstRelDrawFrame + 1;
    if (drawFrameSpan < 1) return;

    const int waveLeft  = qFloor((firstRelDrawFrame + 0.5) * pixelsPerFrame);
    const int waveWidth = qCeil((drawFrameSpan - 1.0) * pixelsPerFrame);
    if (waveWidth < 1) return;

    const int midY       = rowHeight / 2;
    const int halfHeight = midY - 2;
    if (halfHeight <= 0) return;

    const int audioMinAbsFrame = durRect->getMinAbsFrame();

    const int startSec       = qMax(0, qFloor((clampedMin - audioMinAbsFrame) / fps));
    const int endSec         = qFloor((clampedMax - audioMinAbsFrame) / fps);
    const int lastRequestedS = lastRequestedSecond();

    // Flash phase: 2Hz, driven by wall clock (500ms half-period)
    const bool flashOn = (QDateTime::currentMSecsSinceEpoch() / 500) % 2 == 0;
    const int loadAlpha = flashOn ? 80 : 30;
    const QColor loadColor(160, 200, 255, loadAlpha);

    p->save();

    // Pass 1: loading indicator for uncached-but-requested seconds
    if (lastRequestedS >= 0) {
        p->setPen(Qt::NoPen);
        p->setBrush(loadColor);
        for (int sec = startSec; sec <= qMin(endSec, lastRequestedS); ++sec) {
            if (getSamplesForSecond(sec)) continue; // already cached — skip

            const qreal secFrameStart = sec * fps + audioMinAbsFrame;
            const qreal secFrameEnd   = (sec + 1) * fps + audioMinAbsFrame;
            const int pixStart = qMax(waveLeft,
                qFloor((secFrameStart - absFrameRange.fMin + 0.5) * pixelsPerFrame));
            const int pixEnd = qMin(waveLeft + waveWidth,
                qCeil((secFrameEnd - absFrameRange.fMin + 0.5) * pixelsPerFrame));
            if (pixStart < pixEnd) {
                p->drawRect(pixStart, 1, pixEnd - pixStart, rowHeight - 2);
            }
        }

        // Frontier line: pixel position of the start of the next-to-request second
        const qreal frontierFrame = (lastRequestedS + 1) * fps + audioMinAbsFrame;
        const int frontierX = qFloor((frontierFrame - absFrameRange.fMin + 0.5) * pixelsPerFrame);
        if (frontierX >= waveLeft && frontierX < waveLeft + waveWidth) {
            p->setPen(QPen(QColor(200, 230, 255, 200), 1));
            p->drawLine(frontierX, 1, frontierX, rowHeight - 2);
        }
    }

    // Pass 2: waveform for cached seconds
    p->setPen(QPen(QColor(255, 255, 255, 180), 1));
    for (int sec = startSec; sec <= endSec; ++sec) {
        const auto samples = getSamplesForSecond(sec);
        if (!samples) continue;

        const int sr       = samples->fSampleRate;
        const int nSamples = samples->fSampleRange.span();
        if (nSamples <= 0 || sr <= 0) continue;

        const qreal absToSample = sr / fps;

        const qreal secFrameStart = sec * fps + audioMinAbsFrame;
        const qreal secFrameEnd   = (sec + 1) * fps + audioMinAbsFrame;
        const int pixStart = qMax(waveLeft,
            qFloor((secFrameStart - absFrameRange.fMin + 0.5) * pixelsPerFrame));
        const int pixEnd = qMin(waveLeft + waveWidth,
            qCeil((secFrameEnd - absFrameRange.fMin + 0.5) * pixelsPerFrame));

        for (int x = pixStart; x < pixEnd; ++x) {
            const qreal asL = ((absFrameRange.fMin + (x - 0.5) / pixelsPerFrame)
                               - audioMinAbsFrame) * absToSample;
            const qreal asR = ((absFrameRange.fMin + (x + 0.5) / pixelsPerFrame)
                               - audioMinAbsFrame) * absToSample;

            const int base = samples->fSampleRange.fMin;
            const int diS  = qMax(0,            qFloor(asL) - base);
            const int diE  = qMin(nSamples - 1, qFloor(asR) - base);
            if (diS > diE) continue;

            const int count  = diE - diS + 1;
            const int stride = qMax(1, count / 256);

            float peak = 0.0f;
            for (int i = diS; i <= diE; i += stride) {
                const float v = qAbs(sampleToFloat(*samples, i));
                if (v > peak) peak = v;
            }

            const int h = qRound(qMin(1.0f, peak) * halfHeight);
            if (h > 0) p->drawLine(x, midY - h, x, midY + h);
        }
    }

    p->restore();
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
