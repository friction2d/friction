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

#include "lottie/lottiejsonoptimizer.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QtMath>

namespace {

constexpr qreal kNumberScale = 1000;

qreal roundedNumber(const qreal value)
{
    const qreal rounded = qRound64(value*kNumberScale)/kNumberScale;
    return qFuzzyIsNull(rounded) ? 0 : rounded;
}

bool isKeyframeObject(const QJsonObject& object)
{
    return object.contains(QStringLiteral("t")) &&
           object.contains(QStringLiteral("s"));
}

QJsonValue optimizeValue(const QJsonValue& value);

QJsonObject optimizeObject(const QJsonObject& object)
{
    QJsonObject optimized;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        optimized.insert(it.key(), optimizeValue(it.value()));
    }
    return optimized;
}

QJsonArray optimizeArray(const QJsonArray& array)
{
    QJsonArray optimized;
    for (int i = 0; i < array.size(); i++) {
        auto value = optimizeValue(array.at(i));
        if (i + 1 < array.size() && value.isObject()) {
            auto object = value.toObject();
            const auto next = array.at(i + 1);
            if (isKeyframeObject(object) &&
                next.isObject() &&
                isKeyframeObject(next.toObject())) {
                object.remove(QStringLiteral("e"));
                value = object;
            }
        }
        optimized.append(value);
    }
    return optimized;
}

QJsonValue optimizeValue(const QJsonValue& value)
{
    if (value.isDouble()) { return roundedNumber(value.toDouble()); }
    if (value.isArray()) { return optimizeArray(value.toArray()); }
    if (value.isObject()) { return optimizeObject(value.toObject()); }
    return value;
}

}

QJsonObject LottieJsonOptimizer::optimize(const QJsonObject& root)
{
    return optimizeObject(root);
}
