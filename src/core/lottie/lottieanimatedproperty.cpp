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

#include "lottie/lottieanimatedproperty.h"

#include <QSet>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

bool sameScalarValues(const QList<qreal>& values,
                      const qreal tolerance)
{
    if (values.isEmpty()) { return true; }
    const qreal first = values.first();
    for (const qreal value : values) {
        if (qAbs(first - value) > tolerance) { return false; }
    }
    return true;
}

bool samePointValues(const QList<QJsonArray>& values,
                     const qreal tolerance)
{
    if (values.isEmpty()) { return true; }
    const auto first = values.first();
    for (const auto& value : values) {
        if (value.size() != first.size()) { return false; }
        for (int i = 0; i < value.size(); i++) {
            if (qAbs(first.at(i).toDouble() - value.at(i).toDouble()) > tolerance) {
                return false;
            }
        }
    }
    return true;
}

qreal interpolated(const qreal start,
                   const qreal end,
                   const qreal progress)
{
    return start + (end - start)*progress;
}

qreal scalarError(const QList<qreal>& values,
                  const int start,
                  const int end,
                  int& worstIndex)
{
    qreal worstError = 0;
    worstIndex = -1;
    const int span = end - start;
    if (span <= 1) { return worstError; }

    for (int i = start + 1; i < end; i++) {
        const qreal progress = qreal(i - start)/span;
        const qreal expected = interpolated(values.at(start),
                                           values.at(end),
                                           progress);
        const qreal error = qAbs(values.at(i) - expected);
        if (error > worstError) {
            worstError = error;
            worstIndex = i;
        }
    }
    return worstError;
}

qreal pointError(const QList<QJsonArray>& values,
                 const int start,
                 const int end,
                 int& worstIndex)
{
    qreal worstError = 0;
    worstIndex = -1;
    const int span = end - start;
    if (span <= 1) { return worstError; }

    const auto startValue = values.at(start);
    const auto endValue = values.at(end);
    if (startValue.size() != endValue.size()) {
        worstIndex = start + 1;
        return std::numeric_limits<qreal>::max();
    }

    for (int i = start + 1; i < end; i++) {
        const auto value = values.at(i);
        if (value.size() != startValue.size()) {
            worstIndex = i;
            return std::numeric_limits<qreal>::max();
        }

        const qreal progress = qreal(i - start)/span;
        qreal error = 0;
        for (int c = 0; c < value.size(); c++) {
            const qreal expected = interpolated(startValue.at(c).toDouble(),
                                               endValue.at(c).toDouble(),
                                               progress);
            error = qMax(error, qAbs(value.at(c).toDouble() - expected));
        }

        if (error > worstError) {
            worstError = error;
            worstIndex = i;
        }
    }
    return worstError;
}

template<typename ErrorFunc>
void simplifyRange(const int start,
                   const int end,
                   const qreal tolerance,
                   const ErrorFunc& errorFunc,
                   QSet<int>& indices)
{
    int worstIndex = -1;
    const qreal error = errorFunc(start, end, worstIndex);
    if (worstIndex < 0 || error <= tolerance) { return; }

    indices.insert(worstIndex);
    simplifyRange(start, worstIndex, tolerance, errorFunc, indices);
    simplifyRange(worstIndex, end, tolerance, errorFunc, indices);
}

QList<int> simplifiedScalarIndices(const QList<qreal>& values,
                                   const qreal tolerance)
{
    QList<int> result;
    if (values.isEmpty()) { return result; }
    if (values.size() == 1) { return QList<int>{0}; }

    QSet<int> indices{0, values.size() - 1};
    simplifyRange(0, values.size() - 1, tolerance,
                  [&values](const int start, const int end, int& worstIndex) {
                      return scalarError(values, start, end, worstIndex);
                  },
                  indices);
    result = indices.values();
    std::sort(result.begin(), result.end());
    return result;
}

QList<int> simplifiedPointIndices(const QList<QJsonArray>& values,
                                  const qreal tolerance)
{
    QList<int> result;
    if (values.isEmpty()) { return result; }
    if (values.size() == 1) { return QList<int>{0}; }

    QSet<int> indices{0, values.size() - 1};
    simplifyRange(0, values.size() - 1, tolerance,
                  [&values](const int start, const int end, int& worstIndex) {
                      return pointError(values, start, end, worstIndex);
                  },
                  indices);
    result = indices.values();
    std::sort(result.begin(), result.end());
    return result;
}

QJsonArray scalarKeyframes(const QList<qreal>& values,
                           const QList<int>& indices,
                           const FrameRange& frameRange)
{
    QJsonArray keyframes;
    for (int i = 0; i < indices.size(); i++) {
        const int sampleIndex = indices.at(i);
        QJsonObject key;
        key.insert(QStringLiteral("t"), frameRange.fMin + sampleIndex);
        key.insert(QStringLiteral("s"), QJsonArray{values.at(sampleIndex)});
        if (i + 1 < indices.size()) {
            const int nextIndex = indices.at(i + 1);
            key.insert(QStringLiteral("e"), QJsonArray{values.at(nextIndex)});
            key.insert(QStringLiteral("i"), LottieAnimatedProperty::keyframeEase());
            key.insert(QStringLiteral("o"), LottieAnimatedProperty::keyframeEase());
        }
        keyframes.append(key);
    }
    return keyframes;
}

QJsonArray pointKeyframes(const QList<QJsonArray>& values,
                          const QList<int>& indices,
                          const FrameRange& frameRange)
{
    QJsonArray keyframes;
    for (int i = 0; i < indices.size(); i++) {
        const int sampleIndex = indices.at(i);
        QJsonObject key;
        key.insert(QStringLiteral("t"), frameRange.fMin + sampleIndex);
        key.insert(QStringLiteral("s"), values.at(sampleIndex));
        if (i + 1 < indices.size()) {
            const int nextIndex = indices.at(i + 1);
            key.insert(QStringLiteral("e"), values.at(nextIndex));
            key.insert(QStringLiteral("i"), LottieAnimatedProperty::keyframeEase());
            key.insert(QStringLiteral("o"), LottieAnimatedProperty::keyframeEase());
        }
        keyframes.append(key);
    }
    return keyframes;
}

}

QJsonObject LottieAnimatedProperty::staticProperty(const QJsonValue& value)
{
    return QJsonObject{
        {QStringLiteral("a"), 0},
        {QStringLiteral("k"), value}
    };
}

QJsonObject LottieAnimatedProperty::scalar(const QList<qreal>& values,
                                           const FrameRange& frameRange,
                                           const qreal tolerance)
{
    if (values.isEmpty()) { return staticProperty(0); }
    if (sameScalarValues(values, tolerance)) { return staticProperty(values.first()); }

    const auto indices = simplifiedScalarIndices(values, tolerance);
    return QJsonObject{
        {QStringLiteral("a"), 1},
        {QStringLiteral("k"), scalarKeyframes(values, indices, frameRange)}
    };
}

QJsonObject LottieAnimatedProperty::point(const QList<QJsonArray>& values,
                                          const FrameRange& frameRange,
                                          const qreal tolerance)
{
    if (values.isEmpty()) { return staticProperty(QJsonArray()); }
    if (samePointValues(values, tolerance)) { return staticProperty(values.first()); }

    const auto indices = simplifiedPointIndices(values, tolerance);
    return QJsonObject{
        {QStringLiteral("a"), 1},
        {QStringLiteral("k"), pointKeyframes(values, indices, frameRange)}
    };
}

QJsonObject LottieAnimatedProperty::keyframeEase()
{
    return QJsonObject{
        {QStringLiteral("x"), QJsonArray{0.667}},
        {QStringLiteral("y"), QJsonArray{1}}
    };
}
