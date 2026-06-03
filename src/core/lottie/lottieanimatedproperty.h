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

#ifndef LOTTIEANIMATEDPROPERTY_H
#define LOTTIEANIMATEDPROPERTY_H

#include "framerange.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>

namespace LottieAnimatedProperty {

QJsonObject staticProperty(const QJsonValue& value);
QJsonObject scalar(const QList<qreal>& values,
                   const FrameRange& frameRange,
                   const qreal tolerance = 0.001);
QJsonObject point(const QList<QJsonArray>& values,
                  const FrameRange& frameRange,
                  const qreal tolerance = 0.001);
QJsonObject keyframeEase();

}

#endif // LOTTIEANIMATEDPROPERTY_H
