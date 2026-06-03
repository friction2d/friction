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
*/

#include "lottie/lottierealkeyframes.h"

#include "Animators/qpointfanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/qrealkey.h"
#include "lottie/lottieanimatedproperty.h"
#include "simplemath.h"

#include <QList>
#include <QtMath>

namespace {

QJsonArray scalarArray(const qreal value)
{
    return QJsonArray{value};
}

QJsonObject easeObject(const QList<qreal>& x,
                       const QList<qreal>& y)
{
    QJsonArray xArray;
    QJsonArray yArray;
    for (const auto value : x) { xArray.append(qBound(qreal(0), value, qreal(1))); }
    for (const auto value : y) { yArray.append(value); }
    return QJsonObject{
        {QStringLiteral("x"), xArray},
        {QStringLiteral("y"), yArray}
    };
}

qreal normalizedValue(const qreal value,
                      const qreal start,
                      const qreal end,
                      const qreal fallback)
{
    const qreal delta = end - start;
    if (isZero4Dec(delta)) { return fallback; }
    return (value - start)/delta;
}

struct ScalarKeys {
    QList<QrealKey*> keys;
    bool valid = false;
};

ScalarKeys scalarKeys(QrealAnimator* const animator,
                      const FrameRange& frameRange)
{
    ScalarKeys result;
    if (!animator || animator->hasExpression()) { return result; }

    const auto& keys = animator->anim_getKeys();
    if (keys.count() < 2) { return result; }

    for (const auto& key : keys) {
        const auto realKey = static_cast<QrealKey*>(key);
        const int frame = realKey->getAbsFrame();
        if (frame < frameRange.fMin || frame > frameRange.fMax) { continue; }
        result.keys << realKey;
    }

    if (result.keys.size() < 2) { return result; }
    if (result.keys.first()->getAbsFrame() != frameRange.fMin) { return result; }
    if (result.keys.last()->getAbsFrame() != frameRange.fMax) { return result; }

    result.valid = true;
    return result;
}

QJsonObject scalarEaseIn(QrealKey* const prev,
                         QrealKey* const next)
{
    const qreal startFrame = prev->getAbsFrame();
    const qreal endFrame = next->getAbsFrame();
    const qreal span = qMax(qreal(1), endFrame - startFrame);
    const qreal x = (next->getC0AbsFrame() - startFrame)/span;
    const qreal y = normalizedValue(next->getC0Value(),
                                    prev->getValue(),
                                    next->getValue(),
                                    1);
    return easeObject({x}, {y});
}

QJsonObject scalarEaseOut(QrealKey* const prev,
                          QrealKey* const next)
{
    const qreal startFrame = prev->getAbsFrame();
    const qreal endFrame = next->getAbsFrame();
    const qreal span = qMax(qreal(1), endFrame - startFrame);
    const qreal x = (prev->getC1AbsFrame() - startFrame)/span;
    const qreal y = normalizedValue(prev->getC1Value(),
                                    prev->getValue(),
                                    next->getValue(),
                                    0);
    return easeObject({x}, {y});
}

bool compatiblePointKeys(const ScalarKeys& x,
                         const ScalarKeys& y)
{
    if (!x.valid || !y.valid) { return false; }
    if (x.keys.size() != y.keys.size()) { return false; }
    for (int i = 0; i < x.keys.size(); i++) {
        if (x.keys.at(i)->getAbsFrame() != y.keys.at(i)->getAbsFrame()) {
            return false;
        }
    }
    return true;
}

QJsonObject pointEaseIn(QrealKey* const xPrev,
                        QrealKey* const xNext,
                        QrealKey* const yPrev,
                        QrealKey* const yNext,
                        const int dimensions)
{
    const qreal xStartFrame = xPrev->getAbsFrame();
    const qreal xSpan = qMax(qreal(1), qreal(xNext->getAbsFrame() - xPrev->getAbsFrame()));
    const qreal yStartFrame = yPrev->getAbsFrame();
    const qreal ySpan = qMax(qreal(1), qreal(yNext->getAbsFrame() - yPrev->getAbsFrame()));
    QList<qreal> xs{
        (xNext->getC0AbsFrame() - xStartFrame)/xSpan,
        (yNext->getC0AbsFrame() - yStartFrame)/ySpan
    };
    QList<qreal> ys{
        normalizedValue(xNext->getC0Value(), xPrev->getValue(), xNext->getValue(), 1),
        normalizedValue(yNext->getC0Value(), yPrev->getValue(), yNext->getValue(), 1)
    };
    while (xs.size() < dimensions) { xs << 1; }
    while (ys.size() < dimensions) { ys << 1; }
    return easeObject(xs, ys);
}

QJsonObject pointEaseOut(QrealKey* const xPrev,
                         QrealKey* const xNext,
                         QrealKey* const yPrev,
                         QrealKey* const yNext,
                         const int dimensions)
{
    const qreal xStartFrame = xPrev->getAbsFrame();
    const qreal xSpan = qMax(qreal(1), qreal(xNext->getAbsFrame() - xPrev->getAbsFrame()));
    const qreal yStartFrame = yPrev->getAbsFrame();
    const qreal ySpan = qMax(qreal(1), qreal(yNext->getAbsFrame() - yPrev->getAbsFrame()));
    QList<qreal> xs{
        (xPrev->getC1AbsFrame() - xStartFrame)/xSpan,
        (yPrev->getC1AbsFrame() - yStartFrame)/ySpan
    };
    QList<qreal> ys{
        normalizedValue(xPrev->getC1Value(), xPrev->getValue(), xNext->getValue(), 0),
        normalizedValue(yPrev->getC1Value(), yPrev->getValue(), yNext->getValue(), 0)
    };
    while (xs.size() < dimensions) { xs << 0; }
    while (ys.size() < dimensions) { ys << 0; }
    return easeObject(xs, ys);
}

}

QJsonObject LottieRealKeyframes::scalar(QrealAnimator* const animator,
                                        const FrameRange& frameRange,
                                        const ScalarValue& value)
{
    const auto keys = scalarKeys(animator, frameRange);
    if (!keys.valid) { return QJsonObject(); }

    QJsonArray keyframes;
    for (int i = 0; i < keys.keys.size(); i++) {
        const auto key = keys.keys.at(i);
        const qreal rawValue = key->getValue();
        QJsonObject keyframe;
        keyframe.insert(QStringLiteral("t"), key->getAbsFrame());
        keyframe.insert(QStringLiteral("s"), scalarArray(value ? value(rawValue) : rawValue));
        if (i + 1 < keys.keys.size()) {
            const auto next = keys.keys.at(i + 1);
            keyframe.insert(QStringLiteral("i"), scalarEaseIn(key, next));
            keyframe.insert(QStringLiteral("o"), scalarEaseOut(key, next));
        }
        keyframes.append(keyframe);
    }

    return QJsonObject{
        {QStringLiteral("a"), 1},
        {QStringLiteral("k"), keyframes}
    };
}

QJsonObject LottieRealKeyframes::point(QPointFAnimator* const animator,
                                       const FrameRange& frameRange,
                                       const PointValue& value)
{
    if (!animator || !value) { return QJsonObject(); }

    const auto xKeys = scalarKeys(animator->getXAnimator(), frameRange);
    const auto yKeys = scalarKeys(animator->getYAnimator(), frameRange);
    if (!compatiblePointKeys(xKeys, yKeys)) { return QJsonObject(); }

    QJsonArray keyframes;
    for (int i = 0; i < xKeys.keys.size(); i++) {
        const auto xKey = xKeys.keys.at(i);
        const auto yKey = yKeys.keys.at(i);
        const QPointF point(xKey->getValue(), yKey->getValue());
        const auto lottieValue = value(point, xKey->getAbsFrame());
        QJsonObject keyframe;
        keyframe.insert(QStringLiteral("t"), xKey->getAbsFrame());
        keyframe.insert(QStringLiteral("s"), lottieValue);
        if (i + 1 < xKeys.keys.size()) {
            keyframe.insert(QStringLiteral("i"),
                            pointEaseIn(xKey, xKeys.keys.at(i + 1),
                                        yKey, yKeys.keys.at(i + 1),
                                        lottieValue.size()));
            keyframe.insert(QStringLiteral("o"),
                            pointEaseOut(xKey, xKeys.keys.at(i + 1),
                                         yKey, yKeys.keys.at(i + 1),
                                         lottieValue.size()));
        }
        keyframes.append(keyframe);
    }

    return QJsonObject{
        {QStringLiteral("a"), 1},
        {QStringLiteral("k"), keyframes}
    };
}
